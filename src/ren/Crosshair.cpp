#include "Crosshair.h"

#include <cassert>
#include <utility>

#include <Corrade/Containers/ArrayView.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Shaders/FlatGL.h>

#include "gui/GuiState.h"

using namespace ren;
using namespace Magnum;
using namespace Magnum::Math::Literals;

Crosshair::Crosshair(gui::GuiState& gui_state) : gui_state{ gui_state }
{
}

void Crosshair::InitWithOpenGLContext()
{
    const Vector2 rect_vert_positions[]{ // Triangle strip positions, CW
        { 0.0f, 0.0f }, // Bottom left vertex
        { 0.0f, 1.0f }, // Top    left vertex
        { 1.0f, 0.0f }, // Bottom right vertex
        { 1.0f, 1.0f }, // Top    right vertex
    };
    GL::Buffer rect_vert_buf{ GL::Buffer::TargetHint::Array };
    rect_vert_buf.setData(rect_vert_positions);

    rect_mesh = GL::Mesh{};
    rect_mesh.setPrimitive(GL::MeshPrimitive::TriangleStrip)
             .setCount(Containers::arraySize(rect_vert_positions))
             .addVertexBuffer(std::move(rect_vert_buf), 0,
                              Shaders::FlatGL2D::Position{});
    shader = Shaders::FlatGL2D{};
}

void Crosshair::HandleViewportEvent(Vector2i new_window_size,
                                    Vector2 new_dpi_scaling)
{
    window_size = new_window_size;
    dpi_scaling = new_dpi_scaling;
}

void Crosshair::Draw()
{
    Color4 col = gui::CvtImguiCol4(gui_state.vis.IN_crosshair_col);

    // Let the crosshair sensibly resize together with the window's size
    float window_aspect_ratio = (float)window_size.x() / (float)window_size.y();
    float window_factor;
    if (window_aspect_ratio < 16.0f / 9.0f) // HUD is intended for 16:9 screens
        window_factor = (float)(window_size.x()) / 1920.0f;
    else
        window_factor = (float)(window_size.y()) / 1080.0f;

    // Let the crosshair also scale with the DPI setting
    float total_scaling = gui_state.vis.IN_crosshair_scale *
                          dpi_scaling.x() *
                          window_factor;

    unsigned int final_length    = gui_state.vis.IN_crosshair_length
                                   * total_scaling + 0.5f;
    unsigned int final_thickness = gui_state.vis.IN_crosshair_thickness
                                   * total_scaling + 0.5f;
    unsigned int final_gap       = gui_state.vis.IN_crosshair_gap
                                   * total_scaling + 0.5f;

    if (final_length    < 1) final_length    = 1;
    if (final_thickness < 1) final_thickness = 1;

    DrawHoriCrosshairRect(
        window_size.x() / 2 - final_length - final_gap + window_size.x() % 2,
        window_size.x() / 2 - 1            - final_gap + window_size.x() % 2,
        final_thickness, col);
    DrawHoriCrosshairRect(
        window_size.x() / 2 + final_gap,
        window_size.x() / 2 + final_gap + final_length - 1,
        final_thickness, col);
    DrawVertCrosshairRect(
        window_size.y() / 2 - final_length - final_gap + window_size.y() % 2,
        window_size.y() / 2 - 1            - final_gap + window_size.y() % 2,
        final_thickness, col);
    DrawVertCrosshairRect(
        window_size.y() / 2 + final_gap,
        window_size.y() / 2 + final_gap + final_length - 1,
        final_thickness, col);
}

void Crosshair::DrawRect(const Vector2i&  rect_bottom_left_corner,
                         const Vector2ui& rect_extents,
                         const Color4&    rect_color)
{
    // GL::Renderer::Feature::Blending must be enabled and the proper blend
    // function must be set before calling this method.

    Vector2 mesh_scaling =
        2.0f * Vector2{rect_extents} / Vector2{window_size};
    Vector2 mesh_translation = Vector2{ -1.0f, -1.0f } +
        2.0f * Vector2{rect_bottom_left_corner} / Vector2{window_size};

    Matrix3 transformation = Matrix3::translation(mesh_translation) *
                             Matrix3::scaling(mesh_scaling);
    shader.setColor(rect_color)
          .setTransformationProjectionMatrix(transformation)
          .draw(rect_mesh);
}

void Crosshair::DrawHoriCrosshairRect(int left, int right,
                                      unsigned int half_height, const Color4& c)
{
    assert(left <= right);
    int bottom = window_size.y() / 2 - half_height + window_size.y() % 2;
    unsigned int width  = right - left + 1;
    unsigned int height = 2 * half_height - window_size.y() % 2;
    DrawRect({ left, bottom }, { width, height }, c);
}

void Crosshair::DrawVertCrosshairRect(int bottom, int top,
                                      unsigned int half_width, const Color4& c)
{
    assert(bottom <= top);
    int left = window_size.x() / 2 - half_width + window_size.x() % 2;
    unsigned int height = top - bottom + 1;
    unsigned int width  = 2 * half_width - window_size.x() % 2;
    DrawRect({ left, bottom }, { width, height }, c);
}
