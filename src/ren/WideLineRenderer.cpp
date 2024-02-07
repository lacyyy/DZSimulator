#include "ren/WideLineRenderer.h"

#include <cassert>
#include <cmath>

#include <Tracy.hpp>

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Distance.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Intersection.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector4.h>
#include <Magnum/MeshTools/CompileLines.h>
#include <Magnum/MeshTools/GenerateLines.h>
#include <Magnum/Shaders/Line.h>
#include <Magnum/Trade/MeshData.h>

#include "utils_3d.h"

using namespace ren;
using namespace Magnum;
using namespace utils_3d;


// FIXME:
//   Lines are not drawn correctly behind transparent stuff (water, player clips)
//   -> Most likely have to split all lines on the transparent surface!
//     -> Do we then have to draw split lines and transparent surfaces in an
//        ordered fashion (from furthest to closest)? Or not needed?


// Arguments 'start' and 'end' might be modified and/or swapped in order to
// avoid intersection with the camera plane. The argument 'cam_plane' has the
// format as provided by Magnum::Math::planeEquation() .
// Returns true if the entire line is behind the camera plane, false otherwise.
static bool ClipLineToCameraPlane(Vector3& start, Vector3& end, const Vector4& cam_plane) {
    Vector3 dir = end - start;
    Vector3 plane_normal = cam_plane.xyz();
    float   plane_dist   = -cam_plane.w();

    // Too small epsilons seem to cause shortened lines to be drawn imprecisely
    constexpr float EPSILON = 8.0f;

    bool start_behind_cam = Math::dot(start, plane_normal) - plane_dist < EPSILON;
    bool   end_behind_cam = Math::dot(end,   plane_normal) - plane_dist < EPSILON;

    if (start_behind_cam && end_behind_cam)
        return true; // The entire line is behind the camera plane

    if (start_behind_cam) {
        // Invert the line direction so we always look at the problem with
        // the start point in front of the camera
        Vector3 temp = start;
        start = end;
        end = temp;
        dir = -dir;
    }
    // At this point, start is at least EPSILON in front of the camera plane

    // Calculate plane that is extended out from the camera plane by EPSILON
    Vector4 cutoff_plane = { cam_plane.xyz(), cam_plane.w() - EPSILON };

    float t = Math::Intersection::planeLine(cutoff_plane, start, dir);

    // If line end point is behind the cutoff plane -> Shorten the line
    if (!Math::isNan(t) && t > 0.0f && t < 1.0f)
        end = start + t * dir;

    return false; // At least some part of the line is in front of the camera plane
}

void WideLineRenderer::InitWithOpenGLContext()
{
    ZoneScoped;

    // Delayed member construction here (not in constructor) because they
    // require a GL context
    _shader_cap_style_butt = Shaders::LineGL3D{
        Shaders::LineGL3D::Configuration{}.setCapStyle(Shaders::LineCapStyle::Butt)
    };
    _shader_cap_style_round = Shaders::LineGL3D{
        Shaders::LineGL3D::Configuration{}.setCapStyle(Shaders::LineCapStyle::Round)
    };
    
    Vector3 positions_x_line[] { {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} };
    Vector3 positions_y_line[] { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };
    Vector3 positions_z_line[] { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} };

    Trade::MeshData mesh_data_x_line{MeshPrimitive::LineStrip, {}, positions_x_line, {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, Containers::stridedArrayView(positions_x_line)}
    }};
    Trade::MeshData mesh_data_y_line{MeshPrimitive::LineStrip, {}, positions_y_line, {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, Containers::stridedArrayView(positions_y_line)}
    }};
    Trade::MeshData mesh_data_z_line{MeshPrimitive::LineStrip, {}, positions_z_line, {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, Containers::stridedArrayView(positions_z_line)}
    }};
    _x_line_mesh = MeshTools::compileLines(MeshTools::generateLines(mesh_data_x_line));
    _y_line_mesh = MeshTools::compileLines(MeshTools::generateLines(mesh_data_y_line));
    _z_line_mesh = MeshTools::compileLines(MeshTools::generateLines(mesh_data_z_line));
}

