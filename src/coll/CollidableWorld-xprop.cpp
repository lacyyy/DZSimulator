#include "coll/CollidableWorld-xprop.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include <span>
#include <vector>

#include <Tracy.hpp>

#include <Corrade/Containers/Optional.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld.h"
#include "coll/CollidableWorld_Impl.h"
#include "coll/SweptTrace.h"
#include "csgo_parsing/BspMap.h"
#include "utils_3d.h"

using namespace Corrade;
using namespace Magnum;
using namespace coll;
using namespace csgo_parsing;
using namespace utils_3d;
using Plane = BspMap::Plane;


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/utils/vbsp/map.cpp)
#define RENDER_NORMAL_EPSILON 0.00001f

static bool SnapVector(Vector3& normal) {
    for (int i = 0; i < 3; i++) {
        if (std::fabs(normal[i] - 1.0f) < RENDER_NORMAL_EPSILON) {
            normal = { 0.0f, 0.0f, 0.0f };
            normal[i] = 1.0f;
            return true;
        }

        if (std::fabs(normal[i] - -1.0f) < RENDER_NORMAL_EPSILON) {
            normal = { 0.0f, 0.0f, 0.0f };
            normal[i] = -1.0f;
            return true;
        }
    }
    return false;
}

static bool PlaneEqual(const Plane& p, const Vector3& normal, float dist,
    float normalEpsilon, float distEpsilon)
{
    if (std::fabs(p.normal[0] - normal[0]) < normalEpsilon &&
        std::fabs(p.normal[1] - normal[1]) < normalEpsilon &&
        std::fabs(p.normal[2] - normal[2]) < normalEpsilon &&
        std::fabs(p.dist - dist) < distEpsilon)
        return true;
    return false;
}
// --------- end of source-sdk-2013 code ---------


////////////////////////////////////////////////////////////////////////////////


uint64_t CollidableWorld::GetSweptTraceCost_StaticProp(uint32_t sprop_idx)
{
    // See BVH::GetSweptLeafTraceCost() for details and considerations.
    return 1; // Is sprop trace cost dependent on triangle count?
}

uint64_t CollidableWorld::GetSweptTraceCost_DynamicProp(uint32_t dprop_idx)
{
    // See BVH::GetSweptLeafTraceCost() for details and considerations.
    return 1; // Is dprop trace cost dependent on triangle count?
}


////////////////////////////////////////////////////////////////////////////////


Containers::Optional<CollisionCache_XProp>
Create_CollisionCache_XProp(const CollisionModel& cmodel,
                            const Vector3& xprop_origin,
                            const Vector3& xprop_angles,
                            float          xprop_uniform_scale);

Containers::Optional<CollisionCache_XProp>
coll::Create_CollisionCache_StaticProp(const BspMap::StaticProp& sprop,
                                       const CollisionModel& cmodel)
{
    return Create_CollisionCache_XProp(
        cmodel, sprop.origin, sprop.angles, sprop.uniform_scale);
}

Corrade::Containers::Optional<CollisionCache_XProp>
coll::Create_CollisionCache_DynamicProp(const BspMap::Ent_prop_dynamic& dprop,
                                        const CollisionModel& cmodel)
{
    return Create_CollisionCache_XProp(cmodel, dprop.origin, dprop.angles, 1.0f);
}


