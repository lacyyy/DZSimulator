#include "coll/CollidableWorld-funcbrush.h"

#include <cstdint>
#include <string>
#include <vector>

#include <Tracy.hpp>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld.h"
#include "coll/CollidableWorld_Impl.h"
#include "coll/SweptTrace.h"
#include "csgo_parsing/BrushSeparation.h"
#include "csgo_parsing/BspMap.h"
#include "csgo_parsing/utils.h"
#include "utils_3d.h"

using namespace coll;
using namespace Magnum;
using namespace csgo_parsing;
using Plane          = BspMap::Plane;
using Brush          = BspMap::Brush;
using BrushSide      = BspMap::BrushSide;
using Ent_func_brush = BspMap::Ent_func_brush;

uint64_t CollidableWorld::GetSweptTraceCost_FuncBrush(uint32_t func_brush_idx)
{
    // See BVH::GetSweptLeafTraceCost() for details and considerations.
    return 1; // Is func_brush trace cost dependent on total brushside count?
}

void CollidableWorld::DoSweptTrace_FuncBrush(SweptTrace* trace,
    uint32_t func_brush_idx)
{
    ZoneScoped;

    // NOTE: Collision with func_brush entities was not thoroughly tested and
    //       the Source engine might be using an entirely different collision
    //       algorithm for func_brush specifically. (?)
    //       It's possible that further bevel planes need to be created and
    //       tested against when doing hull traces, in the same way we already
    //       do it for collisions with static/dynamic props.
    //       Maybe like this: https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/utils/vbsp/map.cpp#L463-L611
    //       Maybe useful:    https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/utils/vbsp/ivp.cpp#L1340

    // @OPTIMIZATION This function is pretty inefficient. However, there are
    //               only very few func_brush entities in DZ maps, so it might
    //               not matter.

    std::shared_ptr<const BspMap> bsp_map = pImpl->origin_bsp_map;
    const Ent_func_brush& func_brush = bsp_map->entities_func_brush[func_brush_idx];
    if (!func_brush.IsSolid())
        return; // Skip this func_brush

    Vector3 translated_trace_start = trace->info.startpos - func_brush.origin;

    // Order of axis rotations is important! First roll, then pitch, then yaw rotation!
    // @Optimization Use 3x3 rotation matrix, not 4x4, or quaternions
    Matrix4 rot_transformation =
        Matrix4::rotationZ(Deg{ func_brush.angles[1] }) * // (yaw)   rotation around z axis
        Matrix4::rotationY(Deg{ func_brush.angles[0] }) * // (pitch) rotation around y axis
        Matrix4::rotationX(Deg{ func_brush.angles[2] });  // (roll)  rotation around x axis

    if (func_brush.model.size() == 0 || func_brush.model[0] != '*')
        return; // Invalid, abort
    std::string idxStr = func_brush.model.substr(1);
    int64_t modelIdx = utils::ParseIntFromString(idxStr, -1);
    if (modelIdx <= 0 || modelIdx >= (int64_t)bsp_map->models.size())
        return; // Invalid model index, abort

    // NOTE: Rarely in CSGO maps, brushes have invalid brushsides/planes, i.e.
    //       they result in an AABB where (maxs[i] <= mins[i]) for some i.
    //       For the most part, we don't care about these rare cases, except for
    //       one func_brush in CSGO's "Only Up!" map by leander.
    //       (https://steamcommunity.com/sharedfiles/filedetails/?id=3012684086)
    bool are_we_in_csgo_only_up_map =
        bsp_map->map_version == 2915 && bsp_map->sky_name.compare("vertigoblue_hdr") == 0;

    bool hit = false;
    for (size_t brush_idx : bsp_map->GetModelBrushIndices(modelIdx)) {
        const Brush& brush = bsp_map->brushes[brush_idx];

        // Special case: grenadeclip brushes don't work in func_brush entities
        // (for unknown reasons)
        static auto test_f_exclude =
            BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::GRENADECLIP);
        if (test_f_exclude.first && test_f_exclude.first(brush))
            continue;

        static auto test_f_1 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::SOLID);
        static auto test_f_2 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::PLAYERCLIP);
        static auto test_f_3 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::LADDER);

        bool solid_to_player = false;
        if (test_f_1.first && test_f_1.first(brush)) solid_to_player = true;
        if (test_f_2.first && test_f_2.first(brush)) solid_to_player = true;
        if (test_f_3.first && test_f_3.first(brush)) solid_to_player = true;
        if (!solid_to_player)
            continue;

        // @Optimization Ensure the 6 axial brushsides/planes are processed first.
        std::vector<Plane> planes;
        for (int i = 0; i < brush.num_sides; i++) {
            const BrushSide& side = bsp_map->brushsides[brush.first_side + i];

            if (trace->info.isray && side.bevel)
                continue;

            // HACKHACK A specific brush in the CSGO Only Up map (by leander)
            //          has 2 invalid planes that cause issues, skip these.
            if (are_we_in_csgo_only_up_map && brush_idx == 2537)
                if (i == 26 || i == 30)
                    continue;

            Plane plane = bsp_map->planes[side.plane_num];
            plane.normal = rot_transformation.transformVector(plane.normal);
            planes.push_back(plane);
        }

        // -------- start of source-sdk-2013 code --------
        // (taken and modified from source-sdk-2013/<...>/src/utils/vrad/trace.cpp)
        
        // TODO move these constants' definitions and all their other
        //      occurrences somewhere else, maybe into CollidableWorld?
        const float DIST_EPSILON = 0.03125; // 1/32 epsilon to keep floating point happy
        const float NEVER_UPDATED = -9999;

        const Vector3 start = translated_trace_start;
        const Vector3 end   = translated_trace_start + trace->info.delta;
        const Vector3 mins = -trace->info.extents; // Box case only (!trace->info.isray)
        const Vector3 maxs = +trace->info.extents; // Box case only (!trace->info.isray)

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

        bool skip_brush = false;
        for (const Plane& plane : planes) {
            if (trace->info.isray) // Special point case
            {
                // Commented out because bevel planes were sorted out earlier
                //if (side.bevel == 1) // Don't ray trace against bevel planes
                //    continue;

                dist = plane.dist;
            }
            else // General box case
            {
                ofs.x() = (plane.normal.x() < 0.0f) ? maxs.x() : mins.x();
                ofs.y() = (plane.normal.y() < 0.0f) ? maxs.y() : mins.y();
                ofs.z() = (plane.normal.z() < 0.0f) ? maxs.z() : mins.z();

                dist = plane.dist - Math::dot(ofs, plane.normal);
            }

            d1 = Math::dot(start, plane.normal) - dist;
            d2 = Math::dot(end, plane.normal) - dist;

            // If completely in front of face, no intersection
            if (d1 > 0 && d2 > 0) {
                skip_brush = true;
                break;
            }

            if (d2 > 0)
                getout = true; // Endpoint is not in solid
            if (d1 > 0)
                startout = true;

            if (d1 <= 0 && d2 <= 0)
                continue;

            // Crosses face
            if (d1 > d2) {
                // Enter
                f = (d1 - DIST_EPSILON) / (d1 - d2);
                if (f > enterfrac) {
                    enterfrac = f;
                    clipplane = plane;
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

        if (skip_brush)
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
                if (enterfrac < 0)
                    enterfrac = 0;
                trace->results.fraction = enterfrac;
                //trace->results.surface = leadside->texinfo; // Might be -1
                //trace->contents = brush.contents; // TODO: Return hit contents in a better way
                trace->results.plane_normal = clipplane.normal;
            }
        }
        // --------- end of source-sdk-2013 code ---------
    }
}

