#ifndef RENDERING_WORLDRENDERER_H_
#define RENDERING_WORLDRENDERER_H_

#include <memory>
#include <vector>

#include <Corrade/Utility/Resource.h>
#include <Magnum/Magnum.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Shaders/FlatGL.h>

#include "csgo_parsing/BspMap.h"
#include "gui/GuiState.h"
#include "rendering/GlidabilityShader3D.h"
#include "WorldCreation.h"

namespace rendering {

class WorldRenderer {
public:
    WorldRenderer(
        const Corrade::Utility::Resource& resources, gui::GuiState& gui_state);

    // Initialize shaders, requires an OpenGL context to be active.
    void InitShaders();

    void UnloadGeometry(); // Deallocates map data
    void LoadBspMapGeometry(std::shared_ptr<const csgo_parsing::BspMap> bsp_map);

    void Draw(const Magnum::Matrix4& view_proj_transformation,
        const Magnum::Vector3& player_feet_pos,
        const Magnum::Vector3& player_velocity,
        const std::vector<Magnum::Vector3>& bump_mine_positions);

private:
    Magnum::Color4 CvtImguiCol4(float* im_col4);

    const Corrade::Utility::Resource& _resources; // Used to load shader source code
    gui::GuiState& _gui_state;

    // Shaders
    GlidabilityShader3D _glid_shader_instanced{ Magnum::NoCreate };
    GlidabilityShader3D _glid_shader_non_instanced{ Magnum::NoCreate };
    Magnum::Shaders::FlatGL3D _flat_shader{ Magnum::NoCreate };

    // World meshes
    std::unique_ptr<rendering::CsgoMapGeometry> _map_geo{ nullptr };

    // Other meshes
    Magnum::GL::Mesh _mesh_bump_mine{ Magnum::NoCreate };

};

} // namespace rendering

#endif // RENDERING_WORLDRENDERER_H_