Containers::Optional<CollisionCache_XProp>
Create_CollisionCache_XProp(const CollisionModel& cmodel,
                            const Vector3& xprop_origin,
                            const Vector3& xprop_angles,
                            float          xprop_uniform_scale)
{
    ZoneScoped;
    const size_t NUM_SECTIONS = cmodel.section_tri_meshes.size();

    float inv_scale = 1.0f / xprop_uniform_scale;

    Matrix4 xprop_transf = CalcModelTransformationMatrix(
        xprop_origin, xprop_angles, xprop_uniform_scale);
    Matrix3 rotationscaling = xprop_transf.rotationScaling();

    Quaternion rotation = CalcQuaternion(xprop_angles).normalized();
    Quaternion inv_rotation = rotation.invertedNormalized();

    std::vector<CollisionCache_XProp::AABB> section_aabbs;
    section_aabbs.reserve(NUM_SECTIONS);

    // Get AABBs: Apply xprop transformation to every vertex of each section
    bool no_vertex_found = true;
    for (size_t section_idx = 0; section_idx < NUM_SECTIONS; section_idx++) {
        Vector3 aabb_mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
        Vector3 aabb_maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
        for (const Vector3& vert : cmodel.section_tri_meshes[section_idx].vertices) {
            no_vertex_found = false;
            Vector3 transformed_v = xprop_transf.transformPoint(vert);

            // Add transformed vertex to section's AABB
            for (int axis = 0; axis < 3; axis++) {
                aabb_mins[axis] = Math::min(aabb_mins[axis], transformed_v[axis]);
                aabb_maxs[axis] = Math::max(aabb_maxs[axis], transformed_v[axis]);
            }
        }
        section_aabbs.push_back({ .mins = aabb_mins, .maxs = aabb_maxs });
    }
    if (no_vertex_found) { // Invalid collision model
        assert(false && "Create_CollisionCache_XProp(): Invalid collision model");
        return Containers::NullOpt;
    }

    // Create bevel plane LUT of each section
    std::vector<XPropSectionBevelPlaneLut> section_bevel_luts;
    section_bevel_luts.reserve(NUM_SECTIONS);
    for (size_t section_idx = 0; section_idx < NUM_SECTIONS; section_idx++) {
        // Create LUT of section
        section_bevel_luts.emplace_back(rotationscaling, inv_rotation, inv_scale,
            cmodel.section_tri_meshes[section_idx],
            cmodel.section_planes[section_idx]
        );
    }

    return CollisionCache_XProp{
        .inv_rotation = inv_rotation,
        .inv_scale    = inv_scale,
        .section_aabbs      = std::move(section_aabbs),
        .section_bevel_luts = std::move(section_bevel_luts)
    };
}


////////////////////////////////////////////////////////////////////////////////


void DoSweptTrace_XProp(SweptTrace* trace,
                        const Vector3&       xprop_origin,
                        CollisionModel       xprop_collmodel,
                        CollisionCache_XProp xprop_collcache);

void CollidableWorld::DoSweptTrace_StaticProp(SweptTrace* trace, uint32_t sprop_idx)
{
    const BspMap::StaticProp& sprop = pImpl->origin_bsp_map->static_props[sprop_idx];
    const std::string&     mdl_path = pImpl->origin_bsp_map->static_prop_model_dict[sprop.model_idx];
    if (!sprop.IsSolidWithVPhysics()) return; // Skip this static prop

    // Ensure that the required collision model and cache have been created
    assert(pImpl->xprop_coll_models != Corrade::Containers::NullOpt);
    assert(pImpl->coll_caches_sprop != Corrade::Containers::NullOpt);

    // Get collision model
    const auto& collmodel_iter = pImpl->xprop_coll_models->find(mdl_path);
    if (collmodel_iter == pImpl->xprop_coll_models->end())
        return; // This static prop has no collision model, skip
    const CollisionModel& collmodel = collmodel_iter->second;

    // Get collision cache
    const auto& collcache_iter = pImpl->coll_caches_sprop->find(sprop_idx);
    if (collcache_iter == pImpl->coll_caches_sprop->end()) {
        assert(false); // Shouldn't happen
        return; // This static prop has no collision cache, skip
    }
    const CollisionCache_XProp& collcache = collcache_iter->second;

    // Do trace
    DoSweptTrace_XProp(trace, sprop.origin, collmodel, collcache);
}

void CollidableWorld::DoSweptTrace_DynamicProp(SweptTrace* trace, uint32_t dprop_idx)
{
    const BspMap::Ent_prop_dynamic& dprop =
        pImpl->origin_bsp_map->relevant_dynamic_props[dprop_idx];

    // Ensure that the required collision model and cache have been created
    assert(pImpl->xprop_coll_models != Corrade::Containers::NullOpt);
    assert(pImpl->coll_caches_dprop != Corrade::Containers::NullOpt);

    // Get collision model
    const auto& collmodel_iter = pImpl->xprop_coll_models->find(dprop.model);
    if (collmodel_iter == pImpl->xprop_coll_models->end())
        return; // This dynamic prop has no collision model, skip
    const CollisionModel& collmodel = collmodel_iter->second;

    // Get collision cache
    const auto& collcache_iter = pImpl->coll_caches_dprop->find(dprop_idx);
    if (collcache_iter == pImpl->coll_caches_dprop->end()) {
        assert(false); // Shouldn't happen
        return; // This dynamic prop has no collision cache, skip
    }
    const CollisionCache_XProp& collcache = collcache_iter->second;

    // Do trace
    DoSweptTrace_XProp(trace, dprop.origin, collmodel, collcache);
}

