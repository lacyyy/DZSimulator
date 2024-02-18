#include "coll/BVH.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stack>
#include <span>
#include <vector>

#include <Tracy.hpp>

#include <Corrade/Containers/Optional.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld.h"
#include "coll/CollidableWorld_Impl.h"
#include "coll/CollidableWorld-funcbrush.h"
#include "coll/CollidableWorld-xprop.h"
#include "coll/Debugger.h"
#include "coll/SweptTrace.h"
#include "csgo_parsing/BrushSeparation.h"
#include "csgo_parsing/BspMap.h"
#include "csgo_parsing/utils.h"
#include "utils_3d.h"

using namespace coll;
using namespace Magnum;
using namespace csgo_parsing;

// @Optimization Is "Intel Embree" an option to speed up ray intersections?
// @Optimization Look up BVH optimizations in https://github.com/brandonpelfrey/Fast-BVH
// @Optimization Save memory: Store BVH AABB values as int16 (-32768 to 32767)
//               instead of float. Increase exact AABB to nearest integers.

#define PRINT_PREFIX "[BVH]"

BVH::BVH(CollidableWorld& c_world)
{
    // @Optimization Is there potential to parallelize BVH creation?
    bool leaf_creation_success = CreateLeaves(c_world);

    total_leaf_cnt = leaves.size() - 1; // Don't count dummy leaf entry
    Debug{} << PRINT_PREFIX << total_leaf_cnt << "leafs were constructed";

    // Need at least 2 leaves to construct BVH
    if (!leaf_creation_success || total_leaf_cnt < 2) {
        // This is not a fatal error, the user is still able to enter the map.
        Debug{} << PRINT_PREFIX << "WARNING: BVH tree was not created since "
            "something went wrong during leaf creation. BVH traces will do nothing.";
        return; // nodes array is left empty, signaling BVH creation failure
    }

    // Assuming every node has 2 children and each child is a node or a leaf
    const size_t final_node_cnt = total_leaf_cnt - 1;
    nodes.clear();
    nodes.reserve(final_node_cnt);

    // Create leaf reference arrays for each axis.
    // All 3 contain the index to every leaf.
    std::vector<uint32_t> leaf_refs[3];
    for (int axis = 0; axis < 3; axis++) {
        leaf_refs[axis].reserve(total_leaf_cnt);
        for (size_t i = 1; i < leaves.size(); i++) { // Skip dummy leaf at index 0
            leaf_refs[axis].push_back(i);
        }
    }

    // Create root node
    nodes.push_back({});
    const uint32_t root_node_idx = 0;
    Node& root_node = nodes[root_node_idx];
    // Calc all-encompassing AABB
    CalcAabbOfBvhLeaves(leaf_refs[0], &root_node.mins, &root_node.maxs);

    // Sort leafs in the X, Y and Z leaf reference arrays along their respective
    // axis
    Debug{} << PRINT_PREFIX << "Initial leaf sort along axes...";
    for (int axis = 0; axis < 3; axis++) {
        std::sort(leaf_refs[axis].begin(), leaf_refs[axis].end(),
            [this, axis](uint32_t a, uint32_t b) { // Returns true if a is ordered before b
                // Calculate a's and b's centroid position along the axis
                const Leaf& a_leaf = this->leaves[a];
                const Leaf& b_leaf = this->leaves[b];
                float a_axis_pos = 0.5f * (a_leaf.mins[axis] + a_leaf.maxs[axis]);
                float b_axis_pos = 0.5f * (b_leaf.mins[axis] + b_leaf.maxs[axis]);
                return a_axis_pos < b_axis_pos;
            }
        );
    }

    // Assign the entire leaf range to the root node
    std::span<uint32_t> leaf_refs_sorted_along_axis[3] = {
        std::span<uint32_t>{ leaf_refs[0].begin(), total_leaf_cnt }, // along X axis
        std::span<uint32_t>{ leaf_refs[1].begin(), total_leaf_cnt }, // along Y axis
        std::span<uint32_t>{ leaf_refs[2].begin(), total_leaf_cnt }, // along Z axis
    };

    // Build BVH by iteratively splitting nodes down to the BVH leaves
    CreateNodeHierarchy(root_node_idx, leaf_refs_sorted_along_axis, c_world);

    // Set information in each node about the leaves they contain.
    SetNodeContentsInfo();

    Debug{} << PRINT_PREFIX << nodes.size() << "nodes were constructed";
}

bool BVH::WasConstructedSuccessfully()
{
    // A valid BVH must have at least one node and 2 leaves.
    return nodes.size() != 0 && total_leaf_cnt >= 2;
}

