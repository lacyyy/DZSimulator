#ifndef REN_WORLDRENDERER_H_
#define REN_WORLDRENDERER_H_

#include <memory>
#include <vector>

#include <Corrade/Utility/Resource.h>
#include <Magnum/Magnum.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Shaders/FlatGL.h>

#include "gui/GuiState.h"
#include "ren/GlidabilityShader3D.h"
#include "ren/RenderableWorld.h"

namespace ren {

class WorldRenderer {
public:
    WorldRenderer(
        const Corrade::Utility::Resource& resources, gui::GuiState& gui_state);

    // Initialize shaders and meshes that require an OpenGL context to be active.
    void InitWithOpenGLContext();

    void Draw(std::shared_ptr<RenderableWorld> ren_world,
        const Magnum::Matrix4& view_proj_transformation,
        const Magnum::Vector3& player_feet_pos,
        float hori_player_speed,
        const std::vector<Magnum::Vector3>& bump_mine_positions);

private:
    Magnum::Color4 CvtImguiCol4(float* im_col4);

    const Corrade::Utility::Resource& _resources; // Used to load shader source code
    gui::GuiState& _gui_state;

    // Shaders
    GlidabilityShader3D _glid_shader_instanced{ Magnum::NoCreate };
    GlidabilityShader3D _glid_shader_non_instanced{ Magnum::NoCreate };
    Magnum::Shaders::FlatGL3D _flat_shader{ Magnum::NoCreate };

    // Other meshes
    Magnum::GL::Mesh _mesh_bump_mine{ Magnum::NoCreate };

};

} // namespace ren

#endif // REN_WORLDRENDERER_H_
