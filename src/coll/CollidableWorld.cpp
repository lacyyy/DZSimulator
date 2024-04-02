#include "coll/CollidableWorld.h"

#include <memory>
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

void CollidableWorld::DoTrace(Trace* trace)
{
    ZoneScoped;

    if (pImpl->bvh == Corrade::Containers::NullOpt) { // If BVH isn't created
        assert(false && "ERROR: Tried to run CollidableWorld::DoTrace() "
            "before BVH was created!");
        return;
    }

    coll::Debugger::DebugStart_Trace(trace->info);
    pImpl->bvh->DoTrace(trace, *this);
    coll::Debugger::DebugFinish_Trace(trace->results);
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
