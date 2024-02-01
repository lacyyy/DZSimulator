#ifndef UTILS_3D_H_
#define UTILS_3D_H_

#include <cstdint>
#include <limits>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>

namespace utils_3d {
    using Vector3    = Magnum::Vector3;
    using Matrix4    = Magnum::Matrix4;
    using Quaternion = Magnum::Quaternion;

    // Returns the normalized vector.
    // Zero vectors are normalized to zero vectors.
    Vector3 GetNormalized(const Vector3& vec);

    // Normalizes the vector in place and returns its original length.
    // Zero vectors are normalized to zero vectors.
    float NormalizeInPlace(Vector3& vec);

    // obj_ang is pitch, yaw, roll
    Matrix4 CalcModelTransformationMatrix(const Vector3& obj_pos,
        const Vector3& obj_ang, float uniform_scale=1.0f);

    // obj_ang is pitch, yaw, roll
    Quaternion CalcQuaternion(const Vector3& obj_ang);

    // Calculates normal of triangle described by 3 vertices in
    // clockwise direction
    Vector3 CalcNormalCwFront(const Vector3& v1,
                              const Vector3& v2,
                              const Vector3& v3);

    bool IsCwTriangleFacingUp(const Vector3& v1,
                              const Vector3& v2,
                              const Vector3& v3);

    // Calculates direction vectors depending on viewing angles
    void AngleVectors(const Vector3& angles,
                      Vector3* forward = nullptr,
                      Vector3* right   = nullptr,
                      Vector3* up      = nullptr);


    // Generic triangle mesh
    struct TriMesh {
        using VertIdx = uint16_t; // Index into the vertices array
        struct Edge { VertIdx verts[2]; }; // Represented by 2 vertices
        struct Tri  { VertIdx verts[3]; }; // Represented by 3 vertices in CW winding order

        // @Optimization Could half floats be precise enough for vert positions?
        //               If yes, look up available half float intrinsics.
        //               https://github.com/Maratyszcza/FP16 ??
        // @Optimization Is it beneficial for collision performance when these
        //               3 dynamic arrays lie close to each other in memory?
        //               If yes, how can this be achieved during creation?
        std::vector<Vector3> vertices; // (See TriMesh creation func for duplicate-freeness guarantees)
        std::vector<Edge>    edges;    // (See TriMesh creation func for duplicate-freeness guarantees)
        std::vector<Tri>     tris;     // (See TriMesh creation func for duplicate-freeness guarantees)

        static constexpr size_t MAX_VERTICES = 1 + std::numeric_limits<VertIdx>::max();
    };

    void DebugTestProperties_TriMesh(const TriMesh& tri_mesh);

}

#endif // UTILS_3D_H_
