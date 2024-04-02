#include "coll/CollidableWorld-brush.h"

#include <Tracy.hpp>

#include <Magnum/Magnum.h>

#include "coll/CollidableWorld.h"
#include "coll/CollidableWorld_Impl.h"
#include "coll/Trace.h"
#include "csgo_parsing/BrushSeparation.h"
#include "csgo_parsing/BspMap.h"

using namespace coll;
using namespace Magnum;
using namespace csgo_parsing;
using Plane     = BspMap::Plane;
using Brush     = BspMap::Brush;
using BrushSide = BspMap::BrushSide;

uint64_t CollidableWorld::GetTraceCost_Brush(uint32_t brush_idx)
{
    // See BVH::GetLeafTraceCost() for details and considerations.
    return 1; // Is brush trace cost dependent on brushside count?
}

static bool IsBrushSolidToPlayer(const Brush& brush)
{
    static auto test_f_1 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::SOLID);
    static auto test_f_2 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::PLAYERCLIP);
    static auto test_f_3 = BrushSeparation::getBrushCategoryTestFuncs(BrushSeparation::LADDER);
    if (test_f_1.first && test_f_1.first(brush)) return true;
    if (test_f_2.first && test_f_2.first(brush)) return true;
    if (test_f_3.first && test_f_3.first(brush)) return true;
    return false;
}

void CollidableWorld::DoSweptTrace_Brush(Trace* trace, uint32_t brush_idx)
{
    assert(trace->info.isswept);
    ZoneScoped;

    const Brush& brush = pImpl->origin_bsp_map->brushes[brush_idx];
    if (!IsBrushSolidToPlayer(brush))
        return;

    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/utils/vrad/trace.cpp)
    
    // TODO move these constants' definitions and all their other occurrences
    //      somewhere else, maybe into CollidableWorld?
    const float DIST_EPSILON = 0.03125f; // 1/32 epsilon to keep floating point happy
    const float NEVER_UPDATED = -9999.0f;

    const Vector3 start = trace->info.startpos;
    const Vector3 end   = trace->info.startpos + trace->info.delta;
    const Vector3 mins = -trace->info.extents; // Box case only (!trace->info.isray)
    const Vector3 maxs = +trace->info.extents; // Box case only (!trace->info.isray)

    if (!brush.num_sides)
        return;

    const BrushSide* leadside  = nullptr;
    const Plane*     clipplane = nullptr;
    float enterfrac = NEVER_UPDATED;
    float leavefrac = 1.0f;
    bool  getout    = false;
    bool  startout  = false;

    float   dist;
    Vector3 ofs;
    float   d1, d2;
    float   f;

    // @Optimization Ensure the 6 axial brushsides/planes are processed first
    for (int i = 0; i < brush.num_sides; i++)
    {
        const BrushSide& side = pImpl->origin_bsp_map->brushsides[brush.first_side + i];
        const Plane& plane    = pImpl->origin_bsp_map->planes[side.plane_num];

        if (trace->info.isray) // Special point case
        {
            if (side.bevel == 1) // Don't ray trace against bevel planes
                continue;

            dist = plane.dist;
        }
        else // General box case
        {
            // Push the plane out apropriately for mins/maxs
            ofs.x() = (plane.normal.x() < 0.0f) ? maxs.x() : mins.x();
            ofs.y() = (plane.normal.y() < 0.0f) ? maxs.y() : mins.y();
            ofs.z() = (plane.normal.z() < 0.0f) ? maxs.z() : mins.z();

            dist = Math::dot(ofs, plane.normal);
            dist = plane.dist - dist;
        }

        d1 = Math::dot(start, plane.normal) - dist;
        d2 = Math::dot(  end, plane.normal) - dist;

        // If completely in front of face, no intersection
        if (d1 > 0.0f && d2 > 0.0f)
            return;

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
                clipplane = &plane;
                leadside = &side;
            }
        }
        else {
            // Leave
            f = (d1 + DIST_EPSILON) / (d1 - d2);
            if (f < leavefrac)
                leavefrac = f;
        }
    }

    if (!startout) { // If original point was inside brush
        trace->results.startsolid = true;
        if (!getout)
            trace->results.allsolid = true;
        return;
    }

    if (enterfrac < leavefrac) {
        if (enterfrac > NEVER_UPDATED && enterfrac < trace->results.fraction) {
            // New closest object was hit!
            if (enterfrac < 0.0f)
                enterfrac = 0.0f;
            trace->results.fraction     = enterfrac;
            trace->results.plane_normal = clipplane->normal;
            trace->results.surface      = leadside->texinfo; // Might be -1
            //trace->contents = brush.contents; // TODO: Return hit contents in a better way
        }
    }
    // --------- end of source-sdk-2013 code ---------
}

void CollidableWorld::DoUnsweptTrace_Brush(Trace* trace, uint32_t brush_idx)
{
    assert(trace->info.isswept == false);
    ZoneScoped;

    const Brush& brush = pImpl->origin_bsp_map->brushes[brush_idx];
    if (!IsBrushSolidToPlayer(brush))
        return;

    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/utils/vrad/trace.cpp)
    const Vector3 trace_pos = trace->info.startpos;
    const Vector3 mins = -trace->info.extents; // Box case only (!trace->info.isray)
    const Vector3 maxs = +trace->info.extents; // Box case only (!trace->info.isray)

    if (!brush.num_sides)
        return;

    float   dist;
    Vector3 ofs;

    // @Optimization Ensure the 6 axial brushsides/planes are processed first
    for (int i = 0; i < brush.num_sides; i++)
    {
        const BrushSide& side = pImpl->origin_bsp_map->brushsides[brush.first_side + i];
        const Plane& plane    = pImpl->origin_bsp_map->planes[side.plane_num];

        if (trace->info.isray) // Special point case
        {
            if (side.bevel == 1) // Don't ray trace against bevel planes
                continue;

            dist = plane.dist;
        }
        else // General box case
        {
            // Push the plane out apropriately for mins/maxs
            ofs.x() = (plane.normal.x() < 0.0f) ? maxs.x() : mins.x();
            ofs.y() = (plane.normal.y() < 0.0f) ? maxs.y() : mins.y();
            ofs.z() = (plane.normal.z() < 0.0f) ? maxs.z() : mins.z();

            dist = Math::dot(ofs, plane.normal);
            dist = plane.dist - dist;
        }

        float d1 = Math::dot(trace_pos, plane.normal) - dist;

        // If completely in front of face, no intersection
        if (d1 > 0.0f)
            return;
    }

    // If we got here, the trace intersects the brush
    trace->results.startsolid   = true;
    trace->results.allsolid     = true;
    // Trace fraction of 1.0 is interpreted as nothing being hit.
    // -> Need to set fraction to something below 1.0
    trace->results.fraction     = 0.0f;
    // Can't report a hit surface
    trace->results.plane_normal = Vector3(0.0f, 0.0f, 0.0f);
    trace->results.surface      = -1;
    //trace->contents = brush.contents; // TODO: Return hit contents in a better way
    // --------- end of source-sdk-2013 code ---------
}
