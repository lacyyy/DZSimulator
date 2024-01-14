#ifndef REN_WIDELINERENDERER_H_
#define REN_WIDELINERENDERER_H_

#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Shaders/LineGL.h>

#ifdef DZSIM_WEB_PORT
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif

namespace ren {
    class WideLineRenderer {
    public:
#ifdef DZSIM_WEB_PORT
        typedef Magnum::Platform::EmscriptenApplication Application;
#else
        typedef Magnum::Platform::Sdl2Application Application;
#endif

    public:
        void InitWithOpenGLContext(); // Call this after an OpenGL context was created

        void HandleViewportEvent(const Application::ViewportEvent& event);

        void DrawAABB(
            const Magnum::Color4& aabb_col,
            const Magnum::Vector3& aabb_center_pos,
            const Magnum::Vector3& aabb_extents,
            const Magnum::Matrix4& view_proj_transformation,
            const Magnum::Vector3& cam_pos,
            const Magnum::Vector3& cam_dir_normal,
            bool no_depth_test = false);

        void DrawLine(
            const Magnum::Color4& color,
            Magnum::Vector3 start,
            Magnum::Vector3 end,
            const Magnum::Matrix4& view_proj_transformation,
            const Magnum::Vector3& cam_pos,
            const Magnum::Vector3& cam_dir_normal,
            bool no_depth_test = false
        );

        // Usable for visualizing normals. ind_normal must be normalized
        void DrawDirectionIndicator(
            const Magnum::Color4& color,
            const Magnum::Vector3 ind_pos,
            const Magnum::Vector3 ind_normal,
            const Magnum::Matrix4& view_proj_transformation,
            const Magnum::Vector3& cam_pos,
            const Magnum::Vector3& cam_dir_normal,
            bool no_depth_test = false
        );

    private:
        Magnum::Shaders::LineGL3D _shader_cap_style_butt { Magnum::NoCreate };
        Magnum::Shaders::LineGL3D _shader_cap_style_round{ Magnum::NoCreate };
        Magnum::GL::Mesh _x_line_mesh{ Magnum::NoCreate };
        Magnum::GL::Mesh _y_line_mesh{ Magnum::NoCreate };
        Magnum::GL::Mesh _z_line_mesh{ Magnum::NoCreate };
    };

} // namespace ren

#endif // REN_WIDELINERENDERER_H_
