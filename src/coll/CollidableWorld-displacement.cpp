#include "coll/CollidableWorld-displacement.h"

#include <cassert>
#include <cmath>
#include <functional> // for std::hash
#include <string_view>
#include <unordered_set>

#include <Tracy.hpp>

#include <Corrade/Containers/Optional.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld.h"
#include "coll/CollidableWorld_Impl.h"
#include "coll/Debugger.h"
#include "coll/SweptTrace.h"
#include "csgo_parsing/BspMap.h"
#include "utils_3d.h"


using namespace coll;
using namespace Magnum;
using namespace csgo_parsing;
using namespace utils_3d;


//
// IMPORTANT NOTICE:
//
// source-sdk-2013 implements '==' and '!=' vector comparisons using _exact_
// comparisons, where 2 vectors must not differ from each other to compare equal.
//
// In contrast, Magnum::Math (used by DZSimulator) implements them using _fuzzy_
// comparisons, where 2 vectors can differ slightly and still compare equal.
//
// -> Consequently: DZSim code that replicates source-sdk-2013 code must perform
//                  these vector comparisons using the SourceSdkVectorEqual()
//                  function and NOT using the Vector's '==' and '!=' operators!
//

// Compares vectors like source-sdk-2013 does. See note above.
// Returns true if the 2 vectors are exactly equal, false otherwise.
static bool SourceSdkVectorEqual(const Vector3& vec1, const Vector3& vec2) {
    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/public/mathlib/vector.h)
    // More specifically, code is from Vector::operator==()
    return vec1.x() == vec2.x() && vec1.y() == vec2.y() && vec1.z() == vec2.z();
    // --------- end of source-sdk-2013 code ---------
}


uint64_t CollidableWorld::GetSweptTraceCost_Displacement(uint32_t dispcoll_idx)
{
    // See BVH::GetSweptLeafTraceCost() for details and considerations.
    return 1; // Is displacement trace cost dependent on triangle count?
}

void CollidableWorld::DoSweptTrace_Displacement(SweptTrace* trace,
    uint32_t dispcoll_idx)
{
    ZoneScoped;

    assert(pImpl->hull_disp_coll_trees != Corrade::Containers::NullOpt);
    std::vector<CDispCollTree>& hull_disp_coll_trees = *pImpl->hull_disp_coll_trees;

    CDispCollTree& hull_dispcoll = hull_disp_coll_trees[dispcoll_idx];

    if (trace->info.isray) { // ray trace
        // NOTE: Right now, only collision structures of displacements required
        //       for *hull* collision are initialized! Once you need to perform
        //       *ray* collisions on displacements, initialize the displacements
        //       that don't have the NO_RAY_COLL flag as well and use them here!
        assert(0 && "ERROR: Unable to perform ray traces on displacements! See "
            "surrounding comment.");
        return;

        // Displacements with NO_RAY_COLL flag are not considered by AABBTree_Ray
        //hull_dispcoll.AABBTree_Ray(trace); // Returns true on hit
    }
    else { // hull trace
        // Displacements with NO_HULL_COLL flag are not considered by
        // AABBTree_SweepAABB.
        // Displacement collision cache might be created.
        // CAUTION: Not thread-safe yet!
        hull_dispcoll.AABBTree_SweepAABB(trace); // Returns true on hit
    }
}


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/mathlib/mathlib.h)

// plane_t structure
struct cplane_t
{
    Vector3 normal;
    float   dist;
    uint8_t type;     // for fast side tests
    uint8_t signbits; // signx + (signy<<1) + (signz<<1)
    //uint8_t pad[2];
};

// 0-2 are axial planes
#define PLANE_X    0
#define PLANE_Y    1
#define PLANE_Z    2

// 3-5 are non-axial planes snapped to the nearest
#define PLANE_ANYX 3
#define PLANE_ANYY 4
#define PLANE_ANYZ 5

static int BoxOnPlaneSide(const float* emins, const float* emaxs,
    const cplane_t* plane);

static inline int BoxOnPlaneSide(const Vector3& emins, const Vector3& emaxs,
    const cplane_t* plane)
{
    return BoxOnPlaneSide(emins.data(), emaxs.data(), plane);
}
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/mathlib/mathlib_base.cpp)

// Returns 1, 2, or 1 + 2
static int BoxOnPlaneSide(const float* emins, const float* emaxs,
    const cplane_t* p)
{
    float dist1, dist2;
    int   sides;

    // fast axial cases
    if (p->type < 3) {
        if (p->dist <= emins[p->type]) return 1;
        if (p->dist >= emaxs[p->type]) return 2;
        return 3;
    }

    // general case
    switch (p->signbits)
    {
    case 0:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        break;
    case 1:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        break;
    case 2:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        break;
    case 3:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        break;
    case 4:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        break;
    case 5:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        break;
    case 6:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        break;
    case 7:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        break;
    default:
        dist1 = dist2 = 0; // shut up compiler
        assert(0);
        break;
    }

    sides = 0;
    if (dist1 >= p->dist) sides = 1;
    if (dist2 <  p->dist) sides |= 2;
    assert(sides != 0);

    return sides;
}

// Bounding box construction methods
static void ClearBounds(Vector3& mins, Vector3& maxs)
{
    mins[0] = mins[1] = mins[2] = +HUGE_VALF;
    maxs[0] = maxs[1] = maxs[2] = -HUGE_VALF;
}
static void AddPointToBounds(const Vector3& v, Vector3& mins, Vector3& maxs)
{
    float val;
    for (int i = 0; i < 3; i++) {
        val = v[i];
        if (val < mins[i]) mins[i] = val;
        if (val > maxs[i]) maxs[i] = val;
    }
}
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/collisionutils.h)

// Intersects a ray with a triangle, returns distance t along ray.
// t will be less than zero if no intersection occurred
// oneSided will cull collisions which approach the triangle from the back
// side, assuming the vertices are specified in counter-clockwise order
// The vertices need not be specified in that order if oneSided is not used
static float IntersectRayWithTriangle(const SweptTrace& trace,
    const Vector3& v1, const Vector3& v2, const Vector3& v3,
    bool oneSided);

// Test for an intersection (overlap) between an axial-aligned bounding  box
// (AABB) and a triangle.
//
// Triangle points are in counter-clockwise order with the normal facing "out."
//
// Using the "Separating-Axis Theorem" to test for intersections between a
// triangle and an axial-aligned bounding box (AABB).
// 1. 3 Axis Plane Tests - x, y, z
// 2. 9 Edge Planes Tests - the 3 edges of the triangle crossed with all 3 axial 
//                          planes (x, y, z)
// 3. 1 Face Plane Test - the plane the triangle resides in (cplane_t plane)
static bool IsBoxIntersectingTriangle(
    const Vector3& vecBoxCenter, const Vector3& vecBoxExtents,
    const Vector3& v1, const Vector3& v2, const Vector3& v3,
    const cplane_t& plane, float flTolerance);
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/collisionutils.cpp)

// Compute the offset in t along the ray that we'll use for the collision
static float ComputeBoxOffset(const SweptTrace& trace)
{
    if (trace.info.isray)
        return 1e-3f;

    // Find the projection of the box diagonal along the ray...
    float offset =
        Math::abs(trace.info.extents[0] * trace.info.delta[0]) +
        Math::abs(trace.info.extents[1] * trace.info.delta[1]) +
        Math::abs(trace.info.extents[2] * trace.info.delta[2]);

    // We need to divide twice: Once to normalize the computation above
    // so we get something in units of extents, and the second to normalize
    // that with respect to the entire raycast.
    offset *= 1.0f / Math::max(1.0f, trace.info.delta.dot());

    // 1e-3 is an epsilon
    return offset + 1e-3;
}

