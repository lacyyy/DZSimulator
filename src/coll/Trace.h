#ifndef COLL_TRACE_H_
#define COLL_TRACE_H_

#include <cassert>
#include <cstdint>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

namespace coll {

// -------- start of source-sdk-2013 code --------
// (taken and modified from Ray_t, CBaseTrace and CToolTrace in
// source-sdk-2013/<...>/src/public/cmodel.h and
// source-sdk-2013/<...>/src/public/trace.h  and
// source-sdk-2013/<...>/src/utils/vrad/trace.cpp)
struct Trace
{
    // NOTE: A trace is a collision test in the game world. There are two types:
    //       Traces that are "swept" and traces that are not "swept".
    //       When performing swept traces, a point or AABB is moved continuously
    //       from point A to point B, and the trace reports whether it hit
    //       something.
    //       When performing unswept traces, a point or AABB isn't moved at all,
    //       and the trace reports whether it collides with something at a
    //       single position.
    //
    //       Unswept traces are performed by setting the trace start position
    //       equal to the trace end position.


    // Starting information of trace, does not change
    const struct Info {
        Magnum::Vector3 startpos;    // Starting point, centered within the extents
        Magnum::Vector3 startoffset; // Add this to startpos to get actual (uncentered) start
        Magnum::Vector3 delta;       // Direction and length of the ray
        Magnum::Vector3 invdelta;    // Precomputation for Line-AABB collisions. 1 / delta
        Magnum::Vector3 extents;     // Describes an axis aligned box extruded along a ray
        bool            isray;       // Are the extents zero?
        bool            isswept;     // Is delta != 0?
        //uint32_t        contents;    // Only collide with objects with these contents
    } info;


    // Intermediate and final results of trace
    struct Results {
        float           fraction;     // Time completed, 1.0 = didn't hit anything
        Magnum::Vector3 plane_normal; // Surface normal at impact
        int16_t         surface;      // (Only valid if a brush was hit?) Index into BspMap's texinfos array
        bool            startsolid;   // If true, the initial point was in a solid area
        bool            allsolid;     // If true, plane is not valid

        Results() // Initial result values
            : fraction     { 1.0f }
            , plane_normal { 0.0f, 0.0f, 0.0f }
            , surface      { -1 }
            , startsolid   { false }
            , allsolid     { false }
        {}

        bool DidHit() const { // Did the trace hit anything?
            return fraction < 1.0f || startsolid || allsolid;
        }
    } results;

    // =========================================================================

    // Init a ray trace (aka moving a point through the world until it hits something)
    // If start pos is equal to end pos, this becomes an unswept point trace instead.
    Trace(
        const Magnum::Vector3& ray_trace_start,
        const Magnum::Vector3& ray_trace_end)
        : info{
            .startpos    = ray_trace_start,
            .startoffset = { 0.0f, 0.0f, 0.0f },
            .delta       = ray_trace_end - ray_trace_start,
            .invdelta    = ComputeInverseVec(ray_trace_end - ray_trace_start),
            .extents     = { 0.0f, 0.0f, 0.0f },
            .isray       = true,
            .isswept     = (ray_trace_end - ray_trace_start).dot() != 0.0f,
        }
        , results{}
    {
        assert(0 && "WARNING: Ray traces are currently not accurate against some"
            " displacements due to required displacement collision structures "
            " not being initialized because ray traces are currently not used!");
    }

    // Init a hull trace (aka moving an AABB through the world until it hits something)
    // If start pos is equal to end pos, this becomes an unswept hull trace instead.
    Trace(
        const Magnum::Vector3& hull_trace_start,
        const Magnum::Vector3& hull_trace_end,
        const Magnum::Vector3& hull_mins,
        const Magnum::Vector3& hull_maxs)
        : info{
            // Offset start position to make it centered within the extents
            .startpos    = hull_trace_start + 0.5f * (hull_mins + hull_maxs),
            .startoffset = -0.5f * (hull_mins + hull_maxs),
            .delta       = hull_trace_end - hull_trace_start,
            .invdelta    = ComputeInverseVec(hull_trace_end - hull_trace_start),
            .extents     = (hull_maxs - hull_mins) * 0.5f,
            .isray       = false,
            .isswept     = (hull_trace_end - hull_trace_start).dot() != 0.0f,
        }
        , results{}
    {
    }

    // Init a trace from another trace's info
    Trace(const Info& other_info) : info{other_info }, results{}
    {
    }

    // =========================================================================

    // This function does not affect this trace's results! Returns whether this
    // trace hits the given AABB, ignoring all other collidable objects.
    // If the AABB is hit, hit_fraction gets set to the fraction of the point in
    // time of collision. hit_fraction is not modified otherwise!
    bool HitsAabb(const Magnum::Vector3& aabb_mins,
                  const Magnum::Vector3& aabb_maxs,
                  float* hit_fraction = nullptr) const;

private:
    static Magnum::Vector3 ComputeInverseVec(const Magnum::Vector3& vec);
};
// --------- end of source-sdk-2013 code ---------

} // namespace coll

#endif // COLL_TRACE_H_