void DoSweptTrace_XProp(SweptTrace* trace,
                        const Vector3&       xprop_origin,
                        CollisionModel       xprop_collmodel,
                        CollisionCache_XProp xprop_collcache)
{
    const size_t NUM_SECTIONS = xprop_collmodel.section_tri_meshes.size();

    // TODO: Look at https://doc.magnum.graphics/magnum/transformations.html to
    //       possibly improve/optimize the transformations in this method.

    // Transform trace's start, dir and extents into the coordinate system of
    // the unscaled, unrotated and untranslated collision model of the prop
    // (static or dynamic). Then perform trace calculations there.
    // Essentially, apply the reverse of the xprop's transformation to the trace.
    // TODO can probably combine all reverse operations in a single matrix4 mult

    Vector3 transformed_trace_start = trace->info.startpos;
    Vector3 transformed_trace_dir   = trace->info.delta;
    Vector3 transformed_extents     = trace->info.extents;

    // Orthogonal basis vectors that can describe any vector in a rotated
    // coordinate system
    Vector3 unit_vec_0 = { 1.0f, 0.0f, 0.0f };
    Vector3 unit_vec_1 = { 0.0f, 1.0f, 0.0f };
    Vector3 unit_vec_2 = { 0.0f, 0.0f, 1.0f };
    // @Optimization Can we multiply the trace's extents into the unit vecs?

    // Reverse xprop translation (opposite of CalcModelTransformationMatrix())
    transformed_trace_start -= xprop_origin;
    // Reverse xprop rotation (opposite of CalcModelTransformationMatrix())
    const auto& inv_rot = xprop_collcache.inv_rotation;
    transformed_trace_start = inv_rot.transformVectorNormalized(transformed_trace_start);
    transformed_trace_dir   = inv_rot.transformVectorNormalized(transformed_trace_dir);
    unit_vec_0              = inv_rot.transformVectorNormalized(unit_vec_0);
    unit_vec_1              = inv_rot.transformVectorNormalized(unit_vec_1);
    unit_vec_2              = inv_rot.transformVectorNormalized(unit_vec_2);
    // Reverse xprop scaling (opposite of CalcModelTransformationMatrix())
    transformed_trace_start *= xprop_collcache.inv_scale;
    transformed_trace_dir   *= xprop_collcache.inv_scale;
    transformed_extents     *= xprop_collcache.inv_scale;


    enum PlaneCategory {
        MODEL_TRIANGLES,     // Planes of every triangle
        EDGE_BEVELS,         // Bevel planes of some triangle edges
        AABB_TRANSFORMED,    // AABB planes of the transformed section
        AABB_NON_TRANSFORMED // AABB planes of the non-transformed section
    };

    // What plane categories are processed for ray traces, and in what order
    static constexpr std::array<PlaneCategory, 1> RAY_TRACE_CAT_LIST = {
        MODEL_TRIANGLES
    };
    // What plane categories are processed for hull traces, and in what order
    static constexpr std::array<PlaneCategory, 4> HULL_TRACE_CAT_LIST = {
        // @Optimization Changing this order might improve performance
        AABB_NON_TRANSFORMED, AABB_TRANSFORMED, EDGE_BEVELS, MODEL_TRIANGLES
    };

    std::span<const PlaneCategory> plane_categories = trace->info.isray ?
        std::span<const PlaneCategory>(RAY_TRACE_CAT_LIST.begin(), RAY_TRACE_CAT_LIST.size())
        :
        std::span<const PlaneCategory>(HULL_TRACE_CAT_LIST.begin(), HULL_TRACE_CAT_LIST.size());

    // ======== Go through each section independently ========
    // @Optimization Process sections front to back? Skip section if AABB hit
    //               time is past trace's fraction?
    for (size_t section_idx = 0; section_idx < NUM_SECTIONS; section_idx++)
    {
        const Vector3& xprop_section_mins = xprop_collcache.section_aabbs[section_idx].mins;
        const Vector3& xprop_section_maxs = xprop_collcache.section_aabbs[section_idx].maxs;
        // Bloat AABB a little to account for collision calculation tolerances
        Vector3 bloated_xprop_section_mins = xprop_section_mins - Vector3{1.0f, 1.0f, 1.0f };
        Vector3 bloated_xprop_section_maxs = xprop_section_maxs + Vector3{1.0f, 1.0f, 1.0f };
        // Early-out if trace doesn't hit section's bloated AABB
        // @Optimization If the xprop only has 1 section, isn't this check
        //               redundant, as it's already done by BVH code?
        if (!IsAabbHitByFullSweptTrace(trace->info.startpos,
                                       trace->info.invdelta,
                                       trace->info.extents,
                                       bloated_xprop_section_mins,
                                       bloated_xprop_section_maxs))
            continue;

        const std::vector<Plane>& tri_planes_of_section =
            xprop_collmodel.section_planes[section_idx];

        XPropSectionBevelPlaneGenerator bevel_gen(
            xprop_collmodel, xprop_collcache, section_idx);

        // -------- start of source-sdk-2013 code --------
        // (taken and modified from source-sdk-2013/<...>/src/utils/vrad/trace.cpp)

        // TODO Move these constants' definitions and all their other
        //      occurrences somewhere else, maybe into CollidableWorld?
        const float DIST_EPSILON = 0.03125f; // 1/32 epsilon to keep floating point happy
        const float NEVER_UPDATED = -9999;

        const Vector3 start = transformed_trace_start;
        const Vector3 end   = transformed_trace_start + transformed_trace_dir;

        //const BrushSide* leadside = nullptr;
        Plane clipplane;
        float enterfrac = NEVER_UPDATED;
        float leavefrac = 1.0f;
        bool  getout   = false;
        bool  startout = false;

        float   dist;
        Vector3 ofs;
        float   d1, d2;
        float   f;

        // ======== Go through each plane category in order ========
        Plane next_plane;
        bool skip_section = false;
        for (PlaneCategory cur_plane_cat : plane_categories)
        {
            for (size_t plane_idx = 0; true; plane_idx++)
            {
                // ====== Get next plane to process in current category  ======
                if (cur_plane_cat == PlaneCategory::MODEL_TRIANGLES)
                {
                    // Get a triangle's plane
                    if (plane_idx < tri_planes_of_section.size())
                        next_plane = tri_planes_of_section[plane_idx];
                    else
                        break; // Exit this category
                }
                else if (cur_plane_cat == PlaneCategory::EDGE_BEVELS)
                {
                    // @Optimization Note: Precomputing and storing all bevel
                    //                     planes for each prop would require up
                    //                     to a couple megabytes for a CSGO map.
                    // Note: This bevel plane generation for props doesn't lead
                    //       to hull trace results exactly matching those of
                    //       CSGO, but it should be good enough.
                    bool success = bevel_gen.GetNext(&next_plane);
                    if (!success)
                        break; // Exit this category
                }
                else if (cur_plane_cat == PlaneCategory::AABB_TRANSFORMED)
                {
                    // AABB planes of transformed section, transformed back
                    // @Optimization Should these planes be checked last?
                    if (plane_idx > 5) break; // Exit this category
                    switch(plane_idx) {
                        case 0: next_plane = { +unit_vec_0,  (xprop_section_maxs[0] - xprop_origin[0]) * xprop_collcache.inv_scale }; break;
                        case 1: next_plane = { -unit_vec_0, -(xprop_section_mins[0] - xprop_origin[0]) * xprop_collcache.inv_scale }; break;
                        case 2: next_plane = { +unit_vec_1,  (xprop_section_maxs[1] - xprop_origin[1]) * xprop_collcache.inv_scale }; break;
                        case 3: next_plane = { -unit_vec_1, -(xprop_section_mins[1] - xprop_origin[1]) * xprop_collcache.inv_scale }; break;
                        case 4: next_plane = { +unit_vec_2,  (xprop_section_maxs[2] - xprop_origin[2]) * xprop_collcache.inv_scale }; break;
                        case 5: next_plane = { -unit_vec_2, -(xprop_section_mins[2] - xprop_origin[2]) * xprop_collcache.inv_scale }; break;
                    }
                }
                else if (cur_plane_cat == PlaneCategory::AABB_NON_TRANSFORMED)
                {
                    // AABB of non-transformed section (i.e. section from
                    // unrotated, unscaled and untranslated base collision model)
                    // @Optimization Testing showed that these 6 extra planes
                    //               had little effect on getting trace results
                    //               more similar to CSGO. These 6 extra planes
                    //               can probably be removed.
                    //               Update 2024-01-31: Trace results do change
                    //               a bit with these removed. Keeping them for
                    //               now. I'm not sure how thorough those earlier
                    //               comparisons with CSGO trace results were.
                    const Vector3& non_transf_aabb_mins = xprop_collmodel.section_aabbs[section_idx].mins;
                    const Vector3& non_transf_aabb_maxs = xprop_collmodel.section_aabbs[section_idx].maxs;
                    if (plane_idx > 5) break; // Exit this category
                    switch(plane_idx) {
                        case 0: next_plane = { Vector3{ +1.0f,  0.0f,  0.0f },  non_transf_aabb_maxs[0] }; break;
                        case 1: next_plane = { Vector3{ -1.0f,  0.0f,  0.0f }, -non_transf_aabb_mins[0] }; break;
                        case 2: next_plane = { Vector3{  0.0f, +1.0f,  0.0f },  non_transf_aabb_maxs[1] }; break;
                        case 3: next_plane = { Vector3{  0.0f, -1.0f,  0.0f }, -non_transf_aabb_mins[1] }; break;
                        case 4: next_plane = { Vector3{  0.0f,  0.0f, +1.0f },  non_transf_aabb_maxs[2] }; break;
                        case 5: next_plane = { Vector3{  0.0f,  0.0f, -1.0f }, -non_transf_aabb_mins[2] }; break;
                    }
                }
                else assert(0);

                // ======== Process next plane ========
                if (trace->info.isray) // Special point case
                {
                    //if (side.bevel == 1) // Don't ray trace against bevel planes
                    //    continue;

                    dist = next_plane.dist;
                }
                else // General box case
                {
                    // AABB contact point offset in the rotated coordinate system
                    ofs.x() = (Math::dot(unit_vec_0, next_plane.normal) < 0.0f) ? +transformed_extents.x() : -transformed_extents.x();
                    ofs.y() = (Math::dot(unit_vec_1, next_plane.normal) < 0.0f) ? +transformed_extents.y() : -transformed_extents.y();
                    ofs.z() = (Math::dot(unit_vec_2, next_plane.normal) < 0.0f) ? +transformed_extents.z() : -transformed_extents.z();

                    // AABB contact point offset in the regular coordinate system
                    // TODO can probably rewrite this to a matrix mult
                    Vector3 offset =
                        ofs[0] * unit_vec_0 +
                        ofs[1] * unit_vec_1 +
                        ofs[2] * unit_vec_2;

                    dist = next_plane.dist - Math::dot(offset, next_plane.normal);
                }

                d1 = Math::dot(start, next_plane.normal) - dist;
                d2 = Math::dot(  end, next_plane.normal) - dist;

                // If completely in front of face, no intersection
                if (d1 > 0.0f && d2 > 0.0f) {
                    skip_section = true; // Early-out from this entire section
                    break;
                }

                if (d2 > 0.0f)
                    getout = true; // Endpoint is not in solid
                if (d1 > 0.0f)
                    startout = true;

                if (d1 <= 0.0f && d2 <= 0.0f)
                    continue;

                // Crosses face
                if (d1 > d2) {
                    // Enter
                    f = (d1 - DIST_EPSILON) / (d1 - d2);
                    if (f > enterfrac) {
                        enterfrac = f;
                        // Need to transform clipplane back later!
                        clipplane = { next_plane.normal, next_plane.dist };
                        //leadside = &side;
                    }
                }
                else {
                    // Leave
                    f = (d1 + DIST_EPSILON) / (d1 - d2);
                    if (f < leavefrac)
                        leavefrac = f;
                }
            }
            if (skip_section)
                break; // Stop processing planes
        }

        if (skip_section)
            continue; // Go to next section

        if (!startout) { // If original point was inside brush
            trace->results.startsolid = true;
            if (!getout)
                trace->results.allsolid = true;
            continue; // Go to next section
        }

        if (enterfrac < leavefrac) {
            if (enterfrac > NEVER_UPDATED && enterfrac < trace->results.fraction) {
                // New closest object was hit!
                if (enterfrac < 0.0f)
                    enterfrac = 0.0f;
                trace->results.fraction = enterfrac;
                //trace->results.surface = leadside->texinfo; // Might be -1
                //trace->contents = brush.contents; // TODO: Return hit contents in a better way

                // Get regular rotation transformation by inverting the inverted
                // rotation transformation
                Quaternion xprop_rotation =
                    xprop_collcache.inv_rotation.invertedNormalized();

                // Transform plane normal back to regular coordinate system
                trace->results.plane_normal =
                    xprop_rotation.transformVectorNormalized(clipplane.normal);
            }
        }
        // --------- end of source-sdk-2013 code ---------
    }
}


