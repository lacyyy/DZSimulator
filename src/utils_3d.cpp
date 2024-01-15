#include "utils_3d.h"

#include <cfloat>
#include <cmath>

#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Functions.h>
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

    float sy = yaw_sincos.first;
    float cy = yaw_sincos.second;
    float sr = roll_sincos.first;
    float cr = roll_sincos.second;
    float sp = pitch_sincos.first;
    float cp = pitch_sincos.second;

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