bool coll::CalcAabb_FuncBrush(size_t func_brush_idx, const BspMap& bsp_map,
    Vector3* aabb_mins, Vector3* aabb_maxs)
{
    const Ent_func_brush& func_brush = bsp_map.entities_func_brush[func_brush_idx];

    // Get indices of all brushes contained in the func_brush
    if (func_brush.model.size() == 0 || func_brush.model[0] != '*')
        return false; // failure, invalid model
    std::string idx_str = func_brush.model.substr(1);
    int64_t model_idx = utils::ParseIntFromString(idx_str, -1);
    if (model_idx <= 0 || model_idx >= (int64_t)bsp_map.models.size())
        return false; // failure, invalid model
    const auto& brush_indices = bsp_map.GetModelBrushIndices(model_idx);

    // Collect all types of brushes, even if some are irrelevant to DZSimulator.
    // Rather make the AABB too big than too small.
    std::vector<std::vector<Vector3>> faces =
        bsp_map.GetBrushFaceVertices(brush_indices);

    if (faces.size() == 0)
        return false; // failure, non-existing AABB

    // Brush model of func_brush gets rotated and translated
    // @Optimization Use 3x3 rot matrix + translation or quaternion + translation?
    Matrix4 func_brush_transf = utils_3d::CalcModelTransformationMatrix(
        func_brush.origin,
        func_brush.angles
    );

    Vector3 mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
    Vector3 maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };

    // Apply rotation and translation to every vertex
    for (const std::vector<Vector3>& face : faces) {
        for (const Vector3& untransformed_v : face) {
            Vector3 v = func_brush_transf.transformPoint(untransformed_v);

            // Compute AABB of rotated and translated brush model
            for (int axis = 0; axis < 3; axis++) {
                if (v[axis] < mins[axis]) mins[axis] = v[axis];
                if (v[axis] > maxs[axis]) maxs[axis] = v[axis];
            }
        }
    }

    if (aabb_mins) *aabb_mins = mins;
    if (aabb_maxs) *aabb_maxs = maxs;
    return true; // success
}
