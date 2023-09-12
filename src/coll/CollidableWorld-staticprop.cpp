#include "coll/CollidableWorld-staticprop.h"

#include <cassert>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include <Corrade/Containers/Optional.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld.h"
#include "coll/CollidableWorld_Impl.h"
#include "coll/SweptTrace.h"
#include "csgo_parsing/BspMap.h"
#include "utils_3d.h"

using namespace coll;
using namespace Corrade;
using namespace Magnum;
using namespace csgo_parsing;
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

uint64_t CollidableWorld::GetSweptTraceCost_StaticProp(uint32_t sprop_idx)
{
    // See BVH::GetSweptLeafTraceCost() for details and considerations.
    return 1; // Is sprop trace cost dependent on triangle count?
}

Containers::Optional<CollisionCache_StaticProp>
coll::Create_CollisionCache_StaticProp(
    const BspMap::StaticProp& sprop, const CollisionModel& cmodel)
{
    // @Optimization If we are computing AABBs of transformed PHY sections, use
    //               those to quickly calculate sprop AABB from them.
    // @Optimization Maybe computing sprop AABB from a collmodel's shrinkwrap
    //               shape is faster.
    // @Optimization Instead of computing the exact AABB of every sprop, maybe
    //               just compute AABB of oriented/transformed PHY section AABBs

    Containers::Optional<CollisionCache_StaticProp> ret = { CollisionCache_StaticProp{} };

    Matrix4 sprop_transf = utils_3d::CalcModelTransformationMatrix(
        sprop.origin, sprop.angles, sprop.uniform_scale);

    ret->aabb_mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
    ret->aabb_maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };

    // Apply sprop transformation to every vertex
    bool no_vertex_found = true;
    const auto& sections = cmodel.section_tris;
    for (const auto& section : sections) {
        for (const std::vector<Vector3>& tri : section) {
            for (const Vector3& untransformed_v : tri) {
                Vector3 v = sprop_transf.transformPoint(untransformed_v);
                no_vertex_found = false;

                // Add transformed vertex to sprop AABB
                for (int axis = 0; axis < 3; axis++) {
                    ret->aabb_mins[axis] = Math::min(ret->aabb_mins[axis], v[axis]);
                    ret->aabb_maxs[axis] = Math::max(ret->aabb_maxs[axis], v[axis]);
                }
            }
        }
    }
    if (no_vertex_found) { // Invalid collision model
        assert(false && "Create_CollisionCache_StaticProp(): Invalid collision model");
        ret = { Containers::NullOpt };
    }
    return ret;
}

