#include "ren/WorldRenderer.h"

#include <string>
#include <vector>

#include <Tracy.hpp>

#include <Magnum/GL/Renderer.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Angle.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix4.h>

#include "csgo_parsing/BrushSeparation.h"
#include "CsgoConstants.h"
#include "ren/RenderableWorld.h"
#include "utils_3d.h"
#include "WorldCreator.h"

using namespace Magnum;
using namespace Math::Literals;
using namespace ren;

namespace BrushSep = csgo_parsing::BrushSeparation;

WorldRenderer::WorldRenderer(const Utility::Resource& resources,
        gui::GuiState& gui_state)
    : _resources{ resources }
    , _gui_state{ gui_state }
{
}

void WorldRenderer::InitWithOpenGLContext()
{
    ZoneScoped;

    _glid_shader_instanced     = GlidabilityShader3D{  true, _resources };
    _glid_shader_non_instanced = GlidabilityShader3D{ false, _resources };
    _flat_shader = Shaders::FlatGL3D{ };

    _mesh_bump_mine = WorldCreator::CreateBumpMineMesh();
}

void WorldRenderer::Draw(std::shared_ptr<RenderableWorld> ren_world,
    const Matrix4& view_proj_transformation,
    const Vector3& player_feet_pos,
    float hori_player_speed,
    const std::vector<Vector3>& bump_mine_positions)
{
    ZoneScoped;

    Deg hori_light_angle{ _gui_state.vis.IN_hori_light_angle };
    Vector3 light_dir(
        Math::cos(hori_light_angle),
        Math::sin(hori_light_angle),
        0.0f
    ); // vector must be normalized

#ifdef DZSIM_WEB_PORT
    bool has_world_diffuse_lighting = true;
#else
    // Don't do lighting in overlay, it is inaccurate compared to CSGO's lighting
    bool has_world_diffuse_lighting = !_gui_state.video.IN_overlay_mode_enabled;
#endif

    bool glidability_vis_globally_disabled =
        _gui_state.vis.IN_geo_vis_mode != _gui_state.vis.GLID_AT_SPECIFIC_SPEED &&
        _gui_state.vis.IN_geo_vis_mode != _gui_state.vis.GLID_OF_CSGO_SESSION;

    if (hori_player_speed < 1.0)
        hori_player_speed = 1.0;

    // Set some uniforms for both glidability shaders
    for (size_t i = 0; i < 2; i++) {
        // TODO Put uniforms that are shared between multiple shaders in a
        //      uniform buffer?
        GlidabilityShader3D& glid_shader = (i == 0) ?
            _glid_shader_instanced :
            _glid_shader_non_instanced;

        glid_shader
            .SetLightDirection(light_dir)
            .SetPlayerPosition(player_feet_pos)
            .SetHorizontalPlayerSpeed(hori_player_speed)
            .SetSlideSuccessColor   (CvtImguiCol4(_gui_state.vis.IN_col_slide_success))
            .SetSlideAlmostFailColor(CvtImguiCol4(_gui_state.vis.IN_col_slide_almost_fail))
            .SetSlideFailColor      (CvtImguiCol4(_gui_state.vis.IN_col_slide_fail));

        // Game settings
        glid_shader
            .SetGravity(CSGO_CVAR_SV_GRAVITY)
            .SetMinNoGroundChecksVelZ(CSGO_CONST_MIN_NO_GROUND_CHECKS_VEL_Z)
            .SetMaxVelocity(CSGO_CVAR_SV_MAXVELOCITY)
            .SetStandableNormal(CSGO_CVAR_SV_STANDABLE_NORMAL);
    }

    GL::Renderer::setFrontFace(GL::Renderer::FrontFace::ClockWise);

#ifndef DZSIM_WEB_PORT
    GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Fill);
    //GL::Renderer::setPolygonMode(GL::Renderer::PolygonMode::Line);
    //GL::Renderer::setLineWidth(1.0f);
