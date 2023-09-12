#include "coll/SweptTrace.h"

#include <cfloat> // for FLT_MAX

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld.h"

using namespace coll;
using namespace Magnum;

bool SweptTrace::HitsAabbOnFullSweep(
    const Vector3& aabb_mins, const Vector3& aabb_maxs, float* hit_fraction) const
{
    return IsAabbHitByFullSweptTrace(
        this->info.startpos, this->info.invdelta, this->info.extents,
        aabb_mins, aabb_maxs, hit_fraction);
}

Vector3 SweptTrace::ComputeInverseVec(const Vector3& vec) {
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
