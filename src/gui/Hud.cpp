#include "Hud.h"

#include <chrono>

#include <Magnum/Magnum.h>
#include <Corrade/Utility/Debug.h>

#include "Gui.h"
#include "GuiState.h"

using namespace gui;

Hud::Hud(Gui& gui) : _gui(gui), _gui_state(gui.state)
{}

void Hud::Draw()
{
    static auto t_start = std::chrono::steady_clock::now();
    auto t = std::chrono::steady_clock::now() - t_start;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
    
    const long long ANIM_PERIOD_MS = 800;
    const float ANIM_DIST = 200; // pixels

    float anim_progress = (float)(ms % ANIM_PERIOD_MS) / ANIM_PERIOD_MS;

    float offset = anim_progress * ANIM_DIST;

    if (_gui_state.hud.OUT_show_exo_boost_available) {
        for (size_t i = 0; i < 10; i++) {
            DrawArrow(50 + i * 100, 500 - offset, 1.0f);
        }
    }

    DrawSpeedometer();
}

void gui::Hud::DrawSpeedometer()
{
    static float prev_vel = 0.0f;
    float vel = _gui_state.hud.OUT_hori_player_speed;
    float vel_diff = vel - prev_vel;

    std::string vel_text = std::to_string(vel);

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImVec2 screen_center = {
        main_viewport->WorkPos.x + 0.5f * main_viewport->WorkSize.x,
        main_viewport->WorkPos.y + 0.2f * main_viewport->WorkSize.y
    };

    ImGui::GetBackgroundDrawList()->AddText(
        _gui._font_display,
        100.0f,
        ImVec2 (screen_center.x, screen_center.y - 100.0f),
        vel_diff == 0.0f ?
            IM_COL32(255, 255, 255, 255) :
            (vel_diff > 0.0f ?
                IM_COL32(100, 100, 255, 255) :
                IM_COL32(255, 0, 0, 255)),
        vel_text.c_str());

    const float RECT_W = 500.0f;
    const float RECT_H = 100.0f;

    // Border
    ImGui::GetBackgroundDrawList()->AddRect(
        ImVec2(screen_center.x - 0.5* RECT_W, screen_center.y - 0.5 * RECT_H),
        ImVec2(screen_center.x + 0.5* RECT_W, screen_center.y + 0.5 * RECT_H),
        IM_COL32(0, 0, 0, 255),
        0.0f,
        ImDrawFlags_None,
        10.0f);

    const int VEL_RANGE_SIZE = 600;
    const std::vector<ImU32> range_cols = {
        IM_COL32(100, 100, 100, 255),
        IM_COL32(  0, 255,   0, 255),
        IM_COL32(255, 255,   0, 255),
        IM_COL32(255,   0,   0, 255),
        IM_COL32(255,   0, 255, 255),
    };

    size_t fg_bar_idx =
        _gui_state.hud.OUT_hori_player_speed / (float)VEL_RANGE_SIZE;
    if (fg_bar_idx < 0)
        fg_bar_idx = 0;
    if (fg_bar_idx > range_cols.size() - 1)
        fg_bar_idx = range_cols.size() - 1;


    // Inside bars
    float bar_progress =
        ((int)_gui_state.hud.OUT_hori_player_speed % VEL_RANGE_SIZE)
        / (float)VEL_RANGE_SIZE;
    // Background bar
    if (fg_bar_idx > 0) {
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(screen_center.x - 0.5 * RECT_W, screen_center.y - 0.5 * RECT_H),
            ImVec2(screen_center.x + 0.5 * RECT_W, screen_center.y + 0.5 * RECT_H),
            range_cols[fg_bar_idx - 1],
            0.0f,
            ImDrawFlags_None);
    }
    // Foreground bar
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(screen_center.x - 0.5 * RECT_W, screen_center.y - 0.5 * RECT_H),
        ImVec2(screen_center.x - 0.5 * RECT_W + bar_progress * RECT_W, screen_center.y + 0.5 * RECT_H),
        range_cols[fg_bar_idx],
        0.0f,
        ImDrawFlags_None);


    prev_vel = vel;
}

void gui::Hud::DrawArrow(float x, float y, float scale)
{
    const auto col = IM_COL32(127, 0, 255, 200);
    float STEP = scale * 20.0f;


    ImGui::GetBackgroundDrawList()->AddQuadFilled(
        ImVec2(x, y),
        ImVec2(x+STEP, y-STEP),
        ImVec2(x + 2 * STEP, y),
        ImVec2(x + STEP, y + STEP),
        col);

    ImGui::GetBackgroundDrawList()->AddQuadFilled(
        ImVec2(x - STEP, y + STEP),
        ImVec2(x - 2 * STEP, y),
        ImVec2(x, y - 2 * STEP),
        ImVec2(x + STEP, y - STEP),
        col);

}