void BVH::DoSweptTrace(SweptTrace* trace, CollidableWorld& c_world)
{
    ZoneScoped;

    // @Optimization To improve BVH traversal time, build the BVH in a compact
    //               and cache-friendly representation. E.g.: Nodes and leafs
    //               can be stored in a single array, with *depth-first order*.
    //               Note: After changing layout of the nodes array, take a look
    //                     at possibly optimizing BVH::SetNodeContentsInfo().

    if (!WasConstructedSuccessfully())
        return; // Can't trace against non-existent BVH
    const Node& root_node = nodes[0];

#if 0 // Debugging switch
    // Trace against all leaves for debugging purposes
    for (size_t i = 1; i < leaves.size(); i++)
        DoSweptTraceAgainstLeaf(trace, leaves[i], c_world);
    return;
#endif

    // @Optimization Doing an intersection between the AABB that encloses the
    //               trace sweep and the AABB of the root BVH node is possibly
    //               cheaper than doing an accurate sweep against AABB of the
    //               root BVH node.
    // @Optimization We should probably assume that the root node is always hit,
    //               tracing outside the world's bounds should never happen.
    float root_node_aabb_hit_fraction;
    bool is_root_hit = trace->HitsAabbOnFullSweep(root_node.mins, root_node.maxs,
        &root_node_aabb_hit_fraction);
    if (!is_root_hit)
        return;

    // A leaf or a node and its corresponding minimum collision time where the
    // trace hits the leaf's/node's AABB.
    struct TraversalCandidate {
        int32_t node_or_leaf_idx; // See Node struct for details
        float aabb_hit_fraction; // When trace hits this leaf's/node's AABB
    };
    std::stack<TraversalCandidate> traversal_candidates;

    TraversalCandidate root_candidate = {
        .node_or_leaf_idx = 0, // Root node idx
        .aabb_hit_fraction = root_node_aabb_hit_fraction
    };
    traversal_candidates.push(root_candidate);

    // Efficiently traverse the BVH tree
    while (!traversal_candidates.empty()) {
        TraversalCandidate candidate = traversal_candidates.top();
        traversal_candidates.pop();

        // Discard candidate if we already hit something before this candidate's
        // AABB gets hit.
        if (trace->results.fraction < candidate.aabb_hit_fraction)
            continue;

        // Traverse candidate
        if (candidate.node_or_leaf_idx < 0) { // If candidate is a leaf
            int32_t leaf_idx = -candidate.node_or_leaf_idx;
            const Leaf& leaf = leaves[leaf_idx];

            // @Optimization Make sure CDispCollTree code doesn't do the same
            //               AABB check that we already do.
            coll::Debugger::DebugStart_BroadPhaseLeafHit(leaf, leaf_idx);
            DoSweptTraceAgainstLeaf(trace, leaf, c_world);
            coll::Debugger::DebugFinish_BroadPhaseLeafHit();
        }
        else { // If candidate is a node
            const Node& parent_node = nodes[candidate.node_or_leaf_idx];

            // New candidate entries of children whose AABB is hit by the trace
            std::vector<TraversalCandidate> child_candidates;

            // Trace against AABBs of candidate's children
            for (int32_t child_idx : { parent_node.child_l, parent_node.child_r }) {
                Vector3 child_mins;
                Vector3 child_maxs;
                if (child_idx < 0) { // If child is a leaf
                    child_mins = leaves[-child_idx].mins;
                    child_maxs = leaves[-child_idx].maxs;
                }
                else { // If child is a node
                    child_mins = nodes[child_idx].mins;
                    child_maxs = nodes[child_idx].maxs;
                }

                // @Optimization Doing an intersection between the AABB that
                //               encloses the trace sweep and the AABB of BVH
                //               nodes/leaves is possibly cheaper than doing an
                //               accurate sweep against the AABB of BVH nodes/leaves.
                float child_aabb_hit_fraction;
                bool is_child_aabb_hit = trace->HitsAabbOnFullSweep(
                    child_mins, child_maxs, &child_aabb_hit_fraction);

                if (is_child_aabb_hit) {
                    child_candidates.push_back({
                        .node_or_leaf_idx = child_idx,
                        .aabb_hit_fraction = child_aabb_hit_fraction
                        });
                }
            }

            // The child with the smaller hit fraction is traversed before the other.
            // This enables us to potentially discard the child that's further
            // away at a later point in time.
            if (child_candidates.size() == 2) {
                if (child_candidates[0].aabb_hit_fraction <
                    child_candidates[1].aabb_hit_fraction) {
                    traversal_candidates.push(child_candidates[1]);
                    traversal_candidates.push(child_candidates[0]); // <- Closer child on top of the stack
                }
                else {
                    traversal_candidates.push(child_candidates[0]);
                    traversal_candidates.push(child_candidates[1]); // <- Closer child on top of the stack
                }
            }
            else if (child_candidates.size() == 1) { // One child did not get hit
                traversal_candidates.push(child_candidates[0]);
            }
        }
    }
}

bool BVH::DoesAabbIntersectAnyDisplacement(
    const Vector3& aabb_mins, const Vector3& aabb_maxs, CollidableWorld& c_world)
{
    if (!WasConstructedSuccessfully())
        return false;

    // Stack entries are indices into the nodes array
    std::stack<int32_t> nodes_to_traverse;
    nodes_to_traverse.push(0); // Root node idx

    // Efficiently traverse the BVH tree
    while (!nodes_to_traverse.empty()) {
        const Node& parent_node = nodes[nodes_to_traverse.top()];
        nodes_to_traverse.pop();

        // Test this node's children
        for (int32_t child_idx : { parent_node.child_r, parent_node.child_l }) {
            if (child_idx >= 0) { // If child is a node
                const Node& child_node = nodes[child_idx];

                // If child node contains displacements
                if (child_node.contained_leaf_types[Leaf::Type::Displacement] == true) {
                    bool is_child_node_aabb_hit = AabbIntersectsAabb(
                        aabb_mins, aabb_maxs, child_node.mins, child_node.maxs);
                    if (is_child_node_aabb_hit)
                        nodes_to_traverse.push(child_idx);
                }
            }
            else { // If child is a leaf
                const Leaf& leaf = leaves[-child_idx];

                if (leaf.type != Leaf::Type::Displacement)
                    continue; // Skip non-displacement leafs

                bool is_leaf_aabb_hit = AabbIntersectsAabb(
                    aabb_mins, aabb_maxs, leaf.mins, leaf.maxs);
                if (!is_leaf_aabb_hit)
                    continue; // Skip since leaf's AABB is not hit

                CDispCollTree& disp_coll =
                    (*c_world.pImpl->hull_disp_coll_trees)[leaf.disp_coll_idx];
                if (disp_coll.AABBTree_IntersectAABB(aabb_mins, aabb_maxs))
                    return true;
            }
        }
    }
    return false;
}

