#ifndef COLL_BVH_H_
#define COLL_BVH_H_

#include <cstdint>
#include <span>
#include <vector>

#include <Magnum/Math/BitVector.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld.h"
#include "csgo_parsing/BspMap.h"

namespace coll {

class BVH {
public:
    // Construct BVH of CollidableWorld. It must contain at least 2 collidable
    // objects.
    // CAUTION: BVH must only be created after all other collision data in
    //          CollidableWorld was created!
    BVH(CollidableWorld& c_world);

    // Check whether an error occurred during BVH construction.
    // If construction failed, traces cannot be performed.
    bool WasConstructedSuccessfully();

    // Does nothing if WasConstructedSuccessfully() returns false.
    // CAUTION: Not thread-safe yet!
    void DoSweptTrace(SweptTrace* trace, CollidableWorld& c_world);

    // Returns false if WasConstructedSuccessfully() returns false.
    // Only displacements that don't have the NO_HULL_COLL flag are considered.
    bool DoesAabbIntersectAnyDisplacement(
        const Magnum::Vector3& aabb_mins,
        const Magnum::Vector3& aabb_maxs,
        CollidableWorld& c_world);

    // Debug function. Does nothing if WasConstructedSuccessfully() returns false.
    void GetAabbsContainingPoint(const Magnum::Vector3& pt,
        std::vector<Magnum::Vector3>* aabb_mins_list,
        std::vector<Magnum::Vector3>* aabb_maxs_list);

private:
    template<std::size_t size> using BitVector = Magnum::Math::BitVector<size>;

    struct Leaf {
        // AABB of referenced map object, but slightly bloated.
        Magnum::Vector3 mins;
        Magnum::Vector3 maxs;

        // When adding more types, make sure to add cases to switch statements
        // that check these types. COUNT must remain the last enum entry.
        enum Type {
            Brush,
            Displacement,
            StaticProp,
            DynamicProp,
            FuncBrush,
            COUNT
        } type;

        // Index of referenced map object
        union {
            uint32_t     brush_idx; // if type == Brush:        idx into BspMap.brushes
            uint32_t disp_coll_idx; // if type == Displacement: idx into CDispCollTree array
            uint32_t funcbrush_idx; // if type == FuncBrush:    idx into BspMap.entities_func_brush
            uint32_t     sprop_idx; // if type == StaticProp:   idx into BspMap.static_props
            uint32_t     dprop_idx; // if type == DynamicProp:  idx into BspMap.relevant_dynamic_props
        };
    };

    struct Node {
        // AABB encompassing both children's AABB.
        Magnum::Vector3 mins;
        Magnum::Vector3 maxs;

        // Child indices:
        //   Index into nodes  if (idx >= 0).  =>  nodes[idx]
        //   Index into leaves if (idx < 0).   =>  leaves[-idx]
        int32_t child_l;
        int32_t child_r;

        // Flags indicating which types of leafs are contained in this node.
        // A leaf type's enum value signifies its bit position in this BitVector.
        BitVector<Leaf::Type::COUNT> contained_leaf_types{ Magnum::Math::ZeroInit };

        // @Optimization To speed up BVH traversal, we could add further node
        //               content information to this structure such as OR-ed
        //               'contents' flags of contained brushes.
    };

    std::vector<Leaf> leaves; // Has a dummy leaf at index 0
    std::vector<Node> nodes;
    size_t total_leaf_cnt; // Not counting dummy leaf, equal to (leaves.size()-1)

private:
    static bool IsPointInAabb(const Magnum::Vector3& pt,
        const Magnum::Vector3& mins, const Magnum::Vector3& maxs);

    static float CalcAabbSurfaceArea(
        const Magnum::Vector3& mins, const Magnum::Vector3& maxs);

    // leaf_refs is an array of indices into leaves. At least one leaf must be given.
    void CalcAabbOfBvhLeaves(std::span<const uint32_t> leaf_refs,
        Magnum::Vector3* aabb_mins, Magnum::Vector3* aabb_maxs) const;

    static uint64_t GetSweptLeafTraceCost(const Leaf& leaf, CollidableWorld& c_world);

    struct NodeSplitDetails {
        // Axis to split node on. 0 -> X axis, 1 -> Y axis, 2 -> Z axis
        int axis;

        // On axis to split on, index of element to split on.
        // All elements with (idx < elem_idx) go to left child
        // All elements with (idx >= elem_idx) go to right child
        size_t elem_idx;
    };

    // At least 2 leafs must be given for the node that gets split.
    // Splits are always determined in a way that ensures that the resulting
    // children have at least one leaf.
    NodeSplitDetails DetermineBeneficialNodeSplit(const Node& node_to_split,
        std::span<uint32_t> leaf_refs_sorted_along_axis[3],
        CollidableWorld& c_world) const;

    void DoSweptTraceAgainstLeaf(SweptTrace* trace, const Leaf& leaf,
        CollidableWorld& c_world) const;

    // Fills leaves array with one dummy leaf and further leafs.
    // Returns false if leaf creation failed, true otherwise.
    bool CreateLeaves(CollidableWorld& c_world);

    // start_node_idx is an index into nodes.
    // Given start node has its AABB set. Start node must have at least 2 leaves.
    void CreateNodeHierarchy(uint32_t start_node_idx,
        std::span<uint32_t> start_node_leaf_refs_sorted_along_axis[3],
        CollidableWorld& c_world);

    // This function assumes that all leaves and nodes have been created and
    // stored in the nodes and leaves arrays.
    void SetNodeContentsInfo();

    void _GetAabbsContainingPoint_r(const Node& node, const Magnum::Vector3& pt,
        std::vector<Magnum::Vector3>* aabb_mins_list,
        std::vector<Magnum::Vector3>* aabb_maxs_list);

private:
    // Debugger needs to debug, let it access private members.
    friend class Debugger;
    // Benchmarks needs to benchmark, let them access private members.
    friend class Benchmark;
};
    
} // namespace coll

#endif // COLL_BVH_H_