// Intersects a swept box against a triangle
static float IntersectRayWithTriangle(const SweptTrace& trace,
    const Vector3& v1, const Vector3& v2, const Vector3& v3, bool oneSided)
{
    // This is cute: Use barycentric coordinates to represent the triangle
    // Vo(1-u-v) + V1u + V2v and intersect that with a line Po + Dt
    // This gives us 3 equations + 3 unknowns, which we can solve with
    // Cramer's rule...
    //      E1x u + E2x v - Dx t = Pox - Vox
    // There's a couple of other optimizations, Cramer's rule involves
    // computing the determinant of a matrix which has been constructed
    // by three vectors. It turns out that 
    // det | A B C | = -( A x C ) dot B or -(C x B) dot A
    // which we'll use below..

    Vector3 edge1, edge2, org;
    edge1 = v2 - v1;
    edge2 = v3 - v1;

    // Cull out one-sided stuff
    if (oneSided) {
        Vector3 normal;
        normal = Math::cross(edge1, edge2);
        if (Math::dot(normal, trace.info.delta) >= 0.0f)
            return -1.0f;
    }

    // FIXME: This is inaccurate, but fast for boxes
    // We want to do a fast separating axis implementation here
    // with a swept triangle along the reverse direction of the ray.

    // Compute some intermediary terms
    Vector3 dirCrossEdge2, orgCrossEdge1;
    dirCrossEdge2 = Math::cross(trace.info.delta, edge2);

    // Compute the denominator of Cramer's rule:
    //      | -Dx E1x E2x |
    // det  | -Dy E1y E2y | = (D x E2) dot E1
    //      | -Dz E1z E2z |
    float denom = Math::dot(dirCrossEdge2, edge1);
    if (Math::abs(denom) < 1e-6)
        return -1.0f;
    denom = 1.0f / denom;

    // Compute u. It's gotta lie in the range of 0 to 1.
    //                 | -Dx orgx E2x |
    // u = denom * det | -Dy orgy E2y | = (D x E2) dot org
    //                 | -Dz orgz E2z |
    org = trace.info.startpos - v1;
    float u = Math::dot(dirCrossEdge2, org) * denom;
    if ((u < 0.0f) || (u > 1.0f))
        return -1.0f;

    // Compute t and v the same way...
    // In barycentric coords, u + v < 1
    orgCrossEdge1 = Math::cross(org, edge1);
    float v = Math::dot(orgCrossEdge1, trace.info.delta) * denom;
    if ((v < 0.0f) || (v + u > 1.0f))
        return -1.0f;

    // Compute the distance along the ray direction that we need to fudge 
    // when using swept boxes
    float boxt = ComputeBoxOffset(trace);
    float t = Math::dot(orgCrossEdge1, edge2) * denom;
    if ((t < -boxt) || (t > 1.0f + boxt))
        return -1.0f;

    return Math::clamp(t, 0.0f, 1.0f);
}

// Purpose: find the minima and maxima of the 3 given values
static inline void FindMinMax(float v1, float v2, float v3,
    float& min, float& max)
{
    min = max = v1;
    if (v2 < min) min = v2;
    if (v2 > max) max = v2;
    if (v3 < min) min = v3;
    if (v3 > max) max = v3;
}

static inline bool AxisTestEdgeCrossX2(float flEdgeZ, float flEdgeY,
    float flAbsEdgeZ, float flAbsEdgeY, const Vector3& p1, const Vector3& p3,
    const Vector3& vecExtents, float flTolerance)
{
    // Cross Product( axialX(1,0,0) x edge ): x = 0.0f, y = edge.z, z = -edge.y
    // Triangle Point Distances: dist(x) = normal.y * pt(x).y + normal.z * pt(x).z
    float flDist1 = flEdgeZ * p1.y() - flEdgeY * p1.z();
    float flDist3 = flEdgeZ * p3.y() - flEdgeY * p3.z();

    // Extents are symmetric:
    //     dist = abs( normal.y ) * extents.y + abs( normal.z ) * extents.z
    float flDistBox = flAbsEdgeZ * vecExtents.y() + flAbsEdgeY * vecExtents.z();

    // Either dist1, dist3 is the closest point to the box, determine which and
    // test of overlap with box(AABB).
    if (flDist1 < flDist3) {
        if ((flDist1 >  (flDistBox + flTolerance)) ||
            (flDist3 < -(flDistBox + flTolerance)))
            return false;
    } else {
        if ((flDist3 >  (flDistBox + flTolerance)) ||
            (flDist1 < -(flDistBox + flTolerance)))
            return false;
    }
    return true;
}

static inline bool AxisTestEdgeCrossX3(float flEdgeZ, float flEdgeY,
    float flAbsEdgeZ, float flAbsEdgeY, const Vector3& p1, const Vector3& p2,
    const Vector3& vecExtents, float flTolerance)
{
    // Cross Product( axialX(1,0,0) x edge ): x = 0.0f, y = edge.z, z = -edge.y 
    // Triangle Point Distances: dist(x) = normal.y * pt(x).y + normal.z * pt(x).z
    float flDist1 = flEdgeZ * p1.y() - flEdgeY * p1.z();
    float flDist2 = flEdgeZ * p2.y() - flEdgeY * p2.z();

    // Extents are symmetric:
    //     dist = abs( normal.y ) * extents.y + abs( normal.z ) * extents.z
    float flDistBox = flAbsEdgeZ * vecExtents.y() + flAbsEdgeY * vecExtents.z();

    // Either dist1, dist2 is the closest point to the box, determine which and
    // test of overlap with box(AABB).
    if (flDist1 < flDist2) {
        if ((flDist1 >  (flDistBox + flTolerance)) ||
            (flDist2 < -(flDistBox + flTolerance)))
            return false;
    } else {
        if ((flDist2 >  (flDistBox + flTolerance)) ||
            (flDist1 < -(flDistBox + flTolerance)))
            return false;
    }
    return true;
}

static inline bool AxisTestEdgeCrossY2(float flEdgeZ, float flEdgeX,
    float flAbsEdgeZ, float flAbsEdgeX, const Vector3& p1, const Vector3& p3,
    const Vector3& vecExtents, float flTolerance)
{
    // Cross Product( axialY(0,1,0) x edge ): x = -edge.z, y = 0.0f, z = edge.x
    // Triangle Point Distances: dist(x) = normal.x * pt(x).x + normal.z * pt(x).z
    float flDist1 = -flEdgeZ * p1.x() + flEdgeX * p1.z();
    float flDist3 = -flEdgeZ * p3.x() + flEdgeX * p3.z();

    // Extents are symmetric:
    //     dist = abs( normal.x ) * extents.x + abs( normal.z ) * extents.z
    float flDistBox = flAbsEdgeZ * vecExtents.x() + flAbsEdgeX * vecExtents.z();

    // Either dist1, dist3 is the closest point to the box, determine which and
    // test of overlap with box(AABB).
    if (flDist1 < flDist3) {
        if ((flDist1 >  (flDistBox + flTolerance)) ||
            (flDist3 < -(flDistBox + flTolerance)))
            return false;
    } else {
        if ((flDist3 >  (flDistBox + flTolerance)) ||
            (flDist1 < -(flDistBox + flTolerance)))
            return false;
    }
    return true;
}

static inline bool AxisTestEdgeCrossY3(float flEdgeZ, float flEdgeX,
    float flAbsEdgeZ, float flAbsEdgeX, const Vector3& p1, const Vector3& p2,
    const Vector3& vecExtents, float flTolerance)
{
    // Cross Product( axialY(0,1,0) x edge ): x = -edge.z, y = 0.0f, z = edge.x
    // Triangle Point Distances: dist(x) = normal.x * pt(x).x + normal.z * pt(x).z
    float flDist1 = -flEdgeZ * p1.x() + flEdgeX * p1.z();
    float flDist2 = -flEdgeZ * p2.x() + flEdgeX * p2.z();

    // Extents are symmetric:
    //     dist = abs( normal.x ) * extents.x + abs( normal.z ) * extents.z
    float flDistBox = flAbsEdgeZ * vecExtents.x() + flAbsEdgeX * vecExtents.z();

    // Either dist1, dist2 is the closest point to the box, determine which and
    // test of overlap with box(AABB).
    if (flDist1 < flDist2) {
        if ((flDist1 >  (flDistBox + flTolerance)) ||
            (flDist2 < -(flDistBox + flTolerance)))
            return false;
    } else {
        if ((flDist2 >  (flDistBox + flTolerance)) ||
            (flDist1 < -(flDistBox + flTolerance)))
            return false;
    }
    return true;
}

static inline bool AxisTestEdgeCrossZ1(float flEdgeY, float flEdgeX,
    float flAbsEdgeY, float flAbsEdgeX, const Vector3& p2, const Vector3& p3,
    const Vector3& vecExtents, float flTolerance)
{
    // Cross Product( axialZ(0,0,1) x edge ): x = edge.y, y = -edge.x, z = 0.0f
    // Triangle Point Distances: dist(x) = normal.x * pt(x).x + normal.y * pt(x).y
    float flDist2 = flEdgeY * p2.x() - flEdgeX * p2.y();
    float flDist3 = flEdgeY * p3.x() - flEdgeX * p3.y();

    // Extents are symmetric:
    //     dist = abs( normal.x ) * extents.x + abs( normal.y ) * extents.y
    float flDistBox = flAbsEdgeY * vecExtents.x() + flAbsEdgeX * vecExtents.y();

    // Either dist2, dist3 is the closest point to the box, determine which and
    // test of overlap with box(AABB).
    if (flDist3 < flDist2) {
        if ((flDist3 >  (flDistBox + flTolerance)) ||
            (flDist2 < -(flDistBox + flTolerance)))
            return false;
    } else {
        if ((flDist2 >  (flDistBox + flTolerance)) ||
            (flDist3 < -(flDistBox + flTolerance)))
            return false;
    }
    return true;
}

