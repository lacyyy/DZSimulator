#ifndef UTILS_3D_H_
#define UTILS_3D_H_

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Matrix4.h>

namespace utils_3d {

    // Returns the normalized vector.
    // Zero vectors are normalized to zero vectors.
    Magnum::Vector3 GetNormalized(const Magnum::Vector3& vec);

    // Normalizes the vector in place and returns its original length.
    // Zero vectors are normalized to zero vectors.
    float NormalizeInPlace(Magnum::Vector3& vec);

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

    // Calculates direction vectors depending on viewing angles
    void AngleVectors(const Magnum::Vector3& angles,
        Magnum::Vector3* forward = nullptr,
        Magnum::Vector3* right   = nullptr,
        Magnum::Vector3* up      = nullptr);

}

#endif // UTILS_3D_H_