void BVH::GetAabbsContainingPoint(const Vector3& pt,
    std::vector<Vector3>* aabb_mins_list,
    std::vector<Vector3>* aabb_maxs_list)
{
    if (!WasConstructedSuccessfully())
        return;
    const Node& root_node = nodes[0];
    _GetAabbsContainingPoint_r(root_node, pt, aabb_mins_list, aabb_maxs_list);
}

bool BVH::IsPointInAabb(const Vector3& pt,
    const Vector3& mins, const Vector3& maxs)
{
    for (int axis = 0; axis < 3; axis++)
        if (pt[axis] < mins[axis] || pt[axis] > maxs[axis])
            return false;
    return true;
}

float BVH::CalcAabbSurfaceArea(const Vector3& mins, const Vector3& maxs)
{
    Vector3 lengths = maxs - mins;
    return 2.0f * (
        lengths.x() * lengths.z() +
        lengths.x() * lengths.y() +
        lengths.y() * lengths.z()
    ); // Surface area calculation of a rectangular cuboid
}

void BVH::CalcAabbOfBvhLeaves(std::span<const uint32_t> leaf_refs,
    Vector3* aabb_mins, Vector3* aabb_maxs) const
{
    assert(!leaf_refs.empty());
    Vector3 mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
    Vector3 maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
    for (uint32_t leaf_idx : leaf_refs) {
        const Leaf& leaf = leaves[leaf_idx];
        for (int axis = 0; axis < 3; axis++) {
            mins[axis] = Math::min(mins[axis], leaf.mins[axis]);
            maxs[axis] = Math::max(maxs[axis], leaf.maxs[axis]);
        }
    }
    if (aabb_mins) *aabb_mins = mins;
    if (aabb_maxs) *aabb_maxs = maxs;
}

uint64_t BVH::GetSweptLeafTraceCost(const Leaf& leaf, CollidableWorld& c_world)
{
    // Estimated computation time of tracing a leaf under the assumption that
    // the trace already hits that leaf's AABB.

    // MAKE SURE that the sum of estimated costs of an entire map's leaves never
    // overflows the type of this function's return value.
    // To represent the cost value, an integer type was chosen instead of a
    // floating-point type in order to ensure accuracy.
    // E.g.: Adding many small costs to a large cost sum can cause significant
    // inaccuracies when using floating-point types.

    // @Optimization Develop relative leaf trace cost heuristics and use them
    //               here. Do they reduce total trace time?
    // @Optimization On leaf trace cost heuristics: Is it beneficial to estimate
    //               average or worst case trace cost? What exactly do we
    //               optimize? Test both!
    switch (leaf.type) {
    case Leaf::Type::Brush:
        return c_world.GetSweptTraceCost_Brush       (leaf.brush_idx);
    case Leaf::Type::Displacement:
        return c_world.GetSweptTraceCost_Displacement(leaf.disp_coll_idx);
    case Leaf::Type::FuncBrush:
        return c_world.GetSweptTraceCost_FuncBrush   (leaf.funcbrush_idx);
    case Leaf::Type::StaticProp:
        return c_world.GetSweptTraceCost_StaticProp  (leaf.sprop_idx);
    case Leaf::Type::DynamicProp:
        return c_world.GetSweptTraceCost_DynamicProp (leaf.dprop_idx);
    default: // Unknown type
        assert(false && "Unknown Leaf type. Did you forget to add a switch case?");
        return 1;
    }
}