static inline bool AxisTestEdgeCrossZ2(float flEdgeY, float flEdgeX,
    float flAbsEdgeY, float flAbsEdgeX, const Vector3& p1, const Vector3& p3,
    const Vector3& vecExtents, float flTolerance)
{
    // Cross Product( axialZ(0,0,1) x edge ): x = edge.y, y = -edge.x, z = 0.0f
    // Triangle Point Distances: dist(x) = normal.x * pt(x).x + normal.y * pt(x).y
    float flDist1 = flEdgeY * p1.x() - flEdgeX * p1.y();
    float flDist3 = flEdgeY * p3.x() - flEdgeX * p3.y();

    // Extents are symmetric:
    //     dist = abs( normal.x ) * extents.x + abs( normal.y ) * extents.y
    float flDistBox = flAbsEdgeY * vecExtents.x() + flAbsEdgeX * vecExtents.y();

    // Either dist1, dist3 is the closest point to the box, determine which and
    // test of overlap with box(AABB).
    if (flDist1 < flDist3) {
        if ((flDist1 >  (flDistBox + flTolerance)) ||
            (flDist3 < -(flDistBox + flTolerance)))
            return false;
    } else {
        if ((flDist3 >  (flDistBox + flTolerance)) ||
            (flDist1 < -(flDistBox + flTolerance)))
            return false;
    }
    return true;
}

static bool IsBoxIntersectingTriangle(
    const Vector3& vecBoxCenter, const Vector3& vecBoxExtents,
    const Vector3& v1, const Vector3& v2, const Vector3& v3,
    const cplane_t& plane, float flTolerance)
{
    // Test the axial planes (x,y,z) against the min, max of the triangle.
    float flMin, flMax;
    Vector3 p1, p2, p3;

    // x plane
    p1.x() = v1.x() - vecBoxCenter.x();
    p2.x() = v2.x() - vecBoxCenter.x();
    p3.x() = v3.x() - vecBoxCenter.x();
    FindMinMax(p1.x(), p2.x(), p3.x(), flMin, flMax);
    if ((flMin >  (vecBoxExtents.x() + flTolerance)) ||
        (flMax < -(vecBoxExtents.x() + flTolerance)))
        return false;

    // y plane
    p1.y() = v1.y() - vecBoxCenter.y();
    p2.y() = v2.y() - vecBoxCenter.y();
    p3.y() = v3.y() - vecBoxCenter.y();
    FindMinMax(p1.y(), p2.y(), p3.y(), flMin, flMax);
    if ((flMin >  (vecBoxExtents.y() + flTolerance)) ||
        (flMax < -(vecBoxExtents.y() + flTolerance)))
        return false;

    // z plane
    p1.z() = v1.z() - vecBoxCenter.z();
    p2.z() = v2.z() - vecBoxCenter.z();
    p3.z() = v3.z() - vecBoxCenter.z();
    FindMinMax(p1.z(), p2.z(), p3.z(), flMin, flMax);
    if ((flMin >  (vecBoxExtents.z() + flTolerance)) ||
        (flMax < -(vecBoxExtents.z() + flTolerance)))
        return false;

    // Test the 9 edge cases.
    Vector3 vecEdge, vecAbsEdge;

    // edge 0 (cross x,y,z)
    vecEdge = p2 - p1;
    vecAbsEdge.y() = Math::abs(vecEdge.y());
    vecAbsEdge.z() = Math::abs(vecEdge.z());
    if (!AxisTestEdgeCrossX2(vecEdge.z(), vecEdge.y(),
            vecAbsEdge.z(), vecAbsEdge.y(), p1, p3, vecBoxExtents, flTolerance))
        return false;

    vecAbsEdge.x() = Math::abs(vecEdge.x());
    if (!AxisTestEdgeCrossY2(vecEdge.z(), vecEdge.x(),
            vecAbsEdge.z(), vecAbsEdge.x(), p1, p3, vecBoxExtents, flTolerance))
        return false;

    if (!AxisTestEdgeCrossZ1(vecEdge.y(), vecEdge.x(),
            vecAbsEdge.y(), vecAbsEdge.x(), p2, p3, vecBoxExtents, flTolerance))
        return false;

    // edge 1 (cross x,y,z)
    vecEdge = p3 - p2;
    vecAbsEdge.y() = Math::abs(vecEdge.y());
    vecAbsEdge.z() = Math::abs(vecEdge.z());
    if (!AxisTestEdgeCrossX2(vecEdge.z(), vecEdge.y(),
            vecAbsEdge.z(), vecAbsEdge.y(), p1, p2, vecBoxExtents, flTolerance))
        return false;

    vecAbsEdge.x() = Math::abs(vecEdge.x());
    if (!AxisTestEdgeCrossY2(vecEdge.z(), vecEdge.x(),
            vecAbsEdge.z(), vecAbsEdge.x(), p1, p2, vecBoxExtents, flTolerance))
        return false;

    if (!AxisTestEdgeCrossZ2(vecEdge.y(), vecEdge.x(),
            vecAbsEdge.y(), vecAbsEdge.x(), p1, p3, vecBoxExtents, flTolerance))
        return false;

    // edge 2 (cross x,y,z)
    vecEdge = p1 - p3;
    vecAbsEdge.y() = Math::abs(vecEdge.y());
    vecAbsEdge.z() = Math::abs(vecEdge.z());
    if (!AxisTestEdgeCrossX3(vecEdge.z(), vecEdge.y(),
            vecAbsEdge.z(), vecAbsEdge.y(), p1, p2, vecBoxExtents, flTolerance))
        return false;

    vecAbsEdge.x() = Math::abs(vecEdge.x());
    if (!AxisTestEdgeCrossY3(vecEdge.z(), vecEdge.x(),
            vecAbsEdge.z(), vecAbsEdge.x(), p1, p2, vecBoxExtents, flTolerance))
        return false;

    if (!AxisTestEdgeCrossZ1(vecEdge.y(), vecEdge.x(),
            vecAbsEdge.y(), vecAbsEdge.x(), p2, p3, vecBoxExtents, flTolerance))
        return false;

    // Test against the triangle face plane.
    Vector3 vecMin, vecMax;
    vecMin = vecBoxCenter - vecBoxExtents;
    vecMax = vecBoxCenter + vecBoxExtents;
    if (BoxOnPlaneSide(vecMin, vecMax, &plane) != 3)
        return false;
    return true;
}
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/dispcoll_common.h)
#define DISPCOLL_DIST_EPSILON     0.03125f
#define DISPCOLL_ROOTNODE_INDEX   0
#define DISPCOLL_INVALID_FRAC     -99999.9f
#define DISPCOLL_NORMAL_UNDEF     0xffff

FORCEINLINE bool CDispCollTree::IsLeafNode(int iNode)
{
    return iNode >= m_nodes.size() ? true : false;
}

inline void CDispCollTree::CalcClosestExtents(const Vector3& vecPlaneNormal,
    const Vector3& vecBoxExtents, Vector3& vecBoxPoint)
{
    vecBoxPoint[0] = (vecPlaneNormal[0] < 0.0f) ? vecBoxExtents[0] : -vecBoxExtents[0];
    vecBoxPoint[1] = (vecPlaneNormal[1] < 0.0f) ? vecBoxExtents[1] : -vecBoxExtents[1];
    vecBoxPoint[2] = (vecPlaneNormal[2] < 0.0f) ? vecBoxExtents[2] : -vecBoxExtents[2];
}
// --------- end of source-sdk-2013 code ---------


// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/public/dispcoll_common.cpp)

static const Vector3 g_Vec3DispCollEpsilons{
    DISPCOLL_DIST_EPSILON,
    DISPCOLL_DIST_EPSILON,
    DISPCOLL_DIST_EPSILON
};

struct DispCollPlaneIndex_t
{
    Vector3 vecPlane;
    int index;

    // Compare
    bool operator==(const DispCollPlaneIndex_t& other) const {
        return (SourceSdkVectorEqual(this->vecPlane,  other.vecPlane) ||
                SourceSdkVectorEqual(this->vecPlane, -other.vecPlane));
    }
};

class CPlaneIndexHashFuncs
{
public:
    // Hash
    size_t operator()(const DispCollPlaneIndex_t& item) const {
        return HashVector3(item.vecPlane) ^ HashVector3(-item.vecPlane);
    }
private:
    size_t HashVector3(const Vector3& vec) const {
        // Create std::string_view of the vector's values
        const char* ptr = reinterpret_cast<const char*>(vec.data());
        constexpr size_t byte_count = Vector3::Size * sizeof(Vector3::Type);
        std::string_view sv{ ptr, byte_count };

        // Compute vector hash value using std::hash specialization for
        // std::string_view
        return std::hash<std::string_view>{}(sv);
    }
};

// @Optimization Is 512 a good default bucket count?
//               Theoretical max of unique keys during current usage is 672.
//               Test if 512 are enough buckets? Do allocations occur?
static std::unordered_set<DispCollPlaneIndex_t, CPlaneIndexHashFuncs>
                                                  g_DispCollPlaneIndexHash(512);


