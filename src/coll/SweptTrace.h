#ifndef COLL_SWEPTTRACE_H_
#define COLL_SWEPTTRACE_H_

#include <cassert>
#include <cstdint>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Constants.h> // for nan
#include <Magnum/Math/Vector3.h>

namespace coll {

// -------- start of source-sdk-2013 code --------
// (taken and modified from Ray_t, CBaseTrace and CToolTrace in
// source-sdk-2013/<...>/src/public/cmodel.h and
// source-sdk-2013/<...>/src/public/trace.h  and
// source-sdk-2013/<...>/src/utils/vrad/trace.cpp)
struct SweptTrace
{
    // Starting information of trace, does not change
    const struct Info {
        Magnum::Vector3 startpos; // Starting point, centered within the extents
        Magnum::Vector3 delta;    // Direction and length of the ray
        Magnum::Vector3 invdelta; // Precomputation for Line-AABB collisions. 1 / delta
        Magnum::Vector3 extents;  // Describes an axis aligned box extruded along a ray
        bool            isray;    // Are the extents zero?
        //uint32_t        contents; // Only collide with objects with these contents
    } info;


    // Intermediate and final results of trace
    struct Results {
        float           fraction;     // Time completed, 1.0 = didn't hit anything
        Magnum::Vector3 plane_normal; // Surface normal at impact
        // surface is only valid if a brush was hit?
        int16_t         surface;      // Index into BspMap's texinfos array
        bool            startsolid;   // If true, the initial point was in a solid area
        bool            allsolid;     // If true, plane is not valid

        Results() // Initial result values
            : fraction     { 1.0f }
            , plane_normal { Magnum::Math::Constants<float>::nan() }
            , surface      { -1 }
            , startsolid   { false }
            , allsolid     { false }
        {}
    } results;


    // Init a ray trace (aka moving a point through the world until it hits something)
    SweptTrace(
        const Magnum::Vector3& ray_trace_start,
        const Magnum::Vector3& ray_trace_end)
        : info{
            .startpos = ray_trace_start,
            .delta    = ray_trace_end - ray_trace_start,
            .invdelta = ComputeInverseVec(ray_trace_end - ray_trace_start),
            .extents  = { 0.0f, 0.0f, 0.0f},
            .isray    = true,
        }
        , results{}
    {
        assert(0 && "WARNING: Ray traces are currently not accurate against some"
            " displacements due to required displacement collision structures "
            " not being initialized because ray traces are currently not used!");
    }

    // Init a hull trace (aka moving an AABB through the world until it hits something)
    SweptTrace(
        const Magnum::Vector3& hull_trace_start,
        const Magnum::Vector3& hull_trace_end,
        const Magnum::Vector3& hull_mins,
        const Magnum::Vector3& hull_maxs)
        : info{
            // Offset start position to make it centered within the extents
            .startpos = hull_trace_start + (hull_mins + hull_maxs) * 0.5f,
            .delta    = hull_trace_end - hull_trace_start,
            .invdelta = ComputeInverseVec(hull_trace_end - hull_trace_start),
            .extents  = (hull_maxs - hull_mins) * 0.5f,
            .isray    = false,
        }
        , results{}
    {
    }

    // Returns true if this trace hits the given AABB when doing a full sweep,
    // false if not. This function does not modify this trace's results!
    // If AABB is hit, hit_fraction gets set to the fraction of the point in
    // time of collision. hit_fraction is not modified otherwise!
    bool HitsAabbOnFullSweep(
        const Magnum::Vector3& aabb_mins,
        const Magnum::Vector3& aabb_maxs,
        float* hit_fraction = nullptr) const;

private:
    static Magnum::Vector3 ComputeInverseVec(const Magnum::Vector3& vec);
};
// --------- end of source-sdk-2013 code ---------

} // namespace coll

#endif // COLL_SWEPTTRACE_H_