BVH::NodeSplitDetails BVH::DetermineBeneficialNodeSplit(const Node& node_to_split,
    std::span<uint32_t> leaf_refs_sorted_along_axis[3],
    CollidableWorld& c_world) const
{
    const size_t leaf_cnt = leaf_refs_sorted_along_axis[0].size();
    assert(leaf_cnt >= 2);

#if 0 //////// MEDIAN SPLIT METHOD
    int largest_axis = 0; // Determine axis with the largest AABB extent
    for (int current_axis = 1; current_axis < 3; current_axis++) {
        float current_extent = node_to_split.maxs[current_axis] - node_to_split.mins[current_axis];
        float largest_extent = node_to_split.maxs[largest_axis] - node_to_split.mins[largest_axis];
        if (current_extent > largest_extent)
            largest_axis = current_axis;
    }
    NodeSplitDetails split_details = {
        .axis = largest_axis,
        .elem_idx = leaf_cnt / 2, // Split on the middle element
    };
#endif

#if 1 //////// SURFACE AREA HEURISTIC (SAH) METHOD
    // Surface Area Heuristic (SAH) method:
    // Split parent node (P) into a left and right child node (L and R) in a way
    // that minimizes the cost function
    //
    //   c := COST(L) * (SA(L) / SA(P)) + COST(R) * (SA(R) / SA(P))
    //
    // where  COST(L/R) := (Worst or average case?) Cost of tracing all leafs in
    //                     the left/right child node
    // and    SA(P/L/R) := Surface area of AABB of parent/left child/right child
    //                     node
    // 
    // Justification: Given that a convex volume L is contained in another
    //                convex volume P, the conditional probability of a random
    //                ray passing through P also passing through L is a fraction
    //                of their surface areas:  p(L | P) = SA(L) / SA(P)
    //                See: https://en.wikipedia.org/wiki/Crofton_formula

    // @Optimization Most traces are done with the player's hull. This SAH
    //     method relies on the Crofton formula which applies to random rays
    //     passing through convex objects, not random hull sweeps. Therefor,
    //     during these SAH calculations, it might be beneficial to view the
    //     hull trace as a ray trace by bloating all AABBs by half the player's
    //     hull extents before calculating their surface areas.
    // @Optimization To reduce BVH creation time while slightly worsening BVH
    //     quality: Use binning/bucket SAH, with whatever bin size. Instead of
    //     computing SAH cost for a split at every leaf, compute SAH cost for a
    //     split at every bin. Allegedly a good performance/quality compromise.
    //     Another option is to perform SAH on only the largest axis, not all 3.

    float node_aabb_surface_area =
        CalcAabbSurfaceArea(node_to_split.mins, node_to_split.maxs);

    NodeSplitDetails split_details = {}; // Irrelevant init vals

    // Determine split with lowest cost across all 3 axes
    float cur_lowest_sah_cost = HUGE_VALF;

    uint64_t total_leaf_trace_cost = 0;
    for (uint32_t leaf_idx : leaf_refs_sorted_along_axis[0])
        total_leaf_trace_cost += GetSweptLeafTraceCost(leaves[leaf_idx], c_world);

    for (int axis = 0; axis < 3; axis++) {
        // Precompute AABB surface area of right child for every split position
        std::vector<float> r_child_aabb_surface_areas(leaf_cnt); // idx is element to split on
        Vector3 r_child_mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
        Vector3 r_child_maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
        // Go from right to left
        for (size_t split_pos = leaf_cnt - 1; split_pos > 0; split_pos--) {
            // Include leaf at split position in AABB of right child
            uint32_t idx = leaf_refs_sorted_along_axis[axis][split_pos];
            const Leaf& leaf_at_split_pos = leaves[idx];
            for (int i = 0; i < 3; i++) {
                r_child_mins[i] = Math::min(r_child_mins[i], leaf_at_split_pos.mins[i]);
                r_child_maxs[i] = Math::max(r_child_maxs[i], leaf_at_split_pos.maxs[i]);
            }
            // Calculate and store AABB surface area of right child at this point
            r_child_aabb_surface_areas[split_pos] =
                CalcAabbSurfaceArea(r_child_mins, r_child_maxs);
        }
        r_child_aabb_surface_areas[0] = node_aabb_surface_area;


        Vector3 l_child_mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
        Vector3 l_child_maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
        // Cost of tracing all leafs in left/right child
        uint64_t total_l_child_cost = 0;
        uint64_t total_r_child_cost = total_leaf_trace_cost;
        // Check cost of splitting for every split position
        // Go from left to right
        for (size_t split_pos = 0; true; split_pos++) {
            // Skip SAH at split_pos 0 because it's illegal and the left child's
            // AABB doesn't exist yet
            if (split_pos > 0) {
                float l_child_aabb_surface_area = CalcAabbSurfaceArea(l_child_mins, l_child_maxs);
                float r_child_aabb_surface_area = r_child_aabb_surface_areas[split_pos]; // Lookup

                // Likelihood of a trace hitting a child AABB, given that the
                // parent AABB was hit.
                float l_child_aabb_hit_likelihood = l_child_aabb_surface_area / node_aabb_surface_area;
                float r_child_aabb_hit_likelihood = r_child_aabb_surface_area / node_aabb_surface_area;

                // Calculate surface area heuristic (SAH) / cost for this split
                // position on this axis
                float sah_cost =
                    (float)total_l_child_cost * l_child_aabb_hit_likelihood +
                    (float)total_r_child_cost * r_child_aabb_hit_likelihood;

                if (sah_cost < cur_lowest_sah_cost) { // Remember best split
                    cur_lowest_sah_cost = sah_cost;
                    split_details = { .axis = axis, .elem_idx = split_pos };
                }

                // When all legal splits on this axis were checked
                if (split_pos == leaf_cnt - 1)
                    break;
            }

            // Move left-most leaf of right child over to the left child
            uint32_t moved_over_leaf_idx = leaf_refs_sorted_along_axis[axis][split_pos];
            const Leaf& moved_over_leaf = leaves[moved_over_leaf_idx];
            uint64_t moved_over_leaf_cost =
                GetSweptLeafTraceCost(moved_over_leaf, c_world);
            total_l_child_cost += moved_over_leaf_cost;
            total_r_child_cost -= moved_over_leaf_cost;

            // Include moved-over leaf in AABB of left child
            for (int i = 0; i < 3; i++) {
                l_child_mins[i] = Math::min(l_child_mins[i], moved_over_leaf.mins[i]);
                l_child_maxs[i] = Math::max(l_child_maxs[i], moved_over_leaf.maxs[i]);
            }
        }
    }
#endif

    assert(split_details.axis >= 0 && split_details.axis <= 2);
    assert(split_details.elem_idx > 0);        // Split must not leave  left child with no leaves
    assert(split_details.elem_idx < leaf_cnt); // Split must not leave right child with no leaves
    return split_details;
}