// Displacement Collision Triangle
CDispCollTri::CDispCollTri()
{
    Init();
}

void CDispCollTri::Init()
{
    m_vecNormal = { 0.0f, 0.0f, 0.0f };
    m_flDist = 0.0f;
    m_TriData[0].m_IndexDummy = 0;
    m_TriData[1].m_IndexDummy = 0;
    m_TriData[2].m_IndexDummy = 0;
}

void CDispCollTri::CalcPlane(std::vector<Vector3>& m_aVerts)
{
    Vector3 vecEdges[2] = {
        m_aVerts[GetVert(1)] - m_aVerts[GetVert(0)],
        m_aVerts[GetVert(2)] - m_aVerts[GetVert(0)]
    };

    m_vecNormal = Math::cross(vecEdges[1], vecEdges[0]);
    NormalizeInPlace(m_vecNormal);
    m_flDist = Math::dot(m_vecNormal, m_aVerts[GetVert(0)]);

    // Calculate the signbits for the plane - fast test.
    m_ucSignBits = 0;
    m_ucPlaneType = PLANE_ANYZ;
    for (int iAxis = 0; iAxis < 3; iAxis++) {
        if (m_vecNormal[iAxis] < 0.0f)
            m_ucSignBits |= 1 << iAxis;

        if (m_vecNormal[iAxis] == 1.0f)
            m_ucPlaneType = iAxis;
    }
}

static inline void FindMin(float v1, float v2, float v3, int& iMin)
{
    float flMin = v1;
    iMin = 0;
    if (v2 < flMin) { flMin = v2; iMin = 1; }
    if (v3 < flMin) { flMin = v3; iMin = 2; }
}

static inline void FindMax(float v1, float v2, float v3, int& iMax)
{
    float flMax = v1;
    iMax = 0;
    if (v2 > flMax) { flMax = v2; iMax = 1; }
    if (v3 > flMax) { flMax = v3; iMax = 2; }
}

void CDispCollTri::FindMinMax(std::vector<Vector3>& m_aVerts)
{
    int iMin, iMax;
    FindMin(m_aVerts[GetVert(0)].x(), m_aVerts[GetVert(1)].x(), m_aVerts[GetVert(2)].x(), iMin);
    FindMax(m_aVerts[GetVert(0)].x(), m_aVerts[GetVert(1)].x(), m_aVerts[GetVert(2)].x(), iMax);
    SetMin(0, iMin);
    SetMax(0, iMax);

    FindMin(m_aVerts[GetVert(0)].y(), m_aVerts[GetVert(1)].y(), m_aVerts[GetVert(2)].y(), iMin);
    FindMax(m_aVerts[GetVert(0)].y(), m_aVerts[GetVert(1)].y(), m_aVerts[GetVert(2)].y(), iMax);
    SetMin(1, iMin);
    SetMax(1, iMax);

    FindMin(m_aVerts[GetVert(0)].z(), m_aVerts[GetVert(1)].z(), m_aVerts[GetVert(2)].z(), iMin);
    FindMax(m_aVerts[GetVert(0)].z(), m_aVerts[GetVert(1)].z(), m_aVerts[GetVert(2)].z(), iMax);
    SetMin(2, iMin);
    SetMax(2, iMax);
}

// NOTE: DZSimulator does not support SIMD for now. Furthermore, all CDispVector
//       that were 16-aligned have been replaced with unaligned std::vector. Is
//       their alignment necessary for SIMD?

// Intersecting with the quad tree. Returned value explanation:
// if (retval & 1) then box of SW node child was hit
// if (retval & 2) then box of SE node child was hit
// if (retval & 4) then box of NW node child was hit
// if (retval & 8) then box of NE node child was hit
static FORCEINLINE int IntersectRayWithFourBoxes(
    // ==== Original arguments from source-sdk-2013 that utilize SIMD
    ////const FourVectors& rayStart, const FourVectors& invDelta,
    ////const FourVectors& rayExtents,
    ////const FourVectors& boxMins, const FourVectors& boxMaxs
    // ==== Non-SIMD replacements of the arguments above
    const Vector3& rayStart, const Vector3& invDelta, const Vector3& rayExtents,
    const Vector3(&boxMins)[4], const Vector3(&boxMaxs)[4]
    // ==== end of replacement
)
{
    // ==== The following code is the original function body from source-sdk-2013
    //      that utilizes SIMD.
    
    ////// SIMD Test ray against all four boxes at once.
    ////// Each node stores the bboxes of its four children.
    ////FourVectors hitMins = boxMins;
    ////FourVectors hitMaxs = boxMaxs;
    ////hitMins -= rayStart;
    ////hitMaxs -= rayStart;
    ////
    ////// Adjust for swept box by enlarging the child bounds to shrink the sweep
    ////// down to a point
    ////hitMins -= rayExtents;
    ////hitMaxs += rayExtents;
    ////
    ////// Compute the parametric distance along the ray of intersection in each
    ////// dimension
    ////hitMins *= invDelta;
    ////hitMaxs *= invDelta;
    ////
    ////// Find the exit parametric intersection distance in each dimension, for each box
    ////FourVectors exitT = maximum(hitMins, hitMaxs);
    ////// Find the entry parametric intersection distance in each dimension, for each box
    ////FourVectors entryT = minimum(hitMins, hitMaxs);
    ////
    ////// Now find the max overall entry distance across all dimensions for each box
    ////fltx4 minTemp = MaxSIMD(entryT.x, entryT.y);
    ////fltx4 boxEntryT = MaxSIMD(minTemp, entryT.z);
    ////// Now find the min overall exit distance across all dimensions for each box
    ////fltx4 maxTemp = MinSIMD(exitT.x, exitT.y);
    ////fltx4 boxExitT = MinSIMD(maxTemp, exitT.z);
    ////
    ////boxEntryT = MaxSIMD(boxEntryT, Four_Zeros);
    ////boxExitT  = MinSIMD(boxExitT,  Four_Ones );
    ////
    ////// If entry<=exit for the box, we've got a hit.
    ////fltx4 active = CmpLeSIMD(boxEntryT, boxExitT); // Mask of which boxes are active
    ////
    ////// Hit at least one box?
    ////return TestSignSIMD(active);

    // ==== The following code is the non-SIMD replacement of the code above.
    int hit_mask = 0;
    for (int child_idx = 0; child_idx < 4; child_idx++) {
        if (IsAabbHitByFullSweptTrace(rayStart, invDelta, rayExtents,
                                        boxMins[child_idx], boxMaxs[child_idx]))
            hit_mask |= 1 << child_idx;
    }
    return hit_mask;
}

// This does 4 simultaneous box intersections
// NOTE: This can be used as a 1 vs 4 test by replicating a single box into the
// one side
static FORCEINLINE int IntersectFourBoxPairs(
    // ==== Original arguments from source-sdk-2013 that utilize SIMD
    ////const FourVectors& mins0, const FourVectors& maxs0,
    ////const FourVectors& mins1, const FourVectors& maxs1
    // ==== Non-SIMD replacements of the arguments above
    const Vector3& mins0, const Vector3& maxs0,
    const Vector3(&mins1)[4], const Vector3(&maxs1)[4]
    // ==== end of replacement
)
{
    // ==== The following code is the original function body from source-sdk-2013
    //      that utilizes SIMD.

    ////// Find the max mins and min maxs in each dimension
    ////FourVectors intersectMins = maximum(mins0, mins1);
    ////FourVectors intersectMaxs = minimum(maxs0, maxs1);
    //// 
    ////// If intersectMins <= intersectMaxs then the boxes overlap in this dimension
    ////fltx4 overlapX = CmpLeSIMD(intersectMins.x, intersectMaxs.x);
    ////fltx4 overlapY = CmpLeSIMD(intersectMins.y, intersectMaxs.y);
    ////fltx4 overlapZ = CmpLeSIMD(intersectMins.z, intersectMaxs.z);
    //// 
    ////// If the boxes overlap in all three dimensions, they intersect
    ////fltx4 tmp = AndSIMD(overlapX, overlapY);
    ////fltx4 active = AndSIMD(tmp, overlapZ);
    //// 
    ////// Hit at least one box?
    ////return TestSignSIMD(active);

    // ==== The following code is the non-SIMD replacement of the code above.
    int hit_mask = 0;
    for (int child_idx = 0; child_idx < 4; child_idx++)
        if (AabbIntersectsAabb(mins0, maxs0, mins1[child_idx], maxs1[child_idx]))
            hit_mask |= 1 << child_idx;
    return hit_mask;
}