////////////////////////////////////////////////////////////////////////////////


XPropSectionBevelPlaneLut::XPropSectionBevelPlaneLut(
    const Matrix3&    xprop_rotationscaling,
    const Quaternion& xprop_inv_rotation,
    float             xprop_inv_scale,
    const TriMesh& tri_mesh_of_xprop_section,
    const std::vector<BspMap::Plane>& planes_of_xprop_section)
{
    assert(xprop_inv_rotation.isNormalized());

    // List of indices of all validate candidates, unordered!
    std::vector<size_t> valid_candidate_indices;
    // Bevel planes of all valid candidates
    std::vector<Plane> valid_bevel_planes;

    // Calculate which edge bevel planes of this section of this static/dynamic
    // prop are valid and store that information in a LUT.
    // NOTE: Tests showed that these bevel planes don't exactly match CSGO's
    //       static prop hull tracing behaviour, so our results of tracing
    //       against props may slightly differ from CSGO's.
    //       To recreate it somehow, we copied brush bevel plane generation code
    //       from the Source 1 engine, it does its job.
    // Maybe CSGO's true bevel planes of static props are stored in some lump
    // inside a CSGO map file?

    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/utils/vbsp/map.cpp)
    // (Original code found CMapFile::AddBrushBevels() function)

    // @Optimization Note: Precomputing and storing all bevel planes for each
    //                     prop would require up to a couple megabytes for a
    //                     CSGO map.

    // For every unique edge of the section's triangle mesh
    for (size_t unique_edge_idx = 0;
         unique_edge_idx < tri_mesh_of_xprop_section.edges.size();
         unique_edge_idx++)
    {
        const TriMesh::Edge& u_edge = tri_mesh_of_xprop_section.edges[unique_edge_idx];
        Vector3 mesh_edge_v1 = tri_mesh_of_xprop_section.vertices[u_edge.verts[0]];
        Vector3 mesh_edge_v2 = tri_mesh_of_xprop_section.vertices[u_edge.verts[1]];
        // Transform points without translation!
        // @Optimization Is scaling + quaternion rotation faster than 3x3 mult?
        //               --> First test of this led to output differences
        //                   and small to no speed gains.
        mesh_edge_v1 = xprop_rotationscaling * mesh_edge_v1;
        mesh_edge_v2 = xprop_rotationscaling * mesh_edge_v2;

        // Test the non-axial plane edges
        Vector3 vec = mesh_edge_v1 - mesh_edge_v2;
        if (NormalizeInPlace(vec) < 0.5f)
            continue;
        SnapVector(vec);
        int i;
        for (i = 0; i < 3; i++)
            if (vec[i] == -1.0f || vec[i] == 1.0f)
                break; // Axial
        if (i != 3)
            continue; // Only test non-axial edges

        // Try the six possible slanted axials from this edge
        for (int axis = 0; axis < 3; axis++)
        {
            // @Optimization Instead of calculating planes for dir=-1 and dir=1
            //               separately, just calculate one of them, then negate
            //               its final_normal and final_dist to get the other one!
            for (int dir = -1; dir <= 1; dir += 2)
            {
                // Construct a plane
                Vector3 vec2 = { 0.0f, 0.0f, 0.0f };
                vec2[axis] = dir;
                Vector3 normal = Math::cross(vec, vec2);
                if (NormalizeInPlace(normal) < 0.5f)
                    continue;
                float dist = Math::dot(mesh_edge_v1, normal);

                // If this plane is identical to AABB plane, skip it
                // NOTE: Use a larger tolerance for collision planes than for rendering planes
                if (PlaneEqual({.normal={ +1.0f,  0.0f,  0.0f }, .dist=dist}, normal, dist, 0.01f, 0.01f)) continue;
                if (PlaneEqual({.normal={ -1.0f,  0.0f,  0.0f }, .dist=dist}, normal, dist, 0.01f, 0.01f)) continue;
                if (PlaneEqual({.normal={  0.0f, +1.0f,  0.0f }, .dist=dist}, normal, dist, 0.01f, 0.01f)) continue;
                if (PlaneEqual({.normal={  0.0f, -1.0f,  0.0f }, .dist=dist}, normal, dist, 0.01f, 0.01f)) continue;
                if (PlaneEqual({.normal={  0.0f,  0.0f, +1.0f }, .dist=dist}, normal, dist, 0.01f, 0.01f)) continue;
                if (PlaneEqual({.normal={  0.0f,  0.0f, -1.0f }, .dist=dist}, normal, dist, 0.01f, 0.01f)) continue;

                // Transform the constructed plane back
                Vector3 final_normal = xprop_inv_rotation.transformVectorNormalized(normal);
                float   final_dist = dist * xprop_inv_scale;

                // If all the points on all the sides are behind
                // this plane, it is a proper edge bevel
                size_t n;
                for (n = 0; n < tri_mesh_of_xprop_section.vertices.size(); n++) {
                    // @Optimization Is this redundant? Does this filter out planes at all?
                    float d = Math::dot(tri_mesh_of_xprop_section.vertices[n], final_normal) - final_dist;
                    if (d > 0.1f)
                        break; // Point in front
                }
                if (n != tri_mesh_of_xprop_section.vertices.size())
                    continue; // Wasn't part of the outer hull

                size_t m;
                for (m = 0; m < planes_of_xprop_section.size(); m++) {
                    const Plane& other_tri_plane = planes_of_xprop_section[m];

                    // If this plane has already been used, skip it
                    // NOTE: Use a larger tolerance for collision planes than for rendering planes
                    if (PlaneEqual(other_tri_plane, final_normal, final_dist, 0.01f, 0.01f))
                        break;
                }
                if (m != planes_of_xprop_section.size())
                    continue; // Wasn't part of the outer hull

                // Skip if this new plane is identical to a previous bevel plane
                size_t l;
                for (l = 0; l < valid_bevel_planes.size(); l++) {
                    const Plane& previous_bevel_plane = valid_bevel_planes[l];

                    // If this plane has already been used, skip it
                    // NOTE: Use a larger tolerance for collision planes than for rendering planes
                    if (PlaneEqual(previous_bevel_plane, final_normal, final_dist, 0.01f, 0.01f))
                        break;
                }
                if (l != valid_bevel_planes.size())
                    continue;

                // Add this plane
                size_t current_candidate_idx = GetCandidateIdx({
                    .unique_edge_idx = unique_edge_idx,
                    .axis = axis,
                    .dir = dir
                });
                valid_candidate_indices.push_back(current_candidate_idx); // Mark as valid

                // Remember bevel plane for comparison against future candidates
                Plane new_bevel_plane = { .normal = final_normal, .dist = final_dist };
                valid_bevel_planes.push_back(new_bevel_plane);
            }
        }
    }
    // --------- end of source-sdk-2013 code ---------

    // Now, store the valid bevel plane information in a format that's small and
    // fast to look things up in.
    // We start with the sorted list of valid bevel plane candidate indices, e.g.:
    //     valid_candidate_indices = [0, 4, 9, 83, 436, 491]
    // Convert that to an equivalent list of consecutive index steps, e.g.:
    //     valid_candidate_index_steps = [0, 4, 5, 74, 353, 55]
    // Store this in an integer array. Values that match or exceed the maximum
    // value of the used integer type (e.g. >= 255) are split into multiple
    // values, according to the 'recursive indexing' technique, as described by
    // Khalid Sayood in 'Introduction to Data Compression 3rd Ed.'. Example:
    //     valid_candidate_index_steps_recidx = [0, 4, 5, 74, 255, 98, 55]

    // Further example sequence conversions when using 'recursive indexing' and
    // a maximum int value of 255:
    //   A, 0, B  ->  A, 0, B
    //	 A, 1, B  ->  A, 1, B
    //   A, 2, B  ->  A, 2, B
    //	 ...
    //	 A, 253, B  ->  A, 253, B
    //	 A, 254, B  ->  A, 254, B
    //	 A, 255, B  ->  A, 255, 0, B
    //	 A, 256, B  ->  A, 255, 1, B
    //	 A, 257, B  ->  A, 255, 2, B
    //	 ...
    //	 A, 508, B  ->  A, 255, 253, B
    //	 A, 509, B  ->  A, 255, 254, B
    //	 A, 510, B  ->  A, 255, 255, 0, B
    //	 A, 511, B  ->  A, 255, 255, 1, B
    //	 A, 512, B  ->  A, 255, 255, 2, B
    //   ...

    // Maximum usable int value relevant to 'recursive indexing'
    constexpr size_t MAX_RECIDX_VALUE = std::numeric_limits<RecIdxType>::max();

    // Sort and then iterate over list of valid candidate indices in ascending order
    std::sort(valid_candidate_indices.begin(), valid_candidate_indices.end());
    valid_candidate_index_steps_recidx.reserve(1.2f * (float)valid_candidate_indices.size());
    size_t last_valid_candidate_idx = 0;
    for (size_t valid_candidate_idx : valid_candidate_indices) {
        // Get distance to previous valid candidate idx
        size_t dist_to_last_valid = valid_candidate_idx - last_valid_candidate_idx;

        // Encode this distance using 'recursive indexing'
        while (dist_to_last_valid >= MAX_RECIDX_VALUE) {
            valid_candidate_index_steps_recidx.push_back(MAX_RECIDX_VALUE);
            dist_to_last_valid -= MAX_RECIDX_VALUE;
        }
        valid_candidate_index_steps_recidx.push_back(dist_to_last_valid);

        // For following iterations
        last_valid_candidate_idx = valid_candidate_idx;
    }
    valid_candidate_index_steps_recidx.shrink_to_fit();
}

