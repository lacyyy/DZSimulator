#include "utils_3d.h"

#include <cassert>
#include <cfloat>

#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;


Vector3 utils_3d::GetNormalized(const Vector3& vec)
{
    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/public/mathlib/vector.h)
    // (Original code found in _VMX_VectorNormalize() function)

    // Note: Zero vectors are normalized to zero vectors.
    //       -> Need an epsilon here, otherwise NANs could appear.
    float invLength = 1.0f / (vec.length() + FLT_EPSILON);
    return vec * invLength;
    // --------- end of source-sdk-2013 code ---------
}

float utils_3d::NormalizeInPlace(Vector3& vec)
{
    // -------- start of source-sdk-2013 code --------
    // (taken and modified from source-sdk-2013/<...>/src/public/mathlib/vector.h)
    // (Original code found in _VMX_VectorNormalize() function)

    // Note: Zero vectors are normalized to zero vectors.
    //       -> Need an epsilon here, otherwise NANs could appear.
    float length = vec.length();
    float invLength = 1.0f / (length + FLT_EPSILON);
    vec = vec * invLength;
    return length;
    // --------- end of source-sdk-2013 code ---------
}

Vector3 utils_3d::GetVectorPerpendicularToNormal(const Vector3& normal)
{
    assert(normal.length() < 1.2f);
    assert(normal.length() > 0.8f);

    // Step 1: Create a vector that's linearly independent to the input normal.
    //         Approach: Add 5 to the smallest vector component.
    //         I can't prove this produces a linearly independent vector, but it
    //         seems so.
    int smallest_idx = 0;
    if (Math::abs(normal[1]) < Math::abs(normal[smallest_idx])) smallest_idx = 1;
    if (Math::abs(normal[2]) < Math::abs(normal[smallest_idx])) smallest_idx = 2;

    Vector3 linearly_independent_vec = normal;
    linearly_independent_vec[smallest_idx] += 5.0f;

    // Step 2: Create a vector perpendicular to the input normal.
    Vector3 perp_vec = Math::cross(normal, linearly_independent_vec);

    if (perp_vec.isZero()) { // Shouldn't happen
        assert(0);
        return { 0.0f, 0.0f, 0.0f };
    }
    return perp_vec;
}

Matrix4 utils_3d::CalcModelTransformationMatrix(
    const Vector3& obj_pos, const Vector3& obj_ang, const Vector3& obj_scales)
{
    // Order of transformations is important!
    //   Step 1: scaling
    //   Step 2: rotation around X axis (this is roll)
    //   Step 3: rotation around Y axis (this is pitch)
    //   Step 4: rotation around Z axis (this is yaw)
    //   Step 5: translation

    Matrix4 model_transformation =
        Matrix4::translation(obj_pos) *
        Matrix4::rotationZ(Deg{ obj_ang[1] }) * // yaw
        Matrix4::rotationY(Deg{ obj_ang[0] }) * // pitch
        Matrix4::rotationX(Deg{ obj_ang[2] }) * // roll
        Matrix4::scaling(obj_scales);
    // @Precision Perform scaling after rotations?
    return model_transformation;
}

Matrix4 utils_3d::CalcModelTransformationMatrix(
    const Vector3& obj_pos, const Vector3& obj_ang, float uniform_scale)
{
    return CalcModelTransformationMatrix(obj_pos, obj_ang,
        Vector3{ uniform_scale, uniform_scale, uniform_scale });
}

Quaternion utils_3d::CalcQuaternion(const Vector3& obj_ang) {
    // @Optimization Are there faster algorithms for euler angle to quaternion conversion?
    Matrix4 rotation_4x4 =
        Matrix4::rotationZ(Deg{ obj_ang[1] }) * // yaw
        Matrix4::rotationY(Deg{ obj_ang[0] }) * // pitch
        Matrix4::rotationX(Deg{ obj_ang[2] });  // roll

    Matrix3 rotation_3x3 = rotation_4x4.rotationScaling(); // Get upper-left 3x3 part
    return Quaternion::fromMatrix(rotation_3x3);
}

Vector3 utils_3d::CalcNormalCwFront(const Vector3& v1, const Vector3& v2,
    const Vector3& v3)
{
    // Here we assume the cross product will never be the zero vector!
    return Math::cross(v3 - v1, v2 - v1).normalized();
}

// Returns true if the normal vector of the triangle described by three vertices
// in clockwise direction has a positive Z component
bool utils_3d::IsCwTriangleFacingUp(
    const Vector3& v1,
    const Vector3& v2,
    const Vector3& v3)
{
    Vector3 v1_to_v3 = v3 - v1;
    Vector3 v1_to_v2 = v2 - v1;
    float normal_z_component = v1_to_v2.y() * v1_to_v3.x() - v1_to_v2.x() * v1_to_v3.y();
    return normal_z_component > 0.0f;
}

// -------- start of source-sdk-2013 code --------
// (taken and modified from source-sdk-2013/<...>/src/mathlib/mathlib_base.cpp)