int FORCEINLINE CDispCollTree::BuildRayLeafList(int iNode, rayleaflist_t& list)
{
    list.nodeList[0] = iNode;
    int listIndex = 0;
    list.maxIndex = 0;
    while (listIndex <= list.maxIndex) {
        iNode = list.nodeList[listIndex];
        // the rest are all leaves
        if (IsLeafNode(iNode))
            return listIndex;
        listIndex++;
        const CDispCollNode& node = m_nodes[iNode];
        int mask = IntersectRayWithFourBoxes(list.rayStart, list.invDelta,
            list.rayExtents, node.m_mins, node.m_maxs);
        if (mask) {
            int child = Nodes_GetChild(iNode, 0);
            if (mask & 1) {
                list.maxIndex++;
                list.nodeList[list.maxIndex] = child;
            }
            if (mask & 2) {
                list.maxIndex++;
                list.nodeList[list.maxIndex] = child + 1;
            }
            if (mask & 4) {
                list.maxIndex++;
                list.nodeList[list.maxIndex] = child + 2;
            }
            if (mask & 8) {
                list.maxIndex++;
                list.nodeList[list.maxIndex] = child + 3;
            }
            assert(list.maxIndex < MAX_AABB_LIST);
        }
    }
    return listIndex;
}

void CDispCollTree::AABBTree_Create(const std::vector<Vector3>& disp_vertices)
{
    // Copy necessary displacement data.
    AABBTree_CopyDispData(disp_vertices);

    // Setup/create the leaf nodes first so the recusion can use this data to stop.
    AABBTree_CreateLeafs();

    // Create the bounding box of the displacement surface
    AABBTree_CalcBounds();
}

void CDispCollTree::AABBTree_CopyDispData(const std::vector<Vector3>& disp_vertices)
{
    // Allocate collision tree data.
    m_aVerts = std::vector<Vector3>     (GetSize());
    m_aTris  = std::vector<CDispCollTri>(GetTriSize());
    m_leaves = {};
    m_nodes  = {};

    // Copy vertex data.
    // @Optimization Save memory: Maybe don't copy vertices, just reference them?
    assert(disp_vertices.size() == m_aVerts.size());
    for (int iVert = 0; iVert < m_aVerts.size(); iVert++)
        m_aVerts[iVert] = disp_vertices[iVert];

    // Setup triangle data.
    int iTri = 0;
    int width  = GetWidth();
    int height = GetHeight();
    for (int tileY = 0; tileY < height - 1; tileY++) {
        for (int tileX = 0; tileX < width - 1; tileX++) {
            int vert_idx = (tileY * width) + tileX;

            // Create 2 triangles from tile
            for (int tileTri = 0; tileTri < 2; tileTri++) {
                // Switch up triangle-separating diagonal every tile
                if ((tileX + tileY) % 2 == 0) {
                    if (tileTri == 0) {
                        m_aTris[iTri].SetVert(0, vert_idx);
                        m_aTris[iTri].SetVert(1, vert_idx + width);
                        m_aTris[iTri].SetVert(2, vert_idx + width + 1);
                    } else {
                        m_aTris[iTri].SetVert(0, vert_idx);
                        m_aTris[iTri].SetVert(1, vert_idx + width + 1);
                        m_aTris[iTri].SetVert(2, vert_idx + 1);
                    }
                } else {
                    if (tileTri == 0) {
                        m_aTris[iTri].SetVert(0, vert_idx);
                        m_aTris[iTri].SetVert(1, vert_idx + width);
                        m_aTris[iTri].SetVert(2, vert_idx + 1);
                    } else {
                        m_aTris[iTri].SetVert(0, vert_idx + 1);
                        m_aTris[iTri].SetVert(1, vert_idx + width);
                        m_aTris[iTri].SetVert(2, vert_idx + width + 1);
                    }
                }
                // Calculate the plane normal and the min max.
                m_aTris[iTri].CalcPlane(m_aVerts);
                m_aTris[iTri].FindMinMax(m_aVerts);
                // Advance to next triangle
                iTri++;
            }
        }
    }
}

void CDispCollTree::AABBTree_CreateLeafs()
{
    int numLeaves = (GetWidth() - 1) * (GetHeight() - 1);
    int numNodes = Nodes_CalcCount(m_nPower);
    numNodes -= numLeaves;
    m_leaves = std::vector<CDispCollLeaf>(numLeaves);
    m_nodes  = std::vector<CDispCollNode>(numNodes);

    // Get the width and height of the displacement.
    int nWidth  = GetWidth()  - 1;
    int nHeight = GetHeight() - 1;

    for (int iHgt = 0; iHgt < nHeight; iHgt++) {
        for (int iWid = 0; iWid < nWidth; iWid++) {
            int iLeaf = Nodes_GetIndexFromComponents(iWid, iHgt);
            int iIndex = iHgt * nWidth + iWid;
            int iTri = iIndex * 2;

            m_leaves[iLeaf].m_tris[0] = iTri;
            m_leaves[iLeaf].m_tris[1] = iTri + 1;
        }
    }
}

void CDispCollTree::AABBTree_GenerateBoxes_r(int nodeIndex,
    Vector3* pMins, Vector3* pMaxs)
{
    ClearBounds(*pMins, *pMaxs);

    // leaf
    if (nodeIndex >= m_nodes.size()) {
        int iLeaf = nodeIndex - m_nodes.size();

        for (int iTri = 0; iTri < 2; iTri++) {
            int triIndex = m_leaves[iLeaf].m_tris[iTri];
            const CDispCollTri& tri = m_aTris[triIndex];
            AddPointToBounds(m_aVerts[tri.GetVert(0)], *pMins, *pMaxs);
            AddPointToBounds(m_aVerts[tri.GetVert(1)], *pMins, *pMaxs);
            AddPointToBounds(m_aVerts[tri.GetVert(2)], *pMins, *pMaxs);
        }
    }
    // node
    else {
        Vector3 childMins[4], childMaxs[4];
        for (int i = 0; i < 4; i++) {
            int child = Nodes_GetChild(nodeIndex, i);
            AABBTree_GenerateBoxes_r(child, &childMins[i], &childMaxs[i]);
            AddPointToBounds(childMins[i], *pMins, *pMaxs);
            AddPointToBounds(childMaxs[i], *pMins, *pMaxs);
        }

        // ==== The following is the original source-sdk-2013 code that utilizes SIMD.
        ////m_nodes[nodeIndex].m_mins.LoadAndSwizzle(childMins[0], childMins[1],
        ////                                         childMins[2], childMins[3]);
        ////m_nodes[nodeIndex].m_maxs.LoadAndSwizzle(childMaxs[0], childMaxs[1],
        ////                                         childMaxs[2], childMaxs[3]);
        // ==== The following is the non-SIMD replacement of the code above.
        for (int i = 0; i < 4; i++) {
            m_nodes[nodeIndex].m_mins[i] = childMins[i];
            m_nodes[nodeIndex].m_maxs[i] = childMaxs[i];
        }
    }
}

void CDispCollTree::AABBTree_CalcBounds()
{
    // Check data.
    if ((m_aVerts.size() == 0) || (m_nodes.size() == 0))
        return;

    AABBTree_GenerateBoxes_r(0, &m_mins, &m_maxs);

    // Bloat a little.
    for (int iAxis = 0; iAxis < 3; iAxis++) {
        m_mins[iAxis] -= 1.0f;
        m_maxs[iAxis] += 1.0f;
    }
}

bool CDispCollTree::IsCacheGenerated() const
{
    return m_aTrisCache.size() == GetTriSize();
}

void CDispCollTree::EnsureCacheIsCreated()
{
    if (m_aTrisCache.size() == GetTriSize())
        return;

    // Alloc.
    //int nSize = sizeof( CDispCollTriCache ) * GetTriSize();
    int nTriCount = GetTriSize();
    m_aTrisCache = std::vector<CDispCollTriCache>(nTriCount);

    for (int iTri = 0; iTri < nTriCount; iTri++)
        Cache_Create(&m_aTris[iTri], iTri);

    // Clear temporary lookup table that was used by Cache_Create()
    g_DispCollPlaneIndexHash.clear();
}

void CDispCollTree::Uncache() {
    m_aTrisCache  = {};
    m_aEdgePlanes = {};
}

bool CDispCollTree::AABBTree_Ray(SweptTrace* trace, bool bSide)
{
    // Check for ray test.
    if (CheckFlags(BspMap::DispInfo::FLAG_NO_RAY_COLL))
        return false;

    // Check for opacity.
    //if (!(m_nContents & MASK_OPAQUE))
    //    return false;

    CDispCollTri* pImpactTri = NULL;

    AABBTree_TreeTrisRayTest(trace, DISPCOLL_ROOTNODE_INDEX, bSide, &pImpactTri);

    if (pImpactTri) {
        // Collision.
        trace->results.plane_normal = pImpactTri->m_vecNormal;
        return true;
    }

    // No collision.
    return false;
}