void CollidableWorld::DoSweptTrace_StaticProp(SweptTrace* trace,
    uint32_t sprop_idx)
{
    const BspMap::StaticProp& sprop = pImpl->origin_bsp_map->static_props[sprop_idx];
    if (!sprop.IsSolidWithVPhysics())
        return; // Skip this static prop

    const std::string& mdl_path =
        pImpl->origin_bsp_map->static_prop_model_dict[sprop.model_idx];

    // Ensure that required collision models and caches have been created
    assert(pImpl->sprop_coll_models != Corrade::Containers::NullOpt);
    assert(pImpl->coll_caches_sprop != Corrade::Containers::NullOpt);

    // Get collision model
    const auto& iter = pImpl->sprop_coll_models->find(mdl_path);
    if (iter == pImpl->sprop_coll_models->end())
        return; // This static prop has no collision model, skip
    const CollisionModel& collmodel = iter->second;

    const std::vector<std::vector<std::vector<Vector3>>>& section_tris =
        collmodel.section_tris;
    const std::vector<std::vector<Plane>>& section_planes =
        collmodel.section_planes;

    // @Optimization Precalculate this matrix? Are there many sprops with NOP
    //               transforms?
    Matrix4 sprop_transform = utils_3d::CalcModelTransformationMatrix(
        sprop.origin, sprop.angles, sprop.uniform_scale);


    // Transform trace into static prop's coordinate system.
    // (?? I think collmodel's coordinate system was meant)
    // TODO can probably combine all reverse operations in a single matrix4 mult
    
    Vector3 transformed_trace_start = trace->info.startpos;
    Vector3 transformed_trace_dir = trace->info.delta;

    // Orthogonal basis vectors that can describe any vector in a rotated
    // coordinate system
    Vector3 unit_vec_0 = { 1.0f, 0.0f, 0.0f };
    Vector3 unit_vec_1 = { 0.0f, 1.0f, 0.0f };
    Vector3 unit_vec_2 = { 0.0f, 0.0f, 1.0f };
    // TODO Can we multiply the extents values into the unit vecs to save CPU?

    // AABB extents expressed in the rotated coordinate system described above
    Vector3 transformed_extents = trace->info.extents;

    // Reverse sprop translation (opposite of CalcModelTransformationMatrix())
    transformed_trace_start -= sprop.origin;

    // Reverse sprop rotation (opposite of CalcModelTransformationMatrix())
    Matrix4 inv_rot =
        Matrix4::rotationX(Deg{ -sprop.angles[2] }) * // roll
        Matrix4::rotationY(Deg{ -sprop.angles[0] }) * // pitch
        Matrix4::rotationZ(Deg{ -sprop.angles[1] });  // yaw
    transformed_trace_start = inv_rot.transformPoint(transformed_trace_start);
    transformed_trace_dir = inv_rot.transformVector(transformed_trace_dir);
    unit_vec_0 = inv_rot.transformVector(unit_vec_0);
    unit_vec_1 = inv_rot.transformVector(unit_vec_1);
    unit_vec_2 = inv_rot.transformVector(unit_vec_2);

    // Reverse sprop scaling (opposite of CalcModelTransformationMatrix())
    float inv_scale = 1.0f / sprop.uniform_scale;
    transformed_trace_start *= inv_scale;
    transformed_trace_dir   *= inv_scale;
    transformed_extents     *= inv_scale;
    // inv_delta?

    for (size_t section_idx = 0; section_idx < section_tris.size(); section_idx++) {
        const std::vector<std::vector<Vector3>>& tris_of_section = section_tris[section_idx];
        const std::vector<Plane>& planes_of_section = section_planes[section_idx];

        // @Optimization Don't construct all planes at the start.
        //               Instead, construct while iterating.
        std::vector<Plane> planes;
        if (trace->info.isray) planes.reserve(planes_of_section.size());
        else                   planes.reserve(12 + planes_of_section.size());

        if (!trace->info.isray) { // Hull trace only
            // ==== Calculate AABB of section ====

            // AABB of transformed section
            // @Optimization Maybe doing a preliminary IsAabbHitByFullSweptTrace()
            //               with the precomputed AABB of the transformed
            //               section gives early-outs that speed everything up?
            planes.emplace_back(+unit_vec_0, -HUGE_VALF);
            planes.emplace_back(-unit_vec_0, -HUGE_VALF);
            planes.emplace_back(+unit_vec_1, -HUGE_VALF);
            planes.emplace_back(-unit_vec_1, -HUGE_VALF);
            planes.emplace_back(+unit_vec_2, -HUGE_VALF);
            planes.emplace_back(-unit_vec_2, -HUGE_VALF);

            // AABB of non-transformed section
            // @Optimization Testing showed that these 6 extra planes had little
            //               effect on getting trace results more similar to CSGO.
            //               These 6 extra planes can probably be removed.
            planes.emplace_back(Vector3{ +1.0f,  0.0f,  0.0f }, -HUGE_VALF);
            planes.emplace_back(Vector3{ -1.0f,  0.0f,  0.0f }, -HUGE_VALF);
            planes.emplace_back(Vector3{  0.0f, +1.0f,  0.0f }, -HUGE_VALF);
            planes.emplace_back(Vector3{  0.0f, -1.0f,  0.0f }, -HUGE_VALF);
            planes.emplace_back(Vector3{  0.0f,  0.0f, +1.0f }, -HUGE_VALF);
            planes.emplace_back(Vector3{  0.0f,  0.0f, -1.0f }, -HUGE_VALF);

            // Move each plane out to the furthest point it can contain
            // @Optimization Iterate over every unique vertex once, not every
            //               vertex of every triangle.
            for (Plane& plane : planes) {
                for (const std::vector<Vector3>& triangle : tris_of_section) {
                    for (const Vector3& vert : triangle) {
                        float vert_dist = Math::dot(plane.normal, vert);
                        plane.dist = Math::max(plane.dist, vert_dist);
                    }
                }
            }

            // @Optimization Note: Precomputing and storing all bevel planes for
            //               each sprop requires too much memory.
            // @Optimization Maybe precompute order of accepted and discarded
            //               bevel plane candidates, allows for faster discards.
            
            // Add edge bevel planes. Compute them for the transformed sprop.
            // TODO: Add note on this being inaccurate for sprops and how source
            //       engine seems to do it
            
            // -------- start of source-sdk-2013 code --------
            // (taken and modified from source-sdk-2013/<...>/src/utils/vbsp/map.cpp)
            for (const std::vector<Vector3>& triangle : tris_of_section) {
                std::vector<Vector3> transformed_triangle = {
                    // Transform points without translation!
                    sprop_transform.transformVector(triangle[0]),
                    sprop_transform.transformVector(triangle[1]),
                    sprop_transform.transformVector(triangle[2])
                };

                for (int j = 0; j < transformed_triangle.size(); j++) {
                    int k = (j + 1) % transformed_triangle.size();
                    Vector3 vec = transformed_triangle[j] - transformed_triangle[k];

                    // Test the non-axial plane edges
                    float edge_len = vec.length();
                    if (edge_len < 0.5f)
                        continue;
                    vec /= edge_len;
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
                        for (int dir = -1; dir <= 1; dir += 2)
                        {
                            // Construct a plane
                            Vector3 vec2 = { 0.0f, 0.0f, 0.0f };
                            vec2[axis] = dir;
                            Vector3 normal = Math::cross(vec, vec2);
                            float normal_len = normal.length();
                            if (normal_len < 0.5f)
                                continue;
                            normal /= normal_len;
                            float dist = Math::dot(transformed_triangle[j], normal);

                            // Transform the constructed plane back
                            Vector3 final_normal = inv_rot.transformVector(normal);
                            float   final_dist = dist * inv_scale;

                            // If all the points on all the sides are behind
                            // this plane, it is a proper edge bevel
                            size_t m;
                            for (m = 0; m < tris_of_section.size(); m++) {
                                const std::vector<Vector3>& other_tri = tris_of_section[m];
                                const Plane& other_tri_plane = planes_of_section[m];

                                // @Optimization Maybe it's faster to omit the PlaneEqual() check
                                //               and trace against the plane regardless.
                                //               Testing showed that omitting the PlaneEqual() check
                                //               does not seem to alter trace results.

                                // If this plane has already been used, skip it
                                // NOTE: Use a larger tolerance for collision planes than for rendering planes
                                if (PlaneEqual(other_tri_plane, final_normal, final_dist, 0.01f, 0.01f))
                                    break;

                                // @Optimization Should check every vertex once,
                                //               not every vertex of every triangle.
                                size_t l;
                                for (l = 0; l < other_tri.size(); l++)
                                {
                                    float d = Math::dot(other_tri[l], final_normal) - final_dist;
                                    if (d > 0.1f)
                                        break; // Point in front
                                }
                                if (l != other_tri.size())
                                    break;
                            }

                            if (m != tris_of_section.size())
                                continue; // Wasn't part of the outer hull

                            // Add this plane
                            // @Optimization Note: 7% to 24% of generated bevel
                            //               planes on DZ maps are redundant as
                            //               they're duplicates of another
                            //               generated bevel plane
                            planes.emplace_back(final_normal, final_dist);
                        }
                    }
                }
            }
            // --------- end of source-sdk-2013 code ---------
        }

        // Add plane of every triangle
        // @Optimization A few of these planes that describe an AABB-like shape
        //               should be sorted to the beginning of the planes array
        //               to make early-outs (when trace doesn't hit section)
        //               occuring even earlier more likely.
        //               ... wait, isn't this unnecessary for hull traces due to
        //                   AABB planes added earlier?
        for (const Plane& plane : planes_of_section)
            planes.push_back(plane);

        // -------- start of source-sdk-2013 code --------
        // (taken and modified from source-sdk-2013/<...>/src/utils/vrad/trace.cpp)
        
        // TODO Move these constants' definitions and all their other
        //      occurrences somewhere else, maybe into CollidableWorld?
        const float DIST_EPSILON = 0.03125f; // 1/32 epsilon to keep floating point happy
        const float NEVER_UPDATED = -9999;

        const Vector3 start = transformed_trace_start;
        const Vector3 end = transformed_trace_start + transformed_trace_dir;

        //const BrushSide* leadside = nullptr;
        Plane clipplane;
        float enterfrac = NEVER_UPDATED;
        float leavefrac = 1.0f;
        bool  getout = false;
        bool  startout = false;

        float   dist;
        Vector3 ofs;
        float   d1, d2;
        float   f;

        bool skip_section = false;
        // @Optimization Ensure 6 axial planes are processed first
        for (const Plane& plane : planes) {
            if (trace->info.isray) // Special point case
            {
                //if (side.bevel == 1) // Don't ray trace against bevel planes
                //    continue;

                dist = plane.dist;
            }
            else // General box case
            {
                // AABB contact point offset in the rotated coordinate system
                ofs.x() = (Math::dot(unit_vec_0, plane.normal) < 0.0f) ? +transformed_extents.x() : -transformed_extents.x();
                ofs.y() = (Math::dot(unit_vec_1, plane.normal) < 0.0f) ? +transformed_extents.y() : -transformed_extents.y();
                ofs.z() = (Math::dot(unit_vec_2, plane.normal) < 0.0f) ? +transformed_extents.z() : -transformed_extents.z();

                // AABB contact point offset in the regular coordinate system
                // TODO can probably rewrite this to a matrix mult
                Vector3 offset =
                    ofs[0] * unit_vec_0 +
                    ofs[1] * unit_vec_1 +
                    ofs[2] * unit_vec_2;

                dist = plane.dist - Math::dot(offset, plane.normal);
            }

            d1 = Math::dot(start, plane.normal) - dist;
            d2 = Math::dot(  end, plane.normal) - dist;

            // If completely in front of face, no intersection
            if (d1 > 0.0f && d2 > 0.0f) {
                skip_section = true;
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
                    clipplane = { plane.normal, plane.dist };
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
            continue;

        if (!startout) { // If original point was inside brush
            trace->results.startsolid = true;
            if (!getout)
                trace->results.allsolid = true;
            continue;
        }

        if (enterfrac < leavefrac) {
            if (enterfrac > NEVER_UPDATED && enterfrac < trace->results.fraction) {
                // New closest object was hit!
                if (enterfrac < 0.0f)
                    enterfrac = 0.0f;
                trace->results.fraction = enterfrac;
                //trace->results.surface = leadside->texinfo; // Might be -1
                //trace->contents = brush.contents; // TODO: Return hit contents in a better way

                Matrix4 sprop_rot =
                    Matrix4::rotationZ(Deg{ sprop.angles[1] }) * // yaw
                    Matrix4::rotationY(Deg{ sprop.angles[0] }) * // pitch
                    Matrix4::rotationX(Deg{ sprop.angles[2] });  // roll

                // Transform plane normal back to regular coordinate system
                trace->results.plane_normal =
                    sprop_rot.transformVector(clipplane.normal);
            }
        }
        // --------- end of source-sdk-2013 code ---------
    }
}