// Euler QAngle -> Basis Vectors.  Each vector is optional
void utils_3d::AnglesToVectors(const Vector3& angles,
    Vector3* forward, Vector3* right, Vector3* up)
{
    // The following code was originally taken from:
    //     void AngleVectors( const QAngle &angles,
    //                        Vector *forward, Vector *right, Vector *up )

    auto pitch_sincos = Math::sincos(Deg{ angles[0] });
    auto   yaw_sincos = Math::sincos(Deg{ angles[1] });
    auto  roll_sincos = Math::sincos(Deg{ angles[2] });

    float sy = yaw_sincos.first();
    float cy = yaw_sincos.second();
    float sr = roll_sincos.first();
    float cr = roll_sincos.second();
    float sp = pitch_sincos.first();
    float cp = pitch_sincos.second();

    if (forward)
    {
        *forward = {
            cp * cy,
            cp * sy,
            -sp
        };
    }

    if (right)
    {
        *right = {
            (-1 * sr * sp * cy + -1 * cr * -sy),
            (-1 * sr * sp * sy + -1 * cr * cy),
            -1 * sr * cp
        };
    }

    if (up)
    {
        *up = {
            (cr * sp * cy + -sr * -sy),
            (cr * sp * sy + -sr * cy),
            cr * cp
        };
    }
}


Vector3 utils_3d::VectorsToAngles(const Vector3& up,
                                  const Vector3& forward,
                                  const Vector3& left)
{
    // The following code was originally taken from:
    //     void MatrixAngles( const matrix3x4_t& matrix, float *angles )

    Rad pitch, yaw, roll;

    float xyDist = forward.xy().length();

    // enough here to get angles?
    if (xyDist > 0.001f)
    {
        // in our space, forward is the X axis
        yaw   = Rad{ atan2f( forward.y(), forward.x()) };
        pitch = Rad{ atan2f(-forward.z(), xyDist) };
        roll  = Rad{ atan2f(left.z(), up.z()) };
    }
    else // forward is mostly Z, gimbal lock-
    {
        // forward is mostly z, so use right for yaw
        yaw   = Rad{ atan2f(-left.x(), left.y()) };
        pitch = Rad{ atan2f(-forward.z(), xyDist) };

        // Assume no roll in this case as one degree of freedom has been lost
        // (i.e. yaw == roll)
        roll = 0.0_radf;
    }

    return {
        (float)Deg{ pitch },
        (float)Deg{ yaw },
        (float)Deg{ roll }
    };
}
// --------- end of source-sdk-2013 code ---------

void utils_3d::DebugTestProperties_TriMesh(const TriMesh& tri_mesh)
{
    // Are used vertex indices in bounds?
    for(const TriMesh::Edge& edge : tri_mesh.edges)
        if (edge.verts[0] >= tri_mesh.vertices.size() ||
            edge.verts[1] >= tri_mesh.vertices.size())
            Error{} << "TriMesh: Edge's vert idx is out of bounds!";
    for(const TriMesh::Tri& tri : tri_mesh.tris)
        if (tri.verts[0] >= tri_mesh.vertices.size() ||
            tri.verts[1] >= tri_mesh.vertices.size() ||
            tri.verts[2] >= tri_mesh.vertices.size())
            Error{} << "TriMesh: Triangle's vert idx is out of bounds!";

    // Are there duplicate vertices?
    for(size_t i = 0; i < tri_mesh.vertices.size(); i++)
        for(size_t j = i+1; j < tri_mesh.vertices.size(); j++)
            if (tri_mesh.vertices[i] == tri_mesh.vertices[j])
                Error{} << "TriMesh: Contains duplicate vertices!";

    // Are there duplicate edges?
    for(size_t i = 0; i < tri_mesh.edges.size(); i++) {
        for(size_t j = i+1; j < tri_mesh.edges.size(); j++) {
            auto& edge1 = tri_mesh.edges[i].verts;
            auto& edge2 = tri_mesh.edges[j].verts;
            bool same_edge = (edge1[0] == edge2[0] && edge1[1] == edge2[1]) ||
                             (edge1[0] == edge2[1] && edge1[1] == edge2[0]);
            if (same_edge) Error{} << "TriMesh: Contains duplicate edges!";
        }
    }

    // Are there duplicate triangles?
    for(size_t i = 0; i < tri_mesh.tris.size(); i++) {
        for(size_t j = i+1; j < tri_mesh.tris.size(); j++) {
            auto& t1 = tri_mesh.tris[i].verts;
            auto& t2 = tri_mesh.tris[j].verts;
            // Assuming both triangles have CW vertex winding order
            bool same_tri = (t1[0]==t2[0] && t1[1]==t2[1] && t1[2]==t2[2]) ||
                            (t1[0]==t2[1] && t1[1]==t2[2] && t1[2]==t2[0]) ||
                            (t1[0]==t2[2] && t1[1]==t2[0] && t1[2]==t2[1]);
            if (same_tri) Error{} << "TriMesh: Contains duplicate triangles!";
        }
    }

    // Are there redundant vertices that aren't referenced by any triangle?
    for (size_t i = 0; i < tri_mesh.vertices.size(); i++) {
        bool referenced = false;
        for(const TriMesh::Tri& tri : tri_mesh.tris) {
            if (tri.verts[0] == i || tri.verts[1] == i || tri.verts[2] == i) {
                referenced = true;
                break;
            }
        }
        if (!referenced)
            Error{} << "TriMesh: Contains vertex not referenced by any triangle!";
    }
}