void CDispCollTree::AABBTree_TreeTrisRayTest(SweptTrace* trace, int iNode,
    bool bSide, CDispCollTri** pImpactTri)
{
    rayleaflist_t list;
    // NOTE: This part is loop invariant - should be hoisted up as far as possible
    // ==== The following is the original source-sdk-2013 code that utilizes SIMD.
    ////list.invDelta.DuplicateVector(trace->info.invdelta);
    ////list.rayStart.DuplicateVector(trace->info.startpos);
    ////list.rayExtents.DuplicateVector(trace->info.extents + g_Vec3DispCollEpsilons);
    // ==== The following is the non-SIMD replacement of the code above.
    list.invDelta   = trace->info.invdelta;
    list.rayStart   = trace->info.startpos;
    list.rayExtents = trace->info.extents + g_Vec3DispCollEpsilons;
    // ==== end of replacement
    
    int listIndex = BuildRayLeafList(iNode, list);
    for (; listIndex <= list.maxIndex; listIndex++) {
        int leafIndex = list.nodeList[listIndex] - m_nodes.size();

        CDispCollTri* pTri0 = &m_aTris[m_leaves[leafIndex].m_tris[0]];
        CDispCollTri* pTri1 = &m_aTris[m_leaves[leafIndex].m_tris[1]];

        float flFrac = IntersectRayWithTriangle( *trace,
            m_aVerts[pTri0->GetVert(0)],
            m_aVerts[pTri0->GetVert(2)],
            m_aVerts[pTri0->GetVert(1)],
            bSide);
        if ((flFrac >= 0.0f) && (flFrac < trace->results.fraction)) {
            trace->results.fraction = flFrac;
            (*pImpactTri) = pTri0;
        }

        flFrac = IntersectRayWithTriangle(*trace,
            m_aVerts[pTri1->GetVert(0)],
            m_aVerts[pTri1->GetVert(2)],
            m_aVerts[pTri1->GetVert(1)],
            bSide);
        if ((flFrac >= 0.0f) && (flFrac < trace->results.fraction)) {
            trace->results.fraction = flFrac;
            (*pImpactTri) = pTri1;
        }
    }
}

bool CDispCollTree::AABBTree_IntersectAABB(
    const Vector3& absMins,
    const Vector3& absMaxs)
{
    // Check for hull test.
    if (CheckFlags(BspMap::DispInfo::FLAG_NO_HULL_COLL))
        return false;

    cplane_t plane;
    Vector3 center = 0.5f * (absMins + absMaxs);
    Vector3 extents = absMaxs - center;

    int nodeList[MAX_AABB_LIST];
    nodeList[0] = 0;
    int listIndex = 0;
    int maxIndex = 0;

    // NOTE: This part is loop invariant - should be hoisted up as far as possible
    // ==== The following is the original source-sdk-2013 code that utilizes SIMD.
    ////FourVectors mins0;
    ////mins0.DuplicateVector(absMins);
    ////FourVectors maxs0;
    ////maxs0.DuplicateVector(absMaxs);
    ////FourVectors rayExtents;
    // ==== The following is the non-SIMD replacement of the code above.
    Vector3 mins0 = absMins;
    Vector3 maxs0 = absMaxs;
    // ==== end of replacement

    while (listIndex <= maxIndex) {
        int iNode = nodeList[listIndex];
        listIndex++;
        // The rest are all leaves
        if (IsLeafNode(iNode)) {
            for (--listIndex; listIndex <= maxIndex; listIndex++) {
                int leafIndex = nodeList[listIndex] - m_nodes.size();
                CDispCollTri* pTri0 = &m_aTris[m_leaves[leafIndex].m_tris[0]];
                CDispCollTri* pTri1 = &m_aTris[m_leaves[leafIndex].m_tris[1]];

                plane.normal   = pTri0->m_vecNormal;
                plane.dist     = pTri0->m_flDist;
                plane.signbits = pTri0->m_ucSignBits;
                plane.type     = pTri0->m_ucPlaneType;

                if (IsBoxIntersectingTriangle(center, extents,
                    m_aVerts[pTri0->GetVert(0)],
                    m_aVerts[pTri0->GetVert(2)],
                    m_aVerts[pTri0->GetVert(1)],
                    plane, 0.0f))
                    return true;
                plane.normal   = pTri1->m_vecNormal;
                plane.dist     = pTri1->m_flDist;
                plane.signbits = pTri1->m_ucSignBits;
                plane.type     = pTri1->m_ucPlaneType;

                if (IsBoxIntersectingTriangle(center, extents,
                    m_aVerts[pTri1->GetVert(0)],
                    m_aVerts[pTri1->GetVert(2)],
                    m_aVerts[pTri1->GetVert(1)],
                    plane, 0.0f))
                    return true;
            }
            break;
        }
        else
        {
            const CDispCollNode& node = m_nodes[iNode];
            int mask =
                IntersectFourBoxPairs(mins0, maxs0, node.m_mins, node.m_maxs);
            if (mask) {
                int child = Nodes_GetChild(iNode, 0);
                if (mask & 1) {
                    maxIndex++;
                    nodeList[maxIndex] = child;
                }
                if (mask & 2) {
                    maxIndex++;
                    nodeList[maxIndex] = child + 1;
                }
                if (mask & 4) {
                    maxIndex++;
                    nodeList[maxIndex] = child + 2;
                }
                if (mask & 8) {
                    maxIndex++;
                    nodeList[maxIndex] = child + 3;
                }
                assert(maxIndex < MAX_AABB_LIST);
            }
        }
    }
    return false;  // No collision
}

bool CDispCollTree::AABBTree_SweepAABB(SweptTrace* trace)
{
    // Check for hull test.
    if (CheckFlags(BspMap::DispInfo::FLAG_NO_HULL_COLL))
        return false;

    // Save fraction.
    float flFrac = trace->results.fraction;

    // Test ray against the triangles in the list.
    rayleaflist_t list;
    // NOTE: This part is loop invariant - should be hoisted up as far as possible
    // ==== The following is the original source-sdk-2013 code that utilizes SIMD.
    ////list.invDelta.DuplicateVector(trace->info.invdelta);
    ////list.rayStart.DuplicateVector(trace->info.startpos);
    ////list.rayExtents.DuplicateVector(trace->info.extents + g_Vec3DispCollEpsilons);
    // ==== The following is the non-SIMD replacement of the code above.
    list.invDelta   = trace->info.invdelta;
    list.rayStart   = trace->info.startpos;
    list.rayExtents = trace->info.extents + g_Vec3DispCollEpsilons;
    // ==== end of replacement

    int listIndex = BuildRayLeafList(0, list);

    if (listIndex <= list.maxIndex) {
        EnsureCacheIsCreated();
        for (; listIndex <= list.maxIndex; listIndex++) {
            int leafIndex = list.nodeList[listIndex] - m_nodes.size();
            int iTri0 = m_leaves[leafIndex].m_tris[0];
            int iTri1 = m_leaves[leafIndex].m_tris[1];
            CDispCollTri* pTri0 = &m_aTris[iTri0];
            CDispCollTri* pTri1 = &m_aTris[iTri1];

            coll::Debugger::DebugStart_DispCollLeafHit(*this, leafIndex);
            SweepAABBTriIntersect(trace, iTri0, pTri0);
            SweepAABBTriIntersect(trace, iTri1, pTri1);
            coll::Debugger::DebugFinish_DispCollLeafHit();
        }
    }

    // Collision.
    if (trace->results.fraction < flFrac)
        return true;

    // No collision.
    return false;
}

bool CDispCollTree::ResolveRayPlaneIntersect(float flStart, float flEnd,
    const Vector3& vecNormal, float flDist, CDispCollHelper* pHelper)
{
    if ((flStart > 0.0f) && (flEnd > 0.0f)) return false;
    if ((flStart < 0.0f) && (flEnd < 0.0f)) return true;

    float flDenom = flStart - flEnd;
    bool bDenomIsZero = (flDenom == 0.0f);
    if ((flStart >= 0.0f) && (flEnd <= 0.0f)) {
        // Find t - the parametric distance along the trace line.
        float t = (!bDenomIsZero) ? (flStart - DISPCOLL_DIST_EPSILON) / flDenom : 0.0f;
        if (t > pHelper->m_flStartFrac) {
            pHelper->m_flStartFrac = t;
            pHelper->m_vecImpactNormal = vecNormal;
            pHelper->m_flImpactDist = flDist;
        }
    } else {
        // Find t - the parametric distance along the trace line.
        float t = (!bDenomIsZero) ? (flStart + DISPCOLL_DIST_EPSILON) / flDenom : 0.0f;
        if (t < pHelper->m_flEndFrac) {
            pHelper->m_flEndFrac = t;
        }
    }
    return true;
}

