#ifndef REN_CROSSHAIR_H_
#define REN_CROSSHAIR_H_

#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Tags.h>
#include <Magnum/Magnum.h>

#include "gui/GuiState.h"

namespace ren {

class Crosshair {
public:
    Crosshair(gui::GuiState& gui_state);

    void InitWithOpenGLContext();

    // This must be called after window creation and after every window change!
    void HandleViewportEvent(Magnum::Vector2i new_window_size,
                             Magnum::Vector2  new_dpi_scaling);

    void Draw();

private:
    // Draw rectangles using pixel position and pixel width and height.
    // Position (0,0) is the bottom left corner of the screen.
    void DrawRect(const Magnum::Vector2i&  rect_bottom_left_corner,
                  const Magnum::Vector2ui& rect_extents,
                  const Magnum::Color4&    rect_color);
    void DrawHoriCrosshairRect(int left, int right, unsigned int half_height,
                               const Magnum::Color4& c);
    void DrawVertCrosshairRect(int bottom, int top, unsigned int half_width,
                               const Magnum::Color4& c);

    Magnum::GL::Mesh          rect_mesh{ Magnum::NoCreate };
    Magnum::Shaders::FlatGL2D shader   { Magnum::NoCreate };

    gui::GuiState&   gui_state;
    Magnum::Vector2i window_size = { 99, 99 };     // To be overwritten
    Magnum::Vector2  dpi_scaling = { 9.9f, 9.9f }; // To be overwritten
};

} // namespace ren

#endif // REN_CROSSHAIR_H_
