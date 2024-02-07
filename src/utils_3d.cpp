#include "utils_3d.h"

#include <cfloat>

#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

using namespace Magnum;


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

Matrix4 utils_3d::CalcModelTransformationMatrix(
    const Vector3& obj_pos, const Vector3& obj_ang, float uniform_scale)
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
        Matrix4::scaling({ uniform_scale, uniform_scale, uniform_scale });
    return model_transformation;
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
void utils_3d::AngleVectors(const Vector3& angles,
    Vector3* forward, Vector3* right, Vector3* up)
{
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