size_t XPropSectionBevelPlaneLut::GetMemorySize() const {
    return valid_candidate_index_steps_recidx.size() * sizeof(RecIdxType);
}

XPropSectionBevelPlaneGenerator::XPropSectionBevelPlaneGenerator(
    const CollisionModel&       xprop_coll_model,
    const CollisionCache_XProp& xprop_coll_cache,
    size_t idx_of_xprop_section)
    : cur_candidate_idx { 0 }
    , cur_lut_pos       { 0 }
    , xprop_inv_rotation{ xprop_coll_cache.inv_rotation }
    , tri_mesh_of_xprop_section{
        xprop_coll_model.section_tri_meshes[idx_of_xprop_section]
    }
    , valid_candidate_index_steps_recidx{
        xprop_coll_cache.section_bevel_luts[idx_of_xprop_section].valid_candidate_index_steps_recidx
    }
{
}

bool XPropSectionBevelPlaneGenerator::GetNext(BspMap::Plane* out)
{
    assert(out != nullptr);
    using RecIdxType            =  XPropSectionBevelPlaneLut::RecIdxType;
    using CandidateGenParams    =  XPropSectionBevelPlaneLut::CandidateGenParams;
    constexpr auto GetGenParams = &XPropSectionBevelPlaneLut::GetGenParams;

    // Maximum int value of 'recursively indexed' LUT
    constexpr size_t MAX_RECIDX_VALUE = std::numeric_limits<RecIdxType>::max();

    // Get next valid bevel plane candidate index from LUT
    while (true) {
        if (cur_lut_pos >= valid_candidate_index_steps_recidx.size())
            return false; // No more bevel planes left to generate

        // Decode 'recursively indexed' valid candidate index steps.
        // See LUT creation code for an explanation.
        RecIdxType next_lut_val = valid_candidate_index_steps_recidx[cur_lut_pos++];
        cur_candidate_idx += next_lut_val;
        if (next_lut_val < MAX_RECIDX_VALUE)
            break; // Next valid candidate idx was acquired

        // There must be more values after getting MAX_RECIDX_VALUE
        assert(cur_lut_pos < valid_candidate_index_steps_recidx.size());
    }

    // How we generate this bevel plane
    CandidateGenParams gen_params = GetGenParams(cur_candidate_idx);

    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/utils/vbsp/map.cpp)
    // (Original code found CMapFile::AddBrushBevels() function)

    // NOTE: The following code creates a bevel plane essentially the same way
    //       as the code that creates the LUT, except we completely omit any
    //       validity/redundancy checks (since the LUT already tells us it is
    //       valid). In addition, vector operations were simplified/reordered
    //       to calculate the same result, but faster.

    const TriMesh::Edge& unique_edge =
        tri_mesh_of_xprop_section.edges[gen_params.unique_edge_idx];
    Vector3 mesh_edge_v1 = tri_mesh_of_xprop_section.vertices[unique_edge.verts[0]];
    Vector3 mesh_edge_v2 = tri_mesh_of_xprop_section.vertices[unique_edge.verts[1]];
    Vector3 vec = mesh_edge_v1 - mesh_edge_v2;

    // Construct the axial normal
    Vector3 vec2 = { 0.0f, 0.0f, 0.0f };
    vec2[gen_params.axis] = gen_params.dir;

    // Transform(Rotate) axial normal back into coordinate system of unscaled,
    // unrotated and untranslated collision model
    vec2 = xprop_inv_rotation.transformVectorNormalized(vec2);

    // Construct bevel plane on the edge and orthogonal to the axial normal
    Vector3 final_normal = GetNormalized(Math::cross(vec, vec2));
    float   final_dist = Math::dot(mesh_edge_v1, final_normal);

    // Return bevel plane
    if (out) {
        out->normal = final_normal;
        out->dist   = final_dist;
    }
    // --------- end of source-sdk-2013 code ---------
    return true; // Plane generation was successful
}