void WideLineRenderer::HandleViewportEvent(
    const Application::ViewportEvent& /*event*/)
{
    // ...
}

void WideLineRenderer::DrawAABB(
    const Color4& aabb_col, const Vector3& aabb_center_pos, const Vector3& aabb_extents,
    const Matrix4& view_proj_transformation,
    const Vector3& cam_pos, const Vector3& cam_dir_normal, bool no_depth_test)
{
    // Make sure these blending values are set before calling this function:
    //GL::Renderer::enable(GL::Renderer::Feature::Blending);
    //GL::Renderer::setBlendFunction(
    //    GL::Renderer::BlendFunction::One,
    //    GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    // Make lines draw through all geometry, or not
    if (no_depth_test) GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    else               GL::Renderer::enable (GL::Renderer::Feature::DepthTest);
    // Magnum's LineGL shader seems to not have the same vertex winding as us
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);

    // Disabling the depth mask (or disabling the depth test) would make the
    // corners of the drawn AABB look much better, but it also screws up depth
    // ordering of overlapping AABBs. I think AABBs look good enough as it is.
    //GL::Renderer::setDepthMask(false);

    // By default, on-screen line width is constant
    float screen_line_width = 0.0625f;

    // Adjust the line width depending on the AABB's size
    screen_line_width *= Math::clamp(aabb_extents.min(), 16.0f, 72.0f);

    // Adjust the line width depending on distance to camera
    float aabb_cam_dist = (cam_pos - aabb_center_pos).length();
    aabb_cam_dist = Math::max(aabb_cam_dist, 300.0f); // limit max line width
    screen_line_width /= aabb_cam_dist;

    // Adjust the line width depending on vertical screen height.
    // The reason for that is because CSGO has a fixed vertical FOV and these
    // lines are part of the world geometry and should grow with it.
    screen_line_width *= GL::defaultFramebuffer.viewport().size().y();

    Shaders::LineGL3D& shader = _shader_cap_style_butt;
    shader
        .setViewportSize(Vector2{ GL::defaultFramebuffer.viewport().size() })
        .setWidth(screen_line_width)
        .setColor(aabb_col)
        .setSmoothness(1.0f);

    // Lines don't draw correctly if one of their points is behind the camera.
    // -> Truncate lines that intersect the camera's plane

    // Calculate camera plane to detect if points are behind the camera
    Vector4 cam_plane = Math::planeEquation(cam_dir_normal, cam_pos);

    // axis: 0 (X axis), 1 (Y axis), 2 (Z axis)
    // length: Signed length in the positive direction of the selected axis
    auto draw_axial_line = [this, &shader, &view_proj_transformation, &cam_plane]
    (Vector3 start, int axis, float length) {
        Vector3 base_dir;
        if (axis == 0) base_dir = Vector3{ 1.0f, 0.0f, 0.0f };
        if (axis == 1) base_dir = Vector3{ 0.0f, 1.0f, 0.0f };
        if (axis == 2) base_dir = Vector3{ 0.0f, 0.0f, 1.0f };
        Vector3 end = start + base_dir * length;

        bool entirely_clipped = ClipLineToCameraPlane(start, end, cam_plane);
        if(entirely_clipped)
            return; // Unable to draw line without graphical issues

        // start and end point might have been swapped
        Vector3& clipped_min_pt = start[axis] < end[axis] ? start : end;
        float clipped_length = (end - start).length();
        shader.setTransformationProjectionMatrix(
            view_proj_transformation *
            Matrix4::translation(clipped_min_pt) *
            Matrix4::scaling(Vector3{ clipped_length })
        );
        if(axis == 0) shader.draw(_x_line_mesh);
        if(axis == 1) shader.draw(_y_line_mesh);
        if(axis == 2) shader.draw(_z_line_mesh);
    };

    Vector3 aabb_mins = aabb_center_pos - 0.5f * aabb_extents;
    Vector3 v0 = aabb_mins + Vector3{             0.0f,             0.0f,             0.0f };
    Vector3 v1 = aabb_mins + Vector3{ aabb_extents.x(),             0.0f,             0.0f };
    Vector3 v2 = aabb_mins + Vector3{             0.0f, aabb_extents.y(),             0.0f };
    Vector3 v3 = aabb_mins + Vector3{ aabb_extents.x(), aabb_extents.y(),             0.0f };
    Vector3 v4 = aabb_mins + Vector3{             0.0f,             0.0f, aabb_extents.z() };
    Vector3 v5 = aabb_mins + Vector3{ aabb_extents.x(),             0.0f, aabb_extents.z() };
    Vector3 v6 = aabb_mins + Vector3{             0.0f, aabb_extents.y(), aabb_extents.z() };