inline bool CDispCollTree::FacePlane(const SweptTrace& trace, CDispCollTri* pTri,
    CDispCollHelper* pHelper)
{
    // Calculate the closest point on box to plane (get extents in that direction).
    Vector3 vecExtent;
    CalcClosestExtents(pTri->m_vecNormal, trace.info.extents, vecExtent);

    float flExpandDist =
          pTri->m_flDist
        - Math::dot(pTri->m_vecNormal, vecExtent);
    float flStart =
          Math::dot(pTri->m_vecNormal, trace.info.startpos)
        - flExpandDist;
    float flEnd =
          Math::dot(pTri->m_vecNormal, (trace.info.startpos + trace.info.delta))
        - flExpandDist;

    return ResolveRayPlaneIntersect(flStart, flEnd, pTri->m_vecNormal,
        pTri->m_flDist, pHelper);
}

bool FORCEINLINE CDispCollTree::AxisPlanesXYZ(const SweptTrace& trace,
    CDispCollTri* pTri, CDispCollHelper* pHelper)
{
    static const Vector3 g_ImpactNormalVecs[2][3] = {
        {
            { -1,  0,  0 },
            {  0, -1,  0 },
            {  0,  0, -1 },
        },
        {
            {  1,  0,  0 },
            {  0,  1,  0 },
            {  0,  0,  1 },
        }
    };

    float flDist, flExpDist, flStart, flEnd;
    for (int iAxis = 2; iAxis >= 0; iAxis--) {
        const float rayStart  = trace.info.startpos[iAxis];
        const float rayExtent = trace.info.extents [iAxis];
        const float rayDelta  = trace.info.delta   [iAxis];

        // Min
        flDist = m_aVerts[pTri->GetVert(pTri->GetMin(iAxis))][iAxis];
        flExpDist = flDist - rayExtent;
        flStart = flExpDist - rayStart;
        flEnd = flStart - rayDelta;

        if (!ResolveRayPlaneIntersect(flStart, flEnd,
                g_ImpactNormalVecs[0][iAxis], flDist, pHelper))
            return false;

        // Max
        flDist = m_aVerts[pTri->GetVert(pTri->GetMax(iAxis))][iAxis];
        flExpDist = flDist + rayExtent;
        flStart = rayStart - flExpDist;
        flEnd = flStart + rayDelta;

        if (!ResolveRayPlaneIntersect(flStart, flEnd,
                g_ImpactNormalVecs[1][iAxis], flDist, pHelper))
            return false;
    }
    return true;
}

void CDispCollTree::Cache_Create(CDispCollTri* pTri, int iTri)
{
    Vector3* pVerts[3];
    pVerts[0] = &m_aVerts[pTri->GetVert(0)];
    pVerts[1] = &m_aVerts[pTri->GetVert(1)];
    pVerts[2] = &m_aVerts[pTri->GetVert(2)];

    CDispCollTriCache* pCache = &m_aTrisCache[iTri];
    Vector3 vecEdge;

    // Edge 1
    vecEdge = *pVerts[1] - *pVerts[0];
    Cache_EdgeCrossAxisX(vecEdge, *pVerts[0], *pVerts[2], pTri, pCache->m_iCrossX[0]);
    Cache_EdgeCrossAxisY(vecEdge, *pVerts[0], *pVerts[2], pTri, pCache->m_iCrossY[0]);
    Cache_EdgeCrossAxisZ(vecEdge, *pVerts[0], *pVerts[2], pTri, pCache->m_iCrossZ[0]);
    // Edge 2
    vecEdge = *pVerts[2] - * pVerts[1];
    Cache_EdgeCrossAxisX(vecEdge, *pVerts[1], *pVerts[0], pTri, pCache->m_iCrossX[1]);
    Cache_EdgeCrossAxisY(vecEdge, *pVerts[1], *pVerts[0], pTri, pCache->m_iCrossY[1]);
    Cache_EdgeCrossAxisZ(vecEdge, *pVerts[1], *pVerts[0], pTri, pCache->m_iCrossZ[1]);
    // Edge 3
    vecEdge = *pVerts[0] - * pVerts[2];
    Cache_EdgeCrossAxisX(vecEdge, *pVerts[2], *pVerts[1], pTri, pCache->m_iCrossX[2]);
    Cache_EdgeCrossAxisY(vecEdge, *pVerts[2], *pVerts[1], pTri, pCache->m_iCrossY[2]);
    Cache_EdgeCrossAxisZ(vecEdge, *pVerts[2], *pVerts[1], pTri, pCache->m_iCrossZ[2]);
}

int CDispCollTree::AddPlane(const Vector3& vecNormal)
{
    DispCollPlaneIndex_t planeIndex;

    planeIndex.vecPlane = vecNormal;
    planeIndex.index = m_aEdgePlanes.size();

    auto insert_result = g_DispCollPlaneIndexHash.insert(planeIndex);
    bool bDidInsert = insert_result.second;

    if (!bDidInsert) {
        const DispCollPlaneIndex_t& existingEntry = *insert_result.first;
        if (SourceSdkVectorEqual(existingEntry.vecPlane, vecNormal))
            return existingEntry.index;
        else
            return (existingEntry.index | 0x8000);
    }

    int index = m_aEdgePlanes.size();
    m_aEdgePlanes.push_back(vecNormal);
    return index;
}

// NOTE: The plane distance get stored in the normal x position since it isn't
//       used.
bool CDispCollTree::Cache_EdgeCrossAxisX(const Vector3& vecEdge,
    const Vector3& vecOnEdge, const Vector3& vecOffEdge, CDispCollTri* pTri,
    unsigned short& iPlane)
{
    // Calculate the normal: edge x axisX = ( 0.0, edgeZ, -edgeY )
    Vector3 vecNormal{ 0.0f, vecEdge.z(), -vecEdge.y() };
    NormalizeInPlace(vecNormal);

    // Check for zero length normals.
    if ((vecNormal.y() == 0.0f) || (vecNormal.z() == 0.0f)) {
        iPlane = DISPCOLL_NORMAL_UNDEF;
        return false;
    }
    
    //if ( Math::dot(pTri->m_vecNormal, vecNormal) )
    //{
    //    iPlane = DISPCOLL_NORMAL_UNDEF;
    //    return false;
    //}

    // Finish the plane definition - get distance.
    float flDist =
        (vecNormal.y() * vecOnEdge.y()) +
        (vecNormal.z() * vecOnEdge.z());

    // Special case the point off edge in plane
    float flOffDist =
        (vecNormal.y() * vecOffEdge.y()) +
        (vecNormal.z() * vecOffEdge.z());

    if (!(Math::abs(flOffDist - flDist) < DISPCOLL_DIST_EPSILON)
        && (flOffDist > flDist)) {
        // Adjust plane facing - triangle should be behind the plane.
        vecNormal.x() = -flDist;
        vecNormal.y() = -vecNormal.y();
        vecNormal.z() = -vecNormal.z();
    } else {
        vecNormal.x() = flDist;
    }

    // Add edge plane to edge plane list.
    iPlane = static_cast<unsigned short>(AddPlane(vecNormal));
    // Created the cached edge.
    return true;
}

// NOTE: The plane distance get stored in the normal y position since it isn't
//       used.
bool CDispCollTree::Cache_EdgeCrossAxisY(const Vector3& vecEdge,
    const Vector3& vecOnEdge, const Vector3& vecOffEdge, CDispCollTri* pTri,
    unsigned short& iPlane)
{
    // Calculate the normal: edge x axisY = ( -edgeZ, 0.0, edgeX )
    Vector3 vecNormal{ -vecEdge.z(), 0.0f, vecEdge.x() };
    NormalizeInPlace(vecNormal);

    // Check for zero length normals
    if ((vecNormal.x() == 0.0f) || (vecNormal.z() == 0.0f)) {
        iPlane = DISPCOLL_NORMAL_UNDEF;
        return false;
    }

    //if ( Math::dot(pTri->m_vecNormal, vecNormal) )
    //{
    //    iPlane = DISPCOLL_NORMAL_UNDEF;
    //    return false;
    //}

    // Finish the plane definition - get distance.
    float flDist =
        (vecNormal.x() * vecOnEdge.x()) +
        (vecNormal.z() * vecOnEdge.z());

    // Special case the point off edge in plane
    float flOffDist =
        (vecNormal.x() * vecOffEdge.x()) +
        (vecNormal.z() * vecOffEdge.z());

    if (!(Math::abs(flOffDist - flDist) < DISPCOLL_DIST_EPSILON)
        && (flOffDist > flDist)) {
        // Adjust plane facing if necessay - triangle should be behind the plane.
        vecNormal.x() = -vecNormal.x();
        vecNormal.y() = -flDist;
        vecNormal.z() = -vecNormal.z();
    } else {
        vecNormal.y() = flDist;
    }

    // Add edge plane to edge plane list.
    iPlane = static_cast<unsigned short>(AddPlane(vecNormal));
    // Created the cached edge.
    return true;
}

