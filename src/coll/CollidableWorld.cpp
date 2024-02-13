#include "coll/CollidableWorld.h"

#include <map>
#include <memory>
#include <string>

#include <Tracy.hpp>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld_Impl.h"
#include "coll/Debugger.h"

using namespace coll;
using namespace Magnum;
using namespace csgo_parsing;


CollidableWorld::CollidableWorld(std::shared_ptr<const BspMap> bsp_map)
    : pImpl{ std::make_unique<Impl>(bsp_map) }
{
}

void CollidableWorld::DoSweptTrace(SweptTrace* trace)
{
    ZoneScoped;

    if (pImpl->bvh == Corrade::Containers::NullOpt) { // If BVH isn't created
        assert(false && "ERROR: Tried to run CollidableWorld::DoSweptTrace() "
            "before BVH was created!");
        return;
    }

    coll::Debugger::DebugStart_Trace(trace->info);

    // Do nothing if sweep distance of the swept trace is effectively zero.
    if (trace->info.delta.isZero()) {
        // Assert here for testing purposes, as it's currently unknown if
        // ignoring zero-distance swept traces has side effects in
        // source-sdk-2013's movement code.
        // TODO Test if movement is processed correctly with this abort. If it
        //      is, remove this assert.
        //assert(false);

        // 2024-02-13 Note: Ignoring zero-distance swept traces doesn't seem
        // to cause any issues in player movement (Note: Only walking and
        // jumping is implemented so far, no crouching).
        // But: Some source-sdk-2013 code (e.g. CGameMovement::CanUnDuckJump()
        // and CGameMovement::TryPlayerMove()) makes it obvious that zero-
        // distance swept traces are used in the Source engine to test whether
        // the player hull intersects with any map geometry. Therefor, we
        // should do these tests here for the "uswept box" case and set
        // startsolid to true when intersecting. Can this be achieved by simply
        // calling the DoSweptTrace() routine in these "uswept box" cases?
        // Commented out the assert because it will be replaced.

        coll::Debugger::DebugFinish_Trace(trace->results);
        return;
    }

    pImpl->bvh->DoSweptTrace(trace, *this);
    coll::Debugger::DebugFinish_Trace(trace->results);
}

bool CollidableWorld::DoesAabbIntersectAnyDisplacement(
    const Vector3& aabb_mins, const Vector3& aabb_maxs)
{
    if (pImpl->bvh == Corrade::Containers::NullOpt) { // If BVH isn't created
        assert(false && "ERROR: Tried to run "
            "CollidableWorld::DoesAabbIntersectAnyDisplacement() before BVH was "
            "created!");
        return false;
    }
    
    return pImpl->bvh->DoesAabbIntersectAnyDisplacement(
        aabb_mins, aabb_maxs, *this);
}

bool coll::AabbIntersectsAabb(
    const Vector3& mins0, const Vector3& maxs0,
    const Vector3& mins1, const Vector3& maxs1)
{
    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/public/dispcoll_common.cpp)
    // (AABB intersection code was originally found in IntersectFourBoxPairs())

    // NOTE: The original code was SIMD optimized. DZSimulator does not utilize
    //       SIMD for now. Furthermore, all CDispVector that were 16-aligned
    //       have been replaced with unaligned std::vector. Is their alignment
    //       necessary for SIMD?

    // Find the max mins and min maxs in each dimension
    Vector3 intersectMins = {
        Math::max(mins0.x(), mins1.x()),
        Math::max(mins0.y(), mins1.y()),
        Math::max(mins0.z(), mins1.z())
    };
    Vector3 intersectMaxs = {
        Math::min(maxs0.x(), maxs1.x()),
        Math::min(maxs0.y(), maxs1.y()),
        Math::min(maxs0.z(), maxs1.z())
    };
    // If intersectMins <= intersectMaxs then the boxes overlap in this dimension
    // If the boxes overlap in all three dimensions, they intersect
    return intersectMins.x() <= intersectMaxs.x()
        && intersectMins.y() <= intersectMaxs.y()
        && intersectMins.z() <= intersectMaxs.z();
    // --------- end of source-sdk-2013 code ---------
}

// @Optimization Make these trace tests inline?
//               Use __forceinline on windows, like displacement collision code?
bool coll::IsAabbHitByFullSweptTrace(
    const Vector3& ray_start, const Vector3& inv_delta, const Vector3& ray_extents,
    const Vector3& aabb_mins, const Vector3& aabb_maxs,
    float* hit_fraction)
{
    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/public/dispcoll_common.cpp)
    // (AABB trace code was originally found in IntersectRayWithFourBoxes())

    // NOTE: The original code was SIMD optimized. DZSimulator does not utilize
    //       SIMD for now. Furthermore, all CDispVector that were 16-aligned
    //       have been replaced with unaligned std::vector. Is their alignment
    //       necessary for SIMD?

    Vector3 hit_mins = aabb_mins;
    Vector3 hit_maxs = aabb_maxs;
    // Offset AABB to make trace start at origin
    hit_mins -= ray_start;
    hit_maxs -= ray_start;
    // Adjust for swept box by enlarging the child bounds to shrink the sweep
    // down to a point
    hit_mins -= ray_extents;
    hit_maxs += ray_extents;
    // Compute the parametric distance along the ray of intersection in each
    // dimension
    hit_mins *= inv_delta;
    hit_maxs *= inv_delta;
    // Find the max overall entry time across all dimensions
    float box_entry_t =                  Math::min(hit_mins.x(), hit_maxs.x());
    box_entry_t = Math::max(box_entry_t, Math::min(hit_mins.y(), hit_maxs.y()));
    box_entry_t = Math::max(box_entry_t, Math::min(hit_mins.z(), hit_maxs.z()));
    // Find the min overall exit time across all dimensions
    float box_exit_t  =                  Math::max(hit_mins.x(), hit_maxs.x());
    box_exit_t  = Math::min(box_exit_t,  Math::max(hit_mins.y(), hit_maxs.y()));
    box_exit_t  = Math::min(box_exit_t,  Math::max(hit_mins.z(), hit_maxs.z()));
    // Make sure hit check in the end does not succeed if the hit occurs
    // before the trace start time (t=0) or after the trace end time (t=1).
    box_entry_t = Math::max(box_entry_t, 0.0f);
    box_exit_t  = Math::min(box_exit_t,  1.0f);

    if (box_entry_t <= box_exit_t) { // If entry <= exit, we've got a hit
        if (hit_fraction) *hit_fraction = box_entry_t; // Also return time of hit
        return true;
    }
    return false;
    // --------- end of source-sdk-2013 code ---------
}