void BVH::DoSweptTraceAgainstLeaf(SweptTrace* trace, const Leaf& leaf,
    CollidableWorld& c_world) const
{
    ZoneScoped;

    switch (leaf.type) {
    case Leaf::Type::Brush:
        // @Optimization Is the AABB check before tracing against *every* brush bad?
        c_world.DoSweptTrace_Brush       (trace, leaf.brush_idx    ); break;
    case Leaf::Type::Displacement:
        c_world.DoSweptTrace_Displacement(trace, leaf.disp_coll_idx); break;
    case Leaf::Type::FuncBrush:
        c_world.DoSweptTrace_FuncBrush   (trace, leaf.funcbrush_idx); break;
    case Leaf::Type::StaticProp:
        c_world.DoSweptTrace_StaticProp  (trace, leaf.sprop_idx    ); break;
    case Leaf::Type::DynamicProp:
        c_world.DoSweptTrace_DynamicProp (trace, leaf.dprop_idx    ); break;
    default: // Unknown type
        assert(false && "Unknown Leaf type. Did you forget to add a switch case?");
        break;
    }
}

bool BVH::CreateLeaves(CollidableWorld& c_world)
{
    std::shared_ptr<const BspMap> bsp_map = c_world.pImpl->origin_bsp_map;

    // @Optimization Estimate leaf count to avoid reallocation
    leaves.clear();
    leaves.push_back({}); // Add a dummy leaf at index 0. Needed due to node indexing.

    auto test_f_1 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::SOLID);
    auto test_f_2 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::PLAYERCLIP);
    auto test_f_3 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::GRENADECLIP);
    auto test_f_4 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::LADDER);
    auto test_f_5 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::WATER);

    Debug{} << PRINT_PREFIX << "Beginning creation...";

    // @Optimization Collect leaf types in a different order?

    // Collect relevant brushes
    Debug{} << PRINT_PREFIX << "Collecting AABBs of brushes";
    // @Optimization Collect worldspawn model brush indices only once ever?
    for (size_t brush_idx : bsp_map->GetModelBrushIndices_worldspawn()) {
        const BspMap::Brush& brush = bsp_map->brushes[brush_idx];
        if (!brush.num_sides)
            continue;

        bool relevant = false;
        if (test_f_1.first && test_f_1.first(brush)) relevant = true;
        if (test_f_2.first && test_f_2.first(brush)) relevant = true;
        if (test_f_3.first && test_f_3.first(brush)) relevant = true;
        if (test_f_4.first && test_f_4.first(brush)) relevant = true;
        if (test_f_5.first && test_f_5.first(brush)) relevant = true;
        if (!relevant)
            continue;

        Vector3 mins, maxs;
        bool valid_aabb = bsp_map->GetBrushAABB(brush_idx, &mins, &maxs);
        if (!valid_aabb)
            continue;

        // Bloat AABB a little to account for collision calculation tolerances
        mins -= Vector3{ 1.0f, 1.0f, 1.0f };
        maxs += Vector3{ 1.0f, 1.0f, 1.0f };

        Leaf bvh_leaf{
            .mins = mins,
            .maxs = maxs,
            .type = Leaf::Type::Brush,
            .funcbrush_idx = (uint32_t)brush_idx,
        };
        leaves.push_back(bvh_leaf);
    }

    // Collect relevant func_brush entities
    Debug{} << PRINT_PREFIX << "Collecting AABBs of func_brush entities";
    // If func_brush uses collision caches: TODO Test if fb coll cache exists!
    for (size_t fb_idx = 0; fb_idx < bsp_map->entities_func_brush.size(); fb_idx++) {
        const BspMap::Ent_func_brush& func_brush = bsp_map->entities_func_brush[fb_idx];
        if (!func_brush.IsSolid())
            continue;

        Vector3 mins, maxs;
        bool valid_aabb = CalcAabb_FuncBrush(fb_idx, *bsp_map, &mins, &maxs);
        if (!valid_aabb)
            continue;

        // Bloat AABB a little to account for collision calculation tolerances
        mins -= Vector3{ 1.0f, 1.0f, 1.0f };
        maxs += Vector3{ 1.0f, 1.0f, 1.0f };

        Leaf bvh_leaf{
            .mins = mins,
            .maxs = maxs,
            .type = Leaf::Type::FuncBrush,
            .funcbrush_idx = (uint32_t)fb_idx,
        };
        leaves.push_back(bvh_leaf);
    }

    // Collect relevant displacements
    Debug{} << PRINT_PREFIX << "Collecting AABBs of displacements";
    // Test if displacement collision structures have been constructed
    if (c_world.pImpl->hull_disp_coll_trees == Corrade::Containers::NullOpt) {
        assert(false && "BVH creation FAILED: Displacement collision structures "
                        "are not created yet.");
        return false; // Leaf creation failed
    }
    const std::vector<CDispCollTree>& disp_coll_arr = *c_world.pImpl->hull_disp_coll_trees;
    for (size_t disp_coll_idx = 0; disp_coll_idx < disp_coll_arr.size(); disp_coll_idx++) {
        const CDispCollTree& disp_coll = disp_coll_arr[disp_coll_idx];
        Vector3 aabb_mins, aabb_maxs;
        disp_coll.GetBounds(aabb_mins, aabb_maxs); // Returns bloated AABB

        Leaf bvh_leaf{
            .mins = aabb_mins,
            .maxs = aabb_maxs,
            .type = Leaf::Type::Displacement,
            .disp_coll_idx = (uint32_t)disp_coll_idx,
        };
        leaves.push_back(bvh_leaf);
    }

    // Collect relevant static props
    // Test if static prop collision caches have been constructed
    if (c_world.pImpl->coll_caches_sprop == Corrade::Containers::NullOpt) {
        assert(false && "BVH creation FAILED: Static prop collision caches are "
                        "not created yet.");
        return false; // Leaf creation failed
    }
    const std::map<uint32_t, CollisionCache_XProp>& sprop_coll_caches =
        *c_world.pImpl->coll_caches_sprop;
    Debug{} << PRINT_PREFIX << "Collecting AABBs of static props";
    for (size_t sprop_idx = 0; sprop_idx < bsp_map->static_props.size(); sprop_idx++) {
        const BspMap::StaticProp& sprop = bsp_map->static_props[sprop_idx];
        if (!sprop.IsSolidWithVPhysics())
            continue;

        // Get collision cache of this static prop
        const auto& iter = sprop_coll_caches.find(sprop_idx);
        if (iter == sprop_coll_caches.end())
            continue; // Static prop has no collision cache, skip
        const CollisionCache_XProp& sprop_coll_cache = iter->second;

        // Get exact, non-bloated AABB of static prop
        Vector3 aabb_mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
        Vector3 aabb_maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
        for (const auto& sprop_section_aabb : sprop_coll_cache.section_aabbs) {
            for (int axis = 0; axis < 3; axis++) {
                aabb_mins[axis] = Math::min(aabb_mins[axis], sprop_section_aabb.mins[axis]);
                aabb_maxs[axis] = Math::max(aabb_maxs[axis], sprop_section_aabb.maxs[axis]);
            }
        }
        // Bloat AABB a little to account for collision calculation tolerances
        aabb_mins -= Vector3{ 1.0f, 1.0f, 1.0f };
        aabb_maxs += Vector3{ 1.0f, 1.0f, 1.0f };

        Leaf bvh_leaf{
            .mins = aabb_mins,
            .maxs = aabb_maxs,
            .type = Leaf::Type::StaticProp,
            .sprop_idx = (uint32_t)sprop_idx,
        };
        leaves.push_back(bvh_leaf);
    }

    // Collect relevant dynamic props
    // Test if dynamic prop collision caches have been constructed
    if (c_world.pImpl->coll_caches_dprop == Corrade::Containers::NullOpt) {
        assert(false && "BVH creation FAILED: Dynamic prop collision caches are"
                        " not created yet.");
        return false; // Leaf creation failed
    }
    const std::map<uint32_t, CollisionCache_XProp>& dprop_coll_caches =
        *c_world.pImpl->coll_caches_dprop;
    Debug{} << PRINT_PREFIX << "Collecting AABBs of dynamic props";
    for (size_t dprop_idx = 0;
         dprop_idx < bsp_map->relevant_dynamic_props.size();
         dprop_idx++)
    {
        const BspMap::Ent_prop_dynamic& dprop =
            bsp_map->relevant_dynamic_props[dprop_idx];

        // Get collision cache of this dynamic prop
        const auto& iter = dprop_coll_caches.find(dprop_idx);
        if (iter == dprop_coll_caches.end())
            continue; // Dynamic prop has no collision cache, skip
        const CollisionCache_XProp& dprop_coll_cache = iter->second;

        // Get exact, non-bloated AABB of dynamic prop
        Vector3 aabb_mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
        Vector3 aabb_maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
        for (const auto& dprop_section_aabb : dprop_coll_cache.section_aabbs) {
            for (int axis = 0; axis < 3; axis++) {
                aabb_mins[axis] = Math::min(aabb_mins[axis], dprop_section_aabb.mins[axis]);
                aabb_maxs[axis] = Math::max(aabb_maxs[axis], dprop_section_aabb.maxs[axis]);
            }
        }
        // Bloat AABB a little to account for collision calculation tolerances
        aabb_mins -= Vector3{ 1.0f, 1.0f, 1.0f };
        aabb_maxs += Vector3{ 1.0f, 1.0f, 1.0f };

        Leaf bvh_leaf{
            .mins = aabb_mins,
            .maxs = aabb_maxs,
            .type = Leaf::Type::DynamicProp,
            .dprop_idx = (uint32_t)dprop_idx,
        };
        leaves.push_back(bvh_leaf);
    }

    return true; // Leaf creation succeeded
}

