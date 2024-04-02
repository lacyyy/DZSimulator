#include "coll/Trace.h"

#include <cfloat> // for FLT_MAX

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld.h"

using namespace coll;
using namespace Magnum;

bool Trace::HitsAabb(const Magnum::Vector3 &aabb_mins,
                     const Magnum::Vector3 &aabb_maxs,
                     float* hit_fraction) const
{
	// NOTE: The code in this method was written for swept traces, but also
	//       works for unswept traces.
	
    // @Optimization If this trace is unswept, just do a simple AABB-point
    //               intersection test?
    // @Optimization Make these trace tests inline? Use __forceinline on
    //               windows, like displacement collision code?

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
    hit_mins -= this->info.startpos;
    hit_maxs -= this->info.startpos;
    // Adjust for swept box by enlarging the child bounds to shrink the sweep
    // down to a point
    hit_mins -= this->info.extents;
    hit_maxs += this->info.extents;
    // Compute the parametric distance along the ray of intersection in each
    // dimension
    hit_mins *= this->info.invdelta;
    hit_maxs *= this->info.invdelta;
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

Vector3 Trace::ComputeInverseVec(const Vector3& vec) {
    // This inverse vector calculation was originally written to match
    // source-sdk-2013's displacement collision code 1 to 1.
    // TODO: Does displacement and other collision code still work if we use
    //       float's +/- infinity value instead of FLT_MAX?
    //       Is a positive(!) FLT_MAX or a positive(!) infinity value required
    //       when a vector's component is +0 or -0 ?
    Vector3 inv;
    for (int axis = 0; axis < 3; axis++) {
        if (vec[axis] != 0.0f) inv[axis] = 1.0f / vec[axis];
        else                   inv[axis] = FLT_MAX;
    }
    return inv;
}