//  Vector3 v7 = aabb_mins + Vector3{ aabb_extents.x(), aabb_extents.y(), aabb_extents.z() };

    draw_axial_line(v0, 0, aabb_extents.x());
    draw_axial_line(v2, 0, aabb_extents.x());
    draw_axial_line(v4, 0, aabb_extents.x());
    draw_axial_line(v6, 0, aabb_extents.x());

    draw_axial_line(v0, 1, aabb_extents.y());
    draw_axial_line(v1, 1, aabb_extents.y());
    draw_axial_line(v4, 1, aabb_extents.y());
    draw_axial_line(v5, 1, aabb_extents.y());

    draw_axial_line(v0, 2, aabb_extents.z());
    draw_axial_line(v1, 2, aabb_extents.z());
    draw_axial_line(v2, 2, aabb_extents.z());
    draw_axial_line(v3, 2, aabb_extents.z());

    // Reset draw state
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setDepthMask(true);
}

void WideLineRenderer::DrawLine(
    const Color4& color, Vector3 start, Vector3 end,
    const Matrix4& view_proj_transformation,
    const Vector3& cam_pos, const Vector3& cam_dir_normal, bool no_depth_test)
{
    // Make sure these blending values are set before calling this function:
    //GL::Renderer::enable(GL::Renderer::Feature::Blending);
    //GL::Renderer::setBlendFunction(
    //    GL::Renderer::BlendFunction::One,
    //    GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    // Make lines draw through all geometry, or not
    if (no_depth_test) GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    else               GL::Renderer::enable (GL::Renderer::Feature::DepthTest);
    // Magnum's LineGL shader seems to not have the same vertex winding as us
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);

    // Disabling the depth mask (or disabling the depth test) would make
    // overlapping lines look much better, but it also screws up depth ordering
    // of overlapping lines and line-AABBs. I think lines look good enough as is.
    //GL::Renderer::setDepthMask(false);

    // By default, on-screen line width is constant
    float screen_line_width = 2.0f;

    // Adjust the line width depending on distance to camera
    float line_cam_dist = Math::Distance::lineSegmentPoint(start, end, cam_pos);
    if (Math::isNan(line_cam_dist))
        line_cam_dist = 0.0f;

    line_cam_dist = Math::max(line_cam_dist, 300.0f); // limit max line width
    screen_line_width /= line_cam_dist;

    // Adjust the line width depending on vertical screen height.
    // The reason for that is because CSGO has a fixed vertical FOV and these
    // lines are part of the world geometry and should grow with it.
    screen_line_width *= GL::defaultFramebuffer.viewport().size().y();

    // Lines don't draw correctly if one of their points is behind the camera.
    // -> Truncate lines that intersect the camera's plane

    // Calculate camera plane to detect if points are behind the camera
    Vector4 cam_plane = Math::planeEquation(cam_dir_normal, cam_pos);

    bool entirely_clipped = ClipLineToCameraPlane(start, end, cam_plane);
    if (entirely_clipped)
        return; // Unable to draw line without graphical issues

    Vector3 dir = end - start;
    float clipped_length = dir.length();
    float yaw = 0.0f, pitch = 0.0f;

    if (clipped_length < 0.01) {
        yaw = pitch = 0.0f;
    }
    else {
        dir /= clipped_length; // Normalize direction
        
        if (dir.xy().dot() < 0.001f * 0.001f)
            yaw = 0.0f; // Avoid atan2 if both x and y are zero
        else
            yaw = std::atan2(dir.y(), dir.x());

        if(dir.z() > 0) pitch = -std::acos(dir.xy().length());
        else            pitch = +std::acos(dir.xy().length());
    }

    Shaders::LineGL3D& shader = _shader_cap_style_round;
    shader
        .setViewportSize(Vector2{ GL::defaultFramebuffer.viewport().size() })
        .setWidth(screen_line_width)
        .setColor(color)
        .setSmoothness(1.0f)
        .setTransformationProjectionMatrix(
            view_proj_transformation *
            Matrix4::translation(start) *
            Matrix4::rotationZ(Rad{ yaw   }) *
            Matrix4::rotationY(Rad{ pitch }) *
            Matrix4::scaling(Vector3{ clipped_length })
        )
        .draw(_x_line_mesh);

    // Reset draw state
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setDepthMask(true);
}

