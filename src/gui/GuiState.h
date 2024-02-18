#ifndef GUI_GUISTATE_H_
#define GUI_GUISTATE_H_

#include <deque>
#include <string>
#include <vector>

#include <Corrade/Containers/String.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>

#include "CsgoConstants.h"

namespace gui {

// Important interface between the GUI and the rest of the application.
// GUI state can neatly be read or set.
// IN_* variables hold data created by the GUI/user (i.e. button presses).
// OUT_* variables hold data created by the game that gets used to display info.
// Other variables can be both, bidirectional in that sense.
// Occasionally, IN_* variables get set by the game, e.g. to set it to a valid
// value, even though that variable is primarily set by the GUI.
class GuiState {
public:

    // Misc
    bool app_exit_requested = false;
    bool show_intro_msg_on_startup = true;
    bool OUT_dzsim_update_available = false; // New release on GitHub
    bool IN_open_downloads_page_in_browser = false; // GitHub releases page

    // Window opening / closing
    bool show_window_legal_notices = false;
    bool show_window_demo = false;

    struct Popup {
        void QueueMsgInfo(const std::string& msg);
        void QueueMsgWarn(const std::string& msg);
        void QueueMsgError(const std::string& msg);
        bool IN_visible = false; // Signal to game code to allow cursor to close popup
        bool OUT_close_current = false; // signal from game code to close popup
        // Each popup msg queue entry consists of 2 null-terminated strings, then
        // followed by the actual popup msg. First null-terminated string is a msg
        // type identifier. Second null-terminated string is the popup's window
        // title. The actual popup msg comes after the second null-terminator.
        std::deque<std::string> OUT_msg_q; // ONLY USED INTERNALLY BY GUI
    } popup;

    struct ControlHelp {
        bool OUT_first_person_control_active = false;
    } ctrl_help;

    struct MapSelection {
        bool IN_box_opened = false; // User opened map select combo box
        std::string IN_new_abs_map_path_load = ""; // Absolute file path
        std::string OUT_csgo_path = ""; // Absolute path to game directory
        std::vector<std::string> OUT_loadable_maps; // relative to 'csgo/maps/'
        size_t OUT_num_highlighted_maps = 0; // Color first N map entries
    } map_select;

    struct VisualizationSettings {
        float IN_hori_light_angle = 292.5f; // in degrees, 0.0 to 360.0
        bool IN_draw_displacement_edges = false;

        // RGBA colors
        float IN_col_sky                [4] = { 0.422f, 0.645f, 1.000f, 1.000f };
        float IN_col_water              [4] = { 0.000f, 0.644f, 1.000f, 0.258f };
        float IN_col_ladders            [4] = { 1.000f, 0.000f, 1.000f, 1.000f };
        float IN_col_player_clip        [4] = { 0.407f, 0.570f, 0.000f, 0.869f };
        float IN_col_grenade_clip       [4] = { 1.000f, 1.000f, 1.000f, 0.500f };
        float IN_col_trigger_push       [4] = { 0.400f, 0.000f, 1.000f, 0.500f };
        float IN_col_solid_displacements[4] = { 0.747f, 0.621f, 0.244f, 1.000f };
        float IN_col_solid_xprops       [4] = { 0.796f, 0.551f, 0.194f, 1.000f };
        float IN_col_solid_other_brushes[4] = { 1.000f, 0.929f, 0.000f, 1.000f }; // solid, non-sky, non-water, non-ladder
        float IN_col_solid_disp_boundary[4] = { 0.000f, 1.000f, 0.750f, 1.000f };
        float IN_col_bump_mine          [4] = { 1.000f, 0.000f, 1.000f, 1.000f };
        float IN_col_slide_success      [4] = { 0.000f, 1.000f, 0.000f, 1.000f };
        float IN_col_slide_almost_fail  [4] = { 0.800f, 0.400f, 0.150f, 1.000f };
        float IN_col_slide_fail         [4] = { 0.400f, 0.400f, 0.400f, 1.000f };