void BVH::CreateNodeHierarchy(uint32_t start_node_idx,
    std::span<uint32_t> start_node_leaf_refs_sorted_along_axis[3],
    CollidableWorld& c_world)
{
    struct UnsplitNodeStackEntry {
        // Node in nodes array that needs to be split.
        // Its AABB is set, its children are yet to be determined.
        uint32_t unsplit_node_idx; // idx into nodes

        // Leafs assigned to this node, sorted along all 3 axes.
        std::span<uint32_t> leaf_refs_sorted_along_axis[3];
    };
    // Keep track of nodes to split
    std::stack<UnsplitNodeStackEntry> unsplit_node_stack;

    UnsplitNodeStackEntry first_entry{
        .unsplit_node_idx = start_node_idx,
        .leaf_refs_sorted_along_axis = {
            start_node_leaf_refs_sorted_along_axis[0],
            start_node_leaf_refs_sorted_along_axis[1],
            start_node_leaf_refs_sorted_along_axis[2]
        }
    };
    unsplit_node_stack.push(first_entry);

    while (!unsplit_node_stack.empty()) {
        UnsplitNodeStackEntry next = unsplit_node_stack.top();
        unsplit_node_stack.pop();

        Node& current_node = nodes[next.unsplit_node_idx];
        std::span<uint32_t> leaf_refs_sorted_along_axis[3] = {
            next.leaf_refs_sorted_along_axis[0],
            next.leaf_refs_sorted_along_axis[1],
            next.leaf_refs_sorted_along_axis[2]
        };

        const size_t current_node_leaf_cnt = leaf_refs_sorted_along_axis[0].size();
        assert(current_node_leaf_cnt >= 2);

        // ---- Split this node ----

        NodeSplitDetails split_details = DetermineBeneficialNodeSplit(
            current_node, leaf_refs_sorted_along_axis, c_world);

        // Select axis to split on
        int split_axis = split_details.axis;
        // On axis to split on, select element to split on
        // All entries with (idx < split_pos) go to left child
        // All entries with (idx >= split_pos) go to right child
        size_t split_pos = split_details.elem_idx;

        const size_t l_child_leaf_cnt = split_pos;
        const size_t r_child_leaf_cnt = current_node_leaf_cnt - l_child_leaf_cnt;

        // Compute leaf ref separations between the two children
        std::span<uint32_t> l_child_leaf_refs_sorted_along_axis[3] = { {}, {}, {} };
        std::span<uint32_t> r_child_leaf_refs_sorted_along_axis[3] = { {}, {}, {} };

        // On split axis, just separate the range into two subranges, already sorted.
        l_child_leaf_refs_sorted_along_axis[split_axis] = {
            leaf_refs_sorted_along_axis[split_axis].begin(),
            l_child_leaf_cnt
        };
        r_child_leaf_refs_sorted_along_axis[split_axis] = {
            leaf_refs_sorted_along_axis[split_axis].begin() + l_child_leaf_cnt,
            r_child_leaf_cnt
        };

        // Create LUT to quickly figure out which side a leaf is on.
        const bool L_CHILD = true;
        const bool R_CHILD = false;
        // @Optimization Only allocate this once? Use BitArray?
        std::vector<bool> leaf_lut(leaves.size(), R_CHILD);
        for (uint32_t leaf_idx : l_child_leaf_refs_sorted_along_axis[split_axis])
            leaf_lut[leaf_idx] = L_CHILD;

        // On non-split axes, copy leaf refs and first copy back l_child leaves,
        // then r_child leaves, while keeping the sorted order within both children.
        std::vector<uint32_t> orig_sorted_leaf_refs;
        orig_sorted_leaf_refs.reserve(current_node_leaf_cnt);
        for (int axis = 0; axis < 3; axis++) {
            if (axis == split_axis) continue;
            // Copy all leaf refs to buffer
            orig_sorted_leaf_refs.clear();
            for (uint32_t leaf_idx : leaf_refs_sorted_along_axis[axis])
                orig_sorted_leaf_refs.push_back(leaf_idx);
            // Copy back while keeping sorted order, but separating l_child and
            // r_child leafs
            size_t l_child_pos = 0;
            size_t r_child_pos = l_child_leaf_cnt;
            for (uint32_t leaf_idx : orig_sorted_leaf_refs) {
                if (leaf_lut[leaf_idx] == L_CHILD)
                    leaf_refs_sorted_along_axis[axis][l_child_pos++] = leaf_idx;
                else // leaf_lut[leaf_idx] == R_CHILD
                    leaf_refs_sorted_along_axis[axis][r_child_pos++] = leaf_idx;
            }
            // Split separated leaf refs array into l_child_leaf_refs and
            // r_child_leaf_refs
            l_child_leaf_refs_sorted_along_axis[axis] = {
                leaf_refs_sorted_along_axis[axis].begin(),
                l_child_leaf_cnt
            };
            r_child_leaf_refs_sorted_along_axis[axis] = {
                leaf_refs_sorted_along_axis[axis].begin() + l_child_leaf_cnt,
                r_child_leaf_cnt
            };
        }

        assert(l_child_leaf_cnt > 0);
        if (l_child_leaf_cnt == 1) { // Make left child a leaf 
            uint32_t l_child_leaf_idx = l_child_leaf_refs_sorted_along_axis[0][0];
            current_node.child_l = -((int32_t)l_child_leaf_idx);
        }
        else if (l_child_leaf_cnt >= 2) { // Make left child a node
            uint32_t l_child_node_idx = nodes.size();
            nodes.push_back({});
            Node& l_child_node = nodes.back();
            current_node.child_l = l_child_node_idx;
            // Compute AABB of l_child node
            // @Optimization DetermineBeneficialNodeSplit() already calculated
            //               AABBs of children, no need to calculate again.
            CalcAabbOfBvhLeaves(l_child_leaf_refs_sorted_along_axis[0],
                &l_child_node.mins, &l_child_node.maxs);

            UnsplitNodeStackEntry new_entry{
                .unsplit_node_idx = l_child_node_idx,
                .leaf_refs_sorted_along_axis = {
                    l_child_leaf_refs_sorted_along_axis[0],
                    l_child_leaf_refs_sorted_along_axis[1],
                    l_child_leaf_refs_sorted_along_axis[2]
                }
            };
            unsplit_node_stack.push(new_entry);
        }

        assert(r_child_leaf_cnt > 0);
        if (r_child_leaf_cnt == 1) { // Make right child a leaf 
            uint32_t r_child_leaf_idx = r_child_leaf_refs_sorted_along_axis[0][0];
            current_node.child_r = -((int32_t)r_child_leaf_idx);
        }
        else if (r_child_leaf_cnt >= 2) { // Make right child a node
            uint32_t r_child_node_idx = nodes.size();
            nodes.push_back({});
            Node& r_child_node = nodes.back();
            current_node.child_r = r_child_node_idx;
            // Compute AABB of r_child node
            // @Optimization DetermineBeneficialNodeSplit() already calculated
            //               AABBs of children, no need to calculate again.
            CalcAabbOfBvhLeaves(r_child_leaf_refs_sorted_along_axis[0],
                &r_child_node.mins, &r_child_node.maxs);

            UnsplitNodeStackEntry new_entry{
                .unsplit_node_idx = r_child_node_idx,
                .leaf_refs_sorted_along_axis = {
                    r_child_leaf_refs_sorted_along_axis[0],
                    r_child_leaf_refs_sorted_along_axis[1],
                    r_child_leaf_refs_sorted_along_axis[2]
                }
            };
            unsplit_node_stack.push(new_entry);
        }
    }
}

