////////////////////
// This file contains unused code that demonstrates asynchronously updating the
// vertex buffer of a Magnum::GL::Mesh object. There is no use for this code
// right now and it should be noted that updating an entire buffer is slow.
// There are methods to only update small portions of the buffer, see:
// https://doc.magnum.graphics/magnum/classMagnum_1_1GL_1_1Buffer.html#GL-Buffer-data-mapping
////////////////////

#if 0


#include <memory>
#include <utility>
#include <vector>

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Utility/Assert.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Shaders/GenericGL.h>

#include "utils_3d.h"

using namespace Magnum;
using namespace utils_3d;

namespace _unused_VertBufUpdateDemo {

    // Demonstration buffer update arguments
    struct ColoringArgs {
        Color4 override_color; // Is used if alpha is non-zero
        bool use_ambient_darkening;
        Vector2 light_vec; // normalized
    };

    // Demonstration wrapper object handling a mesh whose colors can be updated
    class DynamicColoredMesh {
    public:
        struct Vertex {
            Vector3 position;
            Color4 color;
        };

        DynamicColoredMesh(
            GL::Mesh&& _mesh,
            GL::Buffer&& _vertex_buffer,
            std::vector<std::vector<Vector3>>&& _faces,
            size_t _num_vertices)
            : mesh{ std::move(_mesh) }
            , vertex_buffer{ std::move(_vertex_buffer) }
            , faces{ std::move(_faces) }
            , num_vertices{ _num_vertices }
        {}

        GL::Mesh mesh;

        // Stores position and color per vertex
        GL::Buffer vertex_buffer{ GL::Buffer::TargetHint::Array };

        // The original faces that make up the structure of the mesh and buffer
        const std::vector<std::vector<Vector3>> faces;

        // num_vertices = 3 * number of triangles
        size_t num_vertices;
    };


    // ---- VERTEX BUFFER UPDATE FUNCTION ----

    void UpdateDynamicColoredMesh(DynamicColoredMesh& dc_mesh,
        const ColoringArgs& col_args)
    {
        using Vertex = DynamicColoredMesh::Vertex;

        if (dc_mesh.num_vertices == 0)
            return;

        // Start updating the buffer contents
        Containers::ArrayView<Vertex> data = Containers::arrayCast<Vertex>(
            dc_mesh.vertex_buffer.map(
                0,
                dc_mesh.num_vertices * sizeof(Vertex),
                GL::Buffer::MapFlag::Write | GL::Buffer::MapFlag::InvalidateBuffer));
        CORRADE_INTERNAL_ASSERT(data);

        Vertex* current_vert = data.begin();

        bool is_col_constant = col_args.override_color.a() != 0.0f;

        // Turn faces into triangles
        for (const std::vector<Vector3>& face : dc_mesh.faces) {
            for (size_t tri = 0; tri < face.size() - 2; ++tri) {
                // individual normal calculation seems to be required, although
                // triangles all face in the same direction
                Vector3 normal =
                    CalcNormalCwFront(face[0], face[tri + 1], face[tri + 2]);

                // Calculate new individual color
                Color4 color;
                if (is_col_constant)
                    color = col_args.override_color;
                else
                    color = { 0.0f, 0.0f, 0.0f, 1.0f }; // placeholder color

                if (col_args.use_ambient_darkening) {
                    // Nonsense individual color calculation demonstration
                    color.r() *= 0.1f;
                    color.g() *= 0.2f;
                    color.b() *= 0.3f;
                }

                CORRADE_ASSERT(current_vert + 3 <= data.end(),
                    "Illegal vertex buffer access!", );

                current_vert->position = face[0];
                current_vert->color = color;
                current_vert++;

                current_vert->position = face[tri + 1];
                current_vert->color = color;
                current_vert++;

                current_vert->position = face[tri + 2];
                current_vert->color = color;
                current_vert++;
            }
        }

        CORRADE_ASSERT(current_vert == data.end(),
            "Vertex buffer has not been filled entirely!", );

        // Finish updating the buffer
        CORRADE_INTERNAL_ASSERT_OUTPUT(dc_mesh.vertex_buffer.unmap());
    }


    // ---- INITIALIZATION FUNCTION ----

    std::unique_ptr<DynamicColoredMesh> GenDynamicColoredMesh(
            std::vector<std::vector<Vector3>>&& faces, const ColoringArgs& col_args)
    {
        using Vertex = DynamicColoredMesh::Vertex;

        // Determine total vertex count for buffer initialization
        size_t num_vertices = 0;
        for (std::vector<Vector3>& face : faces)
            // Each face has n vertices and is split into n-2 triangles
            // Each triangle gets 3 vertices in the buffer
            num_vertices += 3 * (face.size() - 2);

        // Allocate vertex buffer
        GL::Buffer vertex_buf{ GL::Buffer::TargetHint::Array };
        vertex_buf.setData({ nullptr, num_vertices * sizeof(Vertex) },
            GL::BufferUsage::DynamicDraw); // StaticDraw is just as fast?!

        // Set up mesh object
        GL::Mesh mesh;
        mesh.setCount(vertex_buf.size() / sizeof(Vertex))
            .addVertexBuffer(
                vertex_buf, // Important, pass buffer by reference
                0,
                Shaders::GenericGL3D::Position{},
                Shaders::GenericGL3D::Color4{});

        auto dcm = std::make_unique<DynamicColoredMesh>(
            std::move(mesh),
            std::move(vertex_buf),
            std::move(faces),
            num_vertices
            );

        UpdateDynamicColoredMesh(*dcm, col_args);
        return dcm;
    }


} // namespace _unused_VertBufUpdateDemo

#endif