void WideLineRenderer::DrawDirectionIndicator(
    const Color4& color, const Vector3 ind_pos, const Vector3 ind_normal,
    const Matrix4& view_proj_transformation,
    const Vector3& cam_pos, const Vector3& cam_dir_normal, bool no_depth_test)
{
    // Draw line in normal direction
    DrawLine(
        Color3{ 1.0f, 1.0f, 1.0f },
        ind_pos,
        ind_pos + 14.0f * ind_normal,
        view_proj_transformation,
        cam_pos, cam_dir_normal, no_depth_test
    );

    if (!ind_normal.isNormalized()) {
        assert(0);
        return;
    }

    // Step 1: Create a vector that's linearly independent to the input normal.
    //         Approach: Add 5 to the smallest vector component.
    //         I can't prove this produces a linearly independent vector, but it seems so.
    int smallest_idx = 0;
    if (Math::abs(ind_normal[1]) < Math::abs(ind_normal[smallest_idx])) smallest_idx = 1;
    if (Math::abs(ind_normal[2]) < Math::abs(ind_normal[smallest_idx])) smallest_idx = 2;

    Vector3 linearly_independent_vec = ind_normal;
    linearly_independent_vec[smallest_idx] += 5.0f;

    // Step 2: Create a vector perpendicular to the input normal.
    Vector3 perp_vec_1 = Math::cross(ind_normal, linearly_independent_vec);

    if (perp_vec_1.isZero()) { // Shouldn't happen
        assert(0);
        return;
    }
    perp_vec_1 = perp_vec_1.normalized();

    // Step 3: Create a vector perpendicular to both the previous vector and
    //         the input normal.
    Vector3 perp_vec_2 = Math::cross(ind_normal, perp_vec_1);

    // At this point, ind_normal, perp_vec_1 and perp_vec_2 are all
    // perpendicular to each other and have length 1.

    // Draw lines in the plane
    const float CIRCLE_RAY_LENGTH = 10.0f;
    const size_t NUM_CIRCLE_RAYS = 12;
    const float ANGLE_DELTA = 360.0f / (float)NUM_CIRCLE_RAYS;
    Color4 circle_col = { 1.0f * color.rgb(), color.a() };
    for (float angle = 0.0f; angle < 360.0f; angle += ANGLE_DELTA) {
        auto sin_cos = Math::sincos(Deg{ angle });
        Vector3 v = sin_cos.second() * perp_vec_1 + sin_cos.first() * perp_vec_2;

        DrawLine(
            circle_col,
            ind_pos,
            ind_pos + CIRCLE_RAY_LENGTH * v,
            view_proj_transformation,
            cam_pos, cam_dir_normal, no_depth_test
        );
    }
}