void BVH::SetNodeContentsInfo()
{
    if (!WasConstructedSuccessfully()) {
        assert(0);
        return;
    }

    // Index is the node index
    std::vector<bool> are_contents_of_node_set(nodes.size(), false);

    size_t node_contents_set_cnt = 0;
    while (node_contents_set_cnt < nodes.size()) {
        // At the time of writing (2023-09-11) the nodes array was ordered in a
        // breadth-first manner (root node comes first, nodes close to leaves
        // come last).
        // Therefor, iterating the nodes array from back to front is faster here
        // than front to back, since contents info is set starting at the nodes
        // close to leaves.
        for (int64_t i = nodes.size() - 1; i >= 0; i--) {
            if (are_contents_of_node_set[i])
                continue;

            Node& node = nodes[i];

            // Don't set contents yet if a child node's contents are not set yet
            if (node.child_l >= 0 && !are_contents_of_node_set[node.child_l])
                continue;
            if (node.child_r >= 0 && !are_contents_of_node_set[node.child_r])
                continue;

            // Set contents of current node using contents of its children
            for (int32_t child_idx : { node.child_l, node.child_r }) {
                if (child_idx < 0) {
                    const Leaf& leaf = leaves[-child_idx];
                    node.contained_leaf_types.set(leaf.type);
                }
                else {
                    const Node& child_node = nodes[child_idx];
                    node.contained_leaf_types |= child_node.contained_leaf_types;
                }
            }

            are_contents_of_node_set[i] = true;
            node_contents_set_cnt++;
        }
    }
}