#endif

    // Draw displacements
    _glid_shader_non_instanced
        .SetFinalTransformationMatrix(view_proj_transformation)
        // gray-yellow-orange
        .SetOverrideColor(CvtImguiCol4(_gui_state.vis.IN_col_solid_displacements))
        .SetColorOverrideEnabled(glidability_vis_globally_disabled)
        .SetDiffuseLightingEnabled(has_world_diffuse_lighting)
        .draw(ren_world->mesh_displacements);

    // Draw displacement boundaries
    if (_gui_state.vis.IN_draw_displacement_edges)
        _flat_shader
            .setTransformationProjectionMatrix(view_proj_transformation)
            .setColor(CvtImguiCol4(_gui_state.vis.IN_col_solid_disp_boundary))
            .draw(ren_world->mesh_displacement_boundaries);

    // Draw bump mines - they're currently the only thing drawn with CCW vertex winding
    // TODO use instancing?
    GL::Renderer::setFrontFace(GL::Renderer::FrontFace::CounterClockWise);
    for (Vector3 bm_pos : bump_mine_positions) {
        Matrix4 model_transformation =
            utils_3d::CalcModelTransformationMatrix(bm_pos,
                { 0.0f, 0.0f, 0.0f }, 20.0f);
        Matrix4 mvp_transformation =
            view_proj_transformation * model_transformation;

        _glid_shader_non_instanced
            .SetFinalTransformationMatrix(mvp_transformation)
            .SetOverrideColor(CvtImguiCol4(_gui_state.vis.IN_col_bump_mine))
            .SetColorOverrideEnabled(true)
            .SetDiffuseLightingEnabled(has_world_diffuse_lighting)
            .draw(_mesh_bump_mine);
    }
    GL::Renderer::setFrontFace(GL::Renderer::FrontFace::ClockWise);

    // Draw collision models of props (static or dynamic)
    _glid_shader_instanced
        .SetFinalTransformationMatrix(view_proj_transformation)
        .SetColorOverrideEnabled(glidability_vis_globally_disabled)
        .SetOverrideColor(CvtImguiCol4(_gui_state.vis.IN_col_solid_xprops))
        .SetDiffuseLightingEnabled(has_world_diffuse_lighting);
    for (GL::Mesh& instanced_xprop_mesh : ren_world->instanced_xprop_meshes) {
        _glid_shader_instanced.draw(instanced_xprop_mesh);
    }

    // TRANSPARENT BRUSHES MUST BE THE LAST THINGS BEING DRAWN

    // Determine draw order of brush categories
    std::vector<BrushSep::Category> brush_cat_draw_order;
    std::vector<BrushSep::Category> last_drawn_brush_cats;
    brush_cat_draw_order.reserve(ren_world->brush_category_meshes.size());
    for (const auto &kv : ren_world->brush_category_meshes) {
        BrushSep::Category b_cat = kv.first;

        // Transparent things must be the last things being drawn!
        if (b_cat == BrushSep::Category::WATER ||
            b_cat == BrushSep::Category::GRENADECLIP ||
            b_cat == BrushSep::Category::PLAYERCLIP)
            last_drawn_brush_cats.push_back(b_cat);
        else
            brush_cat_draw_order.push_back(b_cat);
    }
    for (BrushSep::Category b_cat : last_drawn_brush_cats)
        brush_cat_draw_order.push_back(b_cat);
        
    // Draw brush categories
    for (BrushSep::Category b_cat : brush_cat_draw_order) {
        // Determine if brush category's surface glidability is visualized
        bool visualize_glidability = false;
        if (b_cat == BrushSep::Category::SOLID ||
            b_cat == BrushSep::Category::PLAYERCLIP)
            visualize_glidability = true;

        if (glidability_vis_globally_disabled)
            visualize_glidability = false;

        // Determine if current brush category's mesh color should be
        // darkened depending on angle to the light. Sky's and water's color
        // should not be influenced by light positions.
        bool has_brush_mesh_diffuse_lighting = has_world_diffuse_lighting;
        if (b_cat == BrushSep::Category::SKY ||
            b_cat == BrushSep::Category::WATER)
            has_brush_mesh_diffuse_lighting = false;

        // Determine override color
        float unknown_col[4] { 1.0f, 1.0f, 1.0f, 1.0f };
        float* b_col = unknown_col;
        switch (b_cat) {
        case BrushSep::SKY:         b_col = _gui_state.vis.IN_col_sky; break;
        case BrushSep::LADDER:      b_col = _gui_state.vis.IN_col_ladders; break;
        case BrushSep::SOLID:       b_col = _gui_state.vis.IN_col_solid_other_brushes; break;
        case BrushSep::WATER:       b_col = _gui_state.vis.IN_col_water; break;
        case BrushSep::PLAYERCLIP:  b_col = _gui_state.vis.IN_col_player_clip; break;
        case BrushSep::GRENADECLIP: b_col = _gui_state.vis.IN_col_grenade_clip; break;
        }

        // Don't draw if alpha is zero.
        // Useful for developing in cases where transparency causes issues
        // (e.g. things disappearing behind transparent surfaces).
        if (b_col[3] == 0.0f)
            continue;

        _glid_shader_non_instanced
            .SetFinalTransformationMatrix(view_proj_transformation)
            .SetOverrideColor(CvtImguiCol4(b_col))
            .SetColorOverrideEnabled(visualize_glidability == false)
            .SetDiffuseLightingEnabled(has_brush_mesh_diffuse_lighting)
            .draw(ren_world->brush_category_meshes[b_cat]);
    }

    // ANYTHING BEING DRAWN AFTER HERE WILL NOT BE VISIBLE BEHIND
    // TRANSPARENT BRUSHES

    // Draw trigger_push entities that can push players.
    // Don't draw if alpha is zero.
    // Useful for developing in cases where transparency causes issues
    // (e.g. things disappearing behind transparent surfaces).
    if(_gui_state.vis.IN_col_trigger_push[3] != 0.0f)
        _glid_shader_non_instanced
            .SetFinalTransformationMatrix(view_proj_transformation)
            .SetOverrideColor(CvtImguiCol4(_gui_state.vis.IN_col_trigger_push))
            .SetColorOverrideEnabled(true)
            .SetDiffuseLightingEnabled(true)
            .draw(ren_world->trigger_push_meshes);

}

Magnum::Color4 WorldRenderer::CvtImguiCol4(float* im_col4)
{
    return Magnum::Color4(im_col4[0], im_col4[1], im_col4[2], im_col4[3]);
}