bool CDispCollTree::Cache_EdgeCrossAxisZ(const Vector3& vecEdge,
    const Vector3& vecOnEdge, const Vector3& vecOffEdge, CDispCollTri* pTri,
    unsigned short& iPlane)
{
    // Calculate the normal: edge x axisZ = ( edgeY, -edgeX, 0.0 )
    Vector3 vecNormal{ vecEdge.y(), -vecEdge.x(), 0.0f };
    NormalizeInPlace(vecNormal);

    // Check for zero length normals
    if ((vecNormal.x() == 0.0f) || (vecNormal.y() == 0.0f)) {
        iPlane = DISPCOLL_NORMAL_UNDEF;
        return false;
    }

    //if ( Math::dot(pTri->m_vecNormal, vecNormal) )
    //{
    //    iPlane = DISPCOLL_NORMAL_UNDEF;
    //    return false;
    //}

    // Finish the plane definition - get distance.
    float flDist =
        (vecNormal.x() * vecOnEdge.x()) +
        (vecNormal.y() * vecOnEdge.y());

    // Special case the point off edge in plane
    float flOffDist =
        (vecNormal.x() * vecOffEdge.x()) +
        (vecNormal.y() * vecOffEdge.y());

    if (!(Math::abs(flOffDist - flDist) < DISPCOLL_DIST_EPSILON)
        && (flOffDist > flDist)) {
        // Adjust plane facing if necessay - triangle should be behind the plane.
        vecNormal.x() = -vecNormal.x();
        vecNormal.y() = -vecNormal.y();
        vecNormal.z() = -flDist;
    } else {
        vecNormal.z() = flDist;
    }

    // Add edge plane to edge plane list.
    iPlane = static_cast<unsigned short>(AddPlane(vecNormal));
    // Created the cached edge.
    return true;
}

template <int AXIS>
bool CDispCollTree::EdgeCrossAxis(const SweptTrace& trace, unsigned short iPlane,
    CDispCollHelper* pHelper)
{
    if (iPlane == DISPCOLL_NORMAL_UNDEF)
        return true;

    // Get the edge plane.
    Vector3 vecNormal;
    if ((iPlane & 0x8000) != 0) {
        vecNormal = m_aEdgePlanes[(iPlane & 0x7fff)];
        vecNormal = -vecNormal;
    } else {
        vecNormal = m_aEdgePlanes[iPlane];
    }

    const int OTHER_AXIS1 = (AXIS + 1) % 3;
    const int OTHER_AXIS2 = (AXIS + 2) % 3;

    // Get the pland distance are "fix" the normal.
    float flDist = vecNormal[AXIS];
    vecNormal[AXIS] = 0.0f;

    // Calculate the closest point on box to plane (get extents in that direction).
    Vector3 vecExtent;
    //vecExtent[AXIS] = 0.0f;
    if (vecNormal[OTHER_AXIS1] < 0.0f) vecExtent[OTHER_AXIS1] = +trace.info.extents[OTHER_AXIS1];
    else                               vecExtent[OTHER_AXIS1] = -trace.info.extents[OTHER_AXIS1];
    if (vecNormal[OTHER_AXIS2] < 0.0f) vecExtent[OTHER_AXIS2] = +trace.info.extents[OTHER_AXIS2];
    else                               vecExtent[OTHER_AXIS2] = -trace.info.extents[OTHER_AXIS2];

    // Expand the plane by the extents of the box to reduce the swept
    // box/triangle test to a ray/extruded triangle test (one of the triangles
    // extruded planes was just calculated above).
    Vector3 vecEnd;
    vecEnd[AXIS] = 0;
    vecEnd[OTHER_AXIS1] = trace.info.startpos[OTHER_AXIS1] + trace.info.delta[OTHER_AXIS1];
    vecEnd[OTHER_AXIS2] = trace.info.startpos[OTHER_AXIS2] + trace.info.delta[OTHER_AXIS2];

    float flExpandDist =  flDist -
                          (vecNormal[OTHER_AXIS1] * vecExtent[OTHER_AXIS1] +
                           vecNormal[OTHER_AXIS2] * vecExtent[OTHER_AXIS2]);
    float flStart = (vecNormal[OTHER_AXIS1] * trace.info.startpos[OTHER_AXIS1] +
                     vecNormal[OTHER_AXIS2] * trace.info.startpos[OTHER_AXIS2])
                    - flExpandDist;
    float flEnd = (vecNormal[OTHER_AXIS1] * vecEnd[OTHER_AXIS1] +
                   vecNormal[OTHER_AXIS2] * vecEnd[OTHER_AXIS2])
                  - flExpandDist;
    return ResolveRayPlaneIntersect(flStart, flEnd, vecNormal, flDist, pHelper);
}

inline bool CDispCollTree::EdgeCrossAxisX(const SweptTrace& trace,
    unsigned short iPlane, CDispCollHelper* pHelper)
{
    return EdgeCrossAxis<0>(trace, iPlane, pHelper);
}

inline bool CDispCollTree::EdgeCrossAxisY(const SweptTrace& trace,
    unsigned short iPlane, CDispCollHelper* pHelper)
{
    return EdgeCrossAxis<1>(trace, iPlane, pHelper);
}

inline bool CDispCollTree::EdgeCrossAxisZ(const SweptTrace& trace,
    unsigned short iPlane, CDispCollHelper* pHelper)
{
    return EdgeCrossAxis<2>(trace, iPlane, pHelper);
}

void CDispCollTree::SweepAABBTriIntersect(SweptTrace* trace, int iTri,
    CDispCollTri* pTri)
{
    // Init test data.
    CDispCollHelper helper;
    helper.m_flEndFrac = 1.0f;
    helper.m_flStartFrac = DISPCOLL_INVALID_FRAC;

    // Make sure objects are traveling toward one another.
    float flDistAlongNormal = Math::dot(pTri->m_vecNormal, trace->info.delta);
    if (flDistAlongNormal > DISPCOLL_DIST_EPSILON)
        return;

    // Test against the axis planes.
    if (!AxisPlanesXYZ(*trace, pTri, &helper))
        return;

    // There are 9 edge tests - edges 1, 2, 3 cross with the box edge
    // (symmetry) 1, 2, 3.
    // However, the box is axis-aligned resulting in axially directional edges
    // -- thus each test is edges 1, 2, and 3 vs. axial planes x, y, and z.
    //
    // There are potentially 9 more tests with edges, the edge's edges and the
    // direction of motion!
    // NOTE: I don't think these tests are necessary for a manifold surface.
    
    CDispCollTriCache* pCache = &m_aTrisCache[iTri];

    // Edges 1-3, interleaved - axis tests are 2d tests
    if (!EdgeCrossAxisX(*trace, pCache->m_iCrossX[0], &helper)) return;
    if (!EdgeCrossAxisX(*trace, pCache->m_iCrossX[1], &helper)) return;
    if (!EdgeCrossAxisX(*trace, pCache->m_iCrossX[2], &helper)) return;

    if (!EdgeCrossAxisY(*trace, pCache->m_iCrossY[0], &helper)) return;
    if (!EdgeCrossAxisY(*trace, pCache->m_iCrossY[1], &helper)) return;
    if (!EdgeCrossAxisY(*trace, pCache->m_iCrossY[2], &helper)) return;

    if (!EdgeCrossAxisZ(*trace, pCache->m_iCrossZ[0], &helper)) return;
    if (!EdgeCrossAxisZ(*trace, pCache->m_iCrossZ[1], &helper)) return;
    if (!EdgeCrossAxisZ(*trace, pCache->m_iCrossZ[2], &helper)) return;

    // Test against the triangle face plane.
    if (!FacePlane(*trace, pTri, &helper))
        return;

    if ((helper.m_flStartFrac < helper.m_flEndFrac) ||
        (Math::abs(helper.m_flStartFrac - helper.m_flEndFrac) < 0.001f))
    {
        if ((helper.m_flStartFrac != DISPCOLL_INVALID_FRAC) &&
            (helper.m_flStartFrac < trace->results.fraction))
        {
            // Clamp -- shouldn't really ever be here!???
            if (helper.m_flStartFrac < 0.0f)
                helper.m_flStartFrac = 0.0f;
            trace->results.fraction = helper.m_flStartFrac;
            trace->results.plane_normal = helper.m_vecImpactNormal;
        }
    }
}

CDispCollTree::CDispCollTree(size_t disp_info_idx, const BspMap& bsp_map)
{
    ZoneScoped;

    const BspMap::DispInfo& disp = bsp_map.dispinfos[disp_info_idx];

    // Copy the flags.
    m_nFlags = disp.flags & ~0x80000000; // Clear the most significant bit

    // Displacement size.
    m_nPower = disp.power;

    // Get vertices. They must be in the same order as they are found in the
    // BSP map file's DISP_VERTS lump.
    std::vector<Vector3> vertices =
        bsp_map.GetDisplacementVertices(disp_info_idx);

    // Create the AABB Tree.
    AABBTree_Create(vertices);
}
// --------- end of source-sdk-2013 code ---------
