#include "utils_3d.h"

#include <cmath>

#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Vector3.h>

using namespace Magnum;

Magnum::Matrix4 utils_3d::CalcModelTransformationMatrix(
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

Vector3 utils_3d::CalcNormalCcwFront(const Vector3& v1, const Vector3& v2,
    const Vector3& v3)
{
    // Here we assume the cross product will never be the zero vector!
    return Math::cross(v3 - v2, v1 - v2).normalized();
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
    const Magnum::Vector3& v1,
    const Magnum::Vector3& v2,
    const Magnum::Vector3& v3)
{
    Vector3 v1_to_v3 = v3 - v1;
    Vector3 v1_to_v2 = v2 - v1;
    float normal_z_component = v1_to_v2.y() * v1_to_v3.x() - v1_to_v2.x() * v1_to_v3.y();
    return normal_z_component > 0.0f;
}