void BVH::_GetAabbsContainingPoint_r(const Node& node, const Vector3& pt,
    std::vector<Vector3>* aabb_mins_list,
    std::vector<Vector3>* aabb_maxs_list)
{
    if (!IsPointInAabb(pt, node.mins, node.maxs))
        return;
    if (aabb_mins_list) aabb_mins_list->push_back(node.mins);
    if (aabb_maxs_list) aabb_maxs_list->push_back(node.maxs);

    int32_t l_idx = node.child_l;
    int32_t r_idx = node.child_r;

    if (l_idx < 0) {
        const Leaf& leaf = leaves[-l_idx];
        if (IsPointInAabb(pt, leaf.mins, leaf.maxs)) {
            if (aabb_mins_list) aabb_mins_list->push_back(leaf.mins);
            if (aabb_maxs_list) aabb_maxs_list->push_back(leaf.maxs);
        }
    }
    if (r_idx < 0) {
        const Leaf& leaf = leaves[-r_idx];
        if (IsPointInAabb(pt, leaf.mins, leaf.maxs)) {
            if (aabb_mins_list) aabb_mins_list->push_back(leaf.mins);
            if (aabb_maxs_list) aabb_maxs_list->push_back(leaf.maxs);
        }
    }

    if (l_idx >= 0) _GetAabbsContainingPoint_r(nodes[l_idx], pt, aabb_mins_list, aabb_maxs_list);
    if (r_idx >= 0) _GetAabbsContainingPoint_r(nodes[r_idx], pt, aabb_mins_list, aabb_maxs_list);
}
