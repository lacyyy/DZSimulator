#ifndef UTILS_3D_H_
#define UTILS_3D_H_

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Matrix4.h>

namespace utils_3d {

    // obj_ang is pitch, yaw, roll
    Magnum::Matrix4 CalcModelTransformationMatrix(const Magnum::Vector3& obj_pos,
        const Magnum::Vector3& obj_ang, float uniform_scale=1.0f);

    // Calculates normal of triangle described by 3 vertices in
    // clockwise direction
    Magnum::Vector3 CalcNormalCwFront (
        const Magnum::Vector3& v1,
        const Magnum::Vector3& v2,
        const Magnum::Vector3& v3);

    bool IsCwTriangleFacingUp(
        const Magnum::Vector3& v1,
        const Magnum::Vector3& v2,
        const Magnum::Vector3& v3);

}

#endif // UTILS_3D_H_