        enum GeometryVisualizationMode {
            GEO_TYPE,
            GLID_AT_SPECIFIC_SPEED, // glidability at specific player speed
            GLID_OF_CSGO_SESSION  // glidability for player in local csgo session
        } IN_geo_vis_mode = GLID_AT_SPECIFIC_SPEED;

        bool IN_display_hori_vel_text = true;
        float IN_hori_vel_text_size = 1.0f;
        float IN_col_hori_vel_text[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        Magnum::Vector2 IN_hori_vel_text_pos = { 0.0f, -0.05f };

        // Only used in GLID_AT_SPECIFIC_SPEED mode
        int IN_specific_glid_vis_hori_speed = 1150; // Horizontal speed of common single bump mine jump
    } vis;

    struct VideoSettings {
        int IN_user_gui_scaling_factor_pct = 100; // percentage

        bool IN_use_custom_fov = false;
        float IN_custom_vert_fov_degrees = float(Magnum::Deg{ CSGO_VERT_FOV });

        enum WindowMode {
            WINDOWED,
            FULLSCREEN_WINDOWED // Windowed, but covering an entire display
        } IN_window_mode = FULLSCREEN_WINDOWED;

        // Only relevant in FULLSCREEN_WINDOWED mode: On which SDL display our
        // window gets placed
        int IN_selected_display_idx = -1; // idx into OUT_available_displays

        struct AvailableDisplay { int x, y, w, h; }; // from SDL_GetDisplayBounds()
        std::vector<AvailableDisplay> OUT_available_displays;
        bool IN_available_display_refresh_needed = false;

#ifndef DZSIM_WEB_PORT
        // In overlay mode, the DZSim window stays always on top of other
        // windows and turns transparent. Additionally, it becomes click-through
        // once the DZSim window loses focus.
        bool IN_overlay_mode_enabled = false;
        float IN_overlay_transparency = 60.0f; // from 0 to 100 
        bool IN_overlay_transparency_is_being_adjusted = false; // Indicates preview
#endif

        bool IN_vsync_enabled = true; // VSync ON by default to use monitor's refresh rate

        // FPS limit, only used if VSync disabled, initialized to default 125 FPS
        unsigned int IN_min_loop_period = 8; // Default FPS limit = 1000ms / 8ms = 125
    } video;

    struct RemoteConsole {
        bool IN_start_connect = false;
        bool IN_disconnect = false;
        bool OUT_is_connecting = false;
        bool OUT_is_connected = false;
        bool OUT_has_connect_failed = false;
        bool OUT_is_disconnecting = false;
        std::string OUT_fail_msg = "";
    } rcon;

    struct GameStateIntegration {
        bool IN_enabled = false;
        bool IN_imitate_spec_cam = false;
        bool OUT_running = false;
        bool OUT_exited_unexpectedly = false;
        std::string OUT_info = "0 gamestates received";
        std::string OUT_latest_json_payload = "";
    } gsi;

    struct Performance {
        float OUT_frame_time_mean_ms;
        Corrade::Containers::String OUT_magnum_profiler_stats;

        // Last frame's game simulation calc time (Changes every frame)
        float OUT_last_sim_calc_time_us;
    } perf;

    struct CollisionDebugging { // Only available in Debug builds
        // Show all displacements that ...
        bool IN_showDispsForHullColl = false;   // ... are used for hull traces
        bool IN_showDispsWithCollCache = false; // ... have generated a collision cache
    } coll_debug;

    struct TestSettings { // Settings that can only be adjusted in Debug builds
        float IN_slider1 = 0.1f;
        float IN_slider2 = 0.1f;
        float IN_slider3 = 0.0f;
        float IN_slider4 = 0.0f;
    } testing;

};

}

#endif // GUI_GUISTATE_H_
