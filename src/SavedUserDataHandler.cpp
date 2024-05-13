#include "SavedUserDataHandler.h"

#include <string>

#ifndef DZSIM_WEB_PORT
#define UNICODE // Required in order to work with paths containing Unicode
#include <Windows.h>
#include <shellapi.h> // For WINAPI's ShellExecuteW()
#endif

#include <Tracy.hpp>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StringStl.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Path.h>
#include <Corrade/Utility/Unicode.h>
#include <json.hpp>

using namespace Corrade;
using json = nlohmann::json;

// Increment this number each time we change the JSON format of user data.
// If possible, also try to make sure the previous versions can be upgraded to
// the latest version (to preserve a user's settings across updates).
const int CURRENT_USER_DATA_FORMAT_VERSION = 2;

// -----------------------------------------------------------------------------

// History of DZSimulator's handling of saved user data:
//
// DZSimulator v0.0.1: Does not save user data
// DZSimulator v0.0.2: Does not save user data
// DZSimulator v0.0.3: Does not save user data
// DZSimulator v0.0.4: Saves user data using UserDataFormat Version 1
// DZSimulator v1.0.0: Saves user data using UserDataFormat Version 2
//                     Note: Does _not_ copy/upgrade user data from v0.0.4!
//                           (To start fresh with new defaults for settings.)

// -----------------------------------------------------------------------------

const char* DEBUG_ID = "[SavedUserData]";

static std::string Color4fToStr(const float *src) {
    return Utility::format("{:.3f} {:.3f} {:.3f} {:.3f}", src[0], src[1], src[2], src[3]);
}

// If nested value doesn't exist, nullptr gets returned
static const json* GetNestedValue(const json *obj, Containers::StringView key1) {
    if (!obj || !obj->contains(key1)) return nullptr;
    return &((*obj)[key1]);
}
static const json* GetNestedValue(const json *obj, Containers::StringView key1, Containers::StringView key2) {
    const json *sub_obj = GetNestedValue(obj, key1);
    if (!sub_obj || !sub_obj->contains(key2)) return nullptr;
    return &((*sub_obj)[key2]);
}
static const json* GetNestedValue(const json *obj, Containers::StringView key1, Containers::StringView key2, Containers::StringView key3) {
    const json* sub_obj = GetNestedValue(obj, key1, key2);
    if (!sub_obj || !sub_obj->contains(key3)) return nullptr;
    return &((*sub_obj)[key3]);
}

static void TryParseBool  (bool&         dest, const json* src) { if (src && src->is_boolean()        ) dest = src->get<bool>();        }
static void TryParseInt   (int&          dest, const json* src) { if (src && src->is_number_integer() ) dest = src->get<int>();         }
static void TryParseUInt  (unsigned int& dest, const json* src) { if (src && src->is_number_unsigned()) dest = src->get<unsigned int>();}
static void TryParseFloat (float&        dest, const json* src) { if (src && src->is_number()         ) dest = src->get<float>();       }
static void TryParseString(std::string&  dest, const json* src) { if (src && src->is_string()         ) dest = src->get<std::string>(); }

// E.g. parse JSON string "0.23 0.12 0.84 0.4" into a float array with 4 elements
static void TryParseColor4f(float* dest, const json* src) {
    std::string s = "";
    TryParseString(s, src);
    Containers::StringView sv = s;
    auto substring_list = sv.splitOnWhitespaceWithoutEmptyParts();
    if (substring_list.size() < 3) // Need at least 3 components
        return;

    float temp[4];
    for (int i = 0; i < 4; i++) {
        if (i == 3 && substring_list.size() == 3) {
            temp[3] = 1.0f; // Set alpha to 1.0 if it's missing
            break;
        }
        try {
            temp[i] = std::stof(substring_list[i]);
        }
        catch (const std::invalid_argument&) { return; }
        catch (const std::out_of_range&    ) { return; }
    }

    for (int i = 0; i < 4; i++)
        dest[i] = temp[i]; // Only copy results to output if no error occurred
}

// The format version of the given user data JSON object must be equal to
// CURRENT_USER_DATA_FORMAT_VERSION.
static gui::GuiState ParseUserDataFromCurrentVersion(const json& user_data) {
    if (!user_data.contains("Settings") || !user_data["Settings"].is_object())
        return {};

    const json *s = &user_data["Settings"];

    gui::GuiState g; // Default-construct to start off with default settings

    TryParseBool(g.show_intro_msg_on_startup, GetNestedValue(s, "ShowIntroductoryMsgOnStartup"));

    TryParseFloat(g.ctrls.IN_mouse_sensitivity, GetNestedValue(s, "Controls", "mouse-sensitivity"));

    TryParseFloat(g.vis.IN_hori_light_angle, GetNestedValue(s, "SunlightDirection"));

    TryParseColor4f(g.vis.IN_col_sky,                 GetNestedValue(s, "WorldGeometryColors", "sky"));
    TryParseColor4f(g.vis.IN_col_water,               GetNestedValue(s, "WorldGeometryColors", "water"));
    TryParseColor4f(g.vis.IN_col_ladders,             GetNestedValue(s, "WorldGeometryColors", "ladder"));
    TryParseColor4f(g.vis.IN_col_player_clip,         GetNestedValue(s, "WorldGeometryColors", "player-clip"));
    TryParseColor4f(g.vis.IN_col_grenade_clip,        GetNestedValue(s, "WorldGeometryColors", "grenade-clip"));
    TryParseColor4f(g.vis.IN_col_trigger_push,        GetNestedValue(s, "WorldGeometryColors", "push-trigger"));
    TryParseColor4f(g.vis.IN_col_solid_displacements, GetNestedValue(s, "WorldGeometryColors", "solid-displacement"));
    TryParseColor4f(g.vis.IN_col_solid_xprops,        GetNestedValue(s, "WorldGeometryColors", "solid-x-prop"));
    TryParseColor4f(g.vis.IN_col_solid_other_brushes, GetNestedValue(s, "WorldGeometryColors", "other-solid-brush"));
    TryParseColor4f(g.vis.IN_col_bump_mine,           GetNestedValue(s, "WorldGeometryColors", "bump-mine"));

    TryParseBool   (g.vis.IN_draw_displacement_edges, GetNestedValue(s, "DisplacementEdges", "show"));
    TryParseColor4f(g.vis.IN_col_solid_disp_boundary, GetNestedValue(s, "DisplacementEdges", "color"));

    std::string geo_vis_mode = "";
    TryParseString(geo_vis_mode, GetNestedValue(s, "GeometryVisualizationModes", "selected-mode"));
    if (geo_vis_mode.compare("geometry-type") == 0)                 g.vis.IN_geo_vis_mode = g.vis.GEO_TYPE;
    if (geo_vis_mode.compare("glidability-of-simulation") == 0)     g.vis.IN_geo_vis_mode = g.vis.GLID_OF_SIMULATION;
    if (geo_vis_mode.compare("glidability-at-specific-speed") == 0) g.vis.IN_geo_vis_mode = g.vis.GLID_AT_SPECIFIC_SPEED;
    if (geo_vis_mode.compare("glidability-of-csgo-session") == 0)   g.vis.IN_geo_vis_mode = g.vis.GLID_OF_CSGO_SESSION;

    const json *geo_vis_mode_specific = GetNestedValue(s, "GeometryVisualizationModes", "ModeSpecificSettings");
    TryParseInt(g.vis.IN_specific_glid_vis_hori_speed, GetNestedValue(geo_vis_mode_specific, "glidability-at-specific-speed", "SimulatedHoriSpeed"));

    TryParseColor4f(g.vis.IN_col_slide_success,     GetNestedValue(s, "SlidabilityColors", "slide-success"));
    TryParseColor4f(g.vis.IN_col_slide_almost_fail, GetNestedValue(s, "SlidabilityColors", "slide-almost-fail"));
    TryParseColor4f(g.vis.IN_col_slide_fail,        GetNestedValue(s, "SlidabilityColors", "slide-fail"));

    TryParseBool   (g.vis.IN_display_hori_vel_text, GetNestedValue(s, "PlayerSpeedDisplay", "show"));
    TryParseFloat  (g.vis.IN_hori_vel_text_size,    GetNestedValue(s, "PlayerSpeedDisplay", "size"));
    TryParseColor4f(g.vis.IN_col_hori_vel_text,     GetNestedValue(s, "PlayerSpeedDisplay", "color"));
    TryParseFloat  (g.vis.IN_hori_vel_text_pos.x(), GetNestedValue(s, "PlayerSpeedDisplay", "position_x"));
    TryParseFloat  (g.vis.IN_hori_vel_text_pos.y(), GetNestedValue(s, "PlayerSpeedDisplay", "position_y"));

    TryParseColor4f(g.vis.IN_crosshair_col,       GetNestedValue(s, "Crosshair", "color"));
    TryParseFloat  (g.vis.IN_crosshair_scale,     GetNestedValue(s, "Crosshair", "scale"));
    TryParseFloat  (g.vis.IN_crosshair_length,    GetNestedValue(s, "Crosshair", "length"));
    TryParseFloat  (g.vis.IN_crosshair_thickness, GetNestedValue(s, "Crosshair", "thickness"));
    TryParseFloat  (g.vis.IN_crosshair_gap,       GetNestedValue(s, "Crosshair", "gap"));

    std::string gamecfg_game_mode = "";
    TryParseString(gamecfg_game_mode, GetNestedValue(s, "GameConfig", "game-mode"));
    if (gamecfg_game_mode == "csgo-danger-zone") g.game_cfg.IN_game_mode = sim::CsgoConfig::GameMode::DANGER_ZONE;
    if (gamecfg_game_mode == "csgo-competitive") g.game_cfg.IN_game_mode = sim::CsgoConfig::GameMode::COMPETITIVE;

    TryParseBool(g.game_cfg.IN_enable_consistent_bumpmine_activations, GetNestedValue(s, "GameConfig", "enable-bump-activation-fix"));
    TryParseBool(g.game_cfg.IN_enable_consistent_rampslides,           GetNestedValue(s, "GameConfig", "enable-rampslide-fix"));

    TryParseBool(g.game_cfg.IN_loadout.has_exojump, GetNestedValue(s, "GameConfig", "PlayerEquipment", "exojump"));

    using Weapon = sim::Entities::Player::Loadout::Weapon;
    std::string active_weapon = "";
    TryParseString(active_weapon, GetNestedValue(s, "GameConfig", "PlayerEquipment", "active-weapon"));
    if (active_weapon == "fists"    ) g.game_cfg.IN_loadout.active_weapon = Weapon::Fists;
    if (active_weapon == "knife"    ) g.game_cfg.IN_loadout.active_weapon = Weapon::Knife;
    if (active_weapon == "bump-mine") g.game_cfg.IN_loadout.active_weapon = Weapon::BumpMine;
    if (active_weapon == "taser"    ) g.game_cfg.IN_loadout.active_weapon = Weapon::Taser;
    if (active_weapon == "xm1014"   ) g.game_cfg.IN_loadout.active_weapon = Weapon::XM1014;

    bool nonactive_fists    = false; TryParseBool(nonactive_fists,    GetNestedValue(s, "GameConfig", "PlayerEquipment", "non_active_fists"));
    bool nonactive_knife    = false; TryParseBool(nonactive_knife,    GetNestedValue(s, "GameConfig", "PlayerEquipment", "non_active_knife"));
    bool nonactive_bumpmine = false; TryParseBool(nonactive_bumpmine, GetNestedValue(s, "GameConfig", "PlayerEquipment", "non_active_bump-mine"));
    bool nonactive_taser    = false; TryParseBool(nonactive_taser,    GetNestedValue(s, "GameConfig", "PlayerEquipment", "non_active_taser"));
    bool nonactive_xm1014   = false; TryParseBool(nonactive_xm1014,   GetNestedValue(s, "GameConfig", "PlayerEquipment", "non_active_xm1014"));
    g.game_cfg.IN_loadout.non_active_weapons.set(Weapon::Fists,    nonactive_fists);
    g.game_cfg.IN_loadout.non_active_weapons.set(Weapon::Knife,    nonactive_knife);
    g.game_cfg.IN_loadout.non_active_weapons.set(Weapon::BumpMine, nonactive_bumpmine);
    g.game_cfg.IN_loadout.non_active_weapons.set(Weapon::Taser,    nonactive_taser);
    g.game_cfg.IN_loadout.non_active_weapons.set(Weapon::XM1014,   nonactive_xm1014);
    // Non-active weapon list must not include the active weapon
    g.game_cfg.IN_loadout.non_active_weapons.set(g.game_cfg.IN_loadout.active_weapon, false);

    std::string window_mode = "";
    TryParseString(window_mode, GetNestedValue(s, "VideoSettings", "WindowSettings", "selected-mode"));
    if (window_mode.compare("windowed") == 0)            g.video.IN_window_mode = g.video.WINDOWED;
    if (window_mode.compare("fullscreen-windowed") == 0) g.video.IN_window_mode = g.video.FULLSCREEN_WINDOWED;

    const json *window_mode_specific = GetNestedValue(s, "VideoSettings", "WindowSettings", "ModeSpecificSettings");
    // CAREFUL! Before this index is first used, it must be checked that it's a valid display index.
    TryParseInt(g.video.IN_selected_display_idx, GetNestedValue(window_mode_specific, "fullscreen-windowed", "selected-display-index"));

    TryParseBool(g.video.IN_vsync_enabled,   GetNestedValue(s, "VideoSettings", "FPSLimit", "enable-vsync"));
    TryParseUInt(g.video.IN_min_loop_period, GetNestedValue(s, "VideoSettings", "FPSLimit", "min-loop-period-when-vsync-disabled"));

    TryParseBool (g.video.IN_use_custom_fov,          GetNestedValue(s, "VideoSettings", "CustomFOV", "use-custom-fov"));
    TryParseFloat(g.video.IN_custom_vert_fov_degrees, GetNestedValue(s, "VideoSettings", "CustomFOV", "custom-vertical-fov"));

#ifndef DZSIM_WEB_PORT
    TryParseBool (g.video.IN_overlay_mode_enabled, GetNestedValue(s, "VideoSettings", "OverlayMode", "enable"));
    TryParseFloat(g.video.IN_overlay_transparency, GetNestedValue(s, "VideoSettings", "OverlayMode", "transparency"));
#endif

    TryParseInt(g.video.IN_user_gui_scaling_factor_pct, GetNestedValue(s, "VideoSettings", "GUI", "size"));

    return g;
}

// Returns the JSON object that will be stored under the "UserData" key in the
// save file.
static json ConvertUserSettingsToUserDataJson(const gui::GuiState& gui_state) {
    json user_data = json::object();

    user_data["Settings"] = json::object();
    json& settings = user_data["Settings"];

    settings["ShowIntroductoryMsgOnStartup"] = gui_state.show_intro_msg_on_startup;

    settings["Controls"]["mouse-sensitivity"] = gui_state.ctrls.IN_mouse_sensitivity;

    settings["SunlightDirection"] = gui_state.vis.IN_hori_light_angle;

    settings["WorldGeometryColors"]["sky"               ] = Color4fToStr(gui_state.vis.IN_col_sky);
    settings["WorldGeometryColors"]["water"             ] = Color4fToStr(gui_state.vis.IN_col_water);
    settings["WorldGeometryColors"]["ladder"            ] = Color4fToStr(gui_state.vis.IN_col_ladders);
    settings["WorldGeometryColors"]["player-clip"       ] = Color4fToStr(gui_state.vis.IN_col_player_clip);
    settings["WorldGeometryColors"]["grenade-clip"      ] = Color4fToStr(gui_state.vis.IN_col_grenade_clip);
    settings["WorldGeometryColors"]["push-trigger"      ] = Color4fToStr(gui_state.vis.IN_col_trigger_push);
    settings["WorldGeometryColors"]["solid-displacement"] = Color4fToStr(gui_state.vis.IN_col_solid_displacements);
    settings["WorldGeometryColors"]["solid-x-prop"      ] = Color4fToStr(gui_state.vis.IN_col_solid_xprops);
    settings["WorldGeometryColors"]["other-solid-brush" ] = Color4fToStr(gui_state.vis.IN_col_solid_other_brushes);
    settings["WorldGeometryColors"]["bump-mine"         ] = Color4fToStr(gui_state.vis.IN_col_bump_mine);

    settings["DisplacementEdges"]["show" ] = gui_state.vis.IN_draw_displacement_edges;
    settings["DisplacementEdges"]["color"] = Color4fToStr(gui_state.vis.IN_col_solid_disp_boundary);

    if (gui_state.vis.IN_geo_vis_mode == gui_state.vis.GEO_TYPE)
        settings["GeometryVisualizationModes"]["selected-mode"] = "geometry-type";
    if (gui_state.vis.IN_geo_vis_mode == gui_state.vis.GLID_OF_SIMULATION)
        settings["GeometryVisualizationModes"]["selected-mode"] = "glidability-of-simulation";
    if (gui_state.vis.IN_geo_vis_mode == gui_state.vis.GLID_AT_SPECIFIC_SPEED)
        settings["GeometryVisualizationModes"]["selected-mode"] = "glidability-at-specific-speed";
    if (gui_state.vis.IN_geo_vis_mode == gui_state.vis.GLID_OF_CSGO_SESSION)
        settings["GeometryVisualizationModes"]["selected-mode"] = "glidability-of-csgo-session";

    settings["GeometryVisualizationModes"]["ModeSpecificSettings"] = json::object();
    json& geo_mode_specific = settings["GeometryVisualizationModes"]["ModeSpecificSettings"];
    geo_mode_specific["glidability-at-specific-speed"]["SimulatedHoriSpeed"] = gui_state.vis.IN_specific_glid_vis_hori_speed;

    settings["SlidabilityColors"]["slide-success"    ] = Color4fToStr(gui_state.vis.IN_col_slide_success);
    settings["SlidabilityColors"]["slide-almost-fail"] = Color4fToStr(gui_state.vis.IN_col_slide_almost_fail);
    settings["SlidabilityColors"]["slide-fail"       ] = Color4fToStr(gui_state.vis.IN_col_slide_fail);

    settings["PlayerSpeedDisplay"]["show"      ] = gui_state.vis.IN_display_hori_vel_text;
    settings["PlayerSpeedDisplay"]["size"      ] = gui_state.vis.IN_hori_vel_text_size;
    settings["PlayerSpeedDisplay"]["color"     ] = Color4fToStr(gui_state.vis.IN_col_hori_vel_text);
    settings["PlayerSpeedDisplay"]["position_x"] = gui_state.vis.IN_hori_vel_text_pos.x();
    settings["PlayerSpeedDisplay"]["position_y"] = gui_state.vis.IN_hori_vel_text_pos.y();

    settings["Crosshair"]["color"    ] = Color4fToStr(gui_state.vis.IN_crosshair_col);
    settings["Crosshair"]["scale"    ] = gui_state.vis.IN_crosshair_scale;
    settings["Crosshair"]["length"   ] = gui_state.vis.IN_crosshair_length;
    settings["Crosshair"]["thickness"] = gui_state.vis.IN_crosshair_thickness;
    settings["Crosshair"]["gap"      ] = gui_state.vis.IN_crosshair_gap;

    if (gui_state.game_cfg.IN_game_mode == sim::CsgoConfig::GameMode::DANGER_ZONE) settings["GameConfig"]["game-mode"] = "csgo-danger-zone";
    if (gui_state.game_cfg.IN_game_mode == sim::CsgoConfig::GameMode::COMPETITIVE) settings["GameConfig"]["game-mode"] = "csgo-competitive";

    settings["GameConfig"]["enable-bump-activation-fix"] = gui_state.game_cfg.IN_enable_consistent_bumpmine_activations;
    settings["GameConfig"]["enable-rampslide-fix"      ] = gui_state.game_cfg.IN_enable_consistent_rampslides;

    using Weapon = sim::Entities::Player::Loadout::Weapon;
    const sim::Entities::Player::Loadout& player_loadout = gui_state.game_cfg.IN_loadout;
    settings["GameConfig"]["PlayerEquipment"]["exojump"] = player_loadout.has_exojump;
    if (player_loadout.active_weapon == Weapon::Fists   ) settings["GameConfig"]["PlayerEquipment"]["active-weapon"] = "fists";
    if (player_loadout.active_weapon == Weapon::Knife   ) settings["GameConfig"]["PlayerEquipment"]["active-weapon"] = "knife";
    if (player_loadout.active_weapon == Weapon::BumpMine) settings["GameConfig"]["PlayerEquipment"]["active-weapon"] = "bump-mine";
    if (player_loadout.active_weapon == Weapon::Taser   ) settings["GameConfig"]["PlayerEquipment"]["active-weapon"] = "taser";
    if (player_loadout.active_weapon == Weapon::XM1014  ) settings["GameConfig"]["PlayerEquipment"]["active-weapon"] = "xm1014";
    settings["GameConfig"]["PlayerEquipment"]["non_active_fists"    ] = player_loadout.non_active_weapons[Weapon::Fists];
    settings["GameConfig"]["PlayerEquipment"]["non_active_knife"    ] = player_loadout.non_active_weapons[Weapon::Knife];
    settings["GameConfig"]["PlayerEquipment"]["non_active_bump-mine"] = player_loadout.non_active_weapons[Weapon::BumpMine];
    settings["GameConfig"]["PlayerEquipment"]["non_active_taser"    ] = player_loadout.non_active_weapons[Weapon::Taser];
    settings["GameConfig"]["PlayerEquipment"]["non_active_xm1014"   ] = player_loadout.non_active_weapons[Weapon::XM1014];

    settings["VideoSettings"] = json::object();
    json& video_settings = settings["VideoSettings"];

    if (gui_state.video.IN_window_mode == gui_state.video.WINDOWED)
        video_settings["WindowSettings"]["selected-mode"] = "windowed";
    if (gui_state.video.IN_window_mode == gui_state.video.FULLSCREEN_WINDOWED)
        video_settings["WindowSettings"]["selected-mode"] = "fullscreen-windowed";

    video_settings["WindowSettings"]["ModeSpecificSettings"] = json::object();
    json& window_mode_specific = video_settings["WindowSettings"]["ModeSpecificSettings"];
    window_mode_specific["fullscreen-windowed"]["selected-display-index"] = gui_state.video.IN_selected_display_idx;

    video_settings["FPSLimit"]["enable-vsync"] = gui_state.video.IN_vsync_enabled;
    video_settings["FPSLimit"]["min-loop-period-when-vsync-disabled"] = gui_state.video.IN_min_loop_period;

    video_settings["CustomFOV"]["use-custom-fov"     ] = gui_state.video.IN_use_custom_fov;
    video_settings["CustomFOV"]["custom-vertical-fov"] = gui_state.video.IN_custom_vert_fov_degrees;

#ifndef DZSIM_WEB_PORT
    video_settings["OverlayMode"]["enable"      ] = gui_state.video.IN_overlay_mode_enabled;
    video_settings["OverlayMode"]["transparency"] = gui_state.video.IN_overlay_transparency;
#endif

    video_settings["GUI"]["size"] = gui_state.video.IN_user_gui_scaling_factor_pct;

    return user_data;
}

// Make sure we can read older versions of user data
static json UpgradeUserDataFromOlderToCurrentVersion(const json& older_user_data, int older_version) {
    assert(older_version <= CURRENT_USER_DATA_FORMAT_VERSION);

    json upgraded_user_data = older_user_data;

    if (older_version == CURRENT_USER_DATA_FORMAT_VERSION)
        return older_user_data; // No conversion needed

    // TODO Everytime the user data format changes, perform incremental
    //      user data conversions here!

    // For example: We need to upgrade user data v2 to v5.
    // Step 1: Upgrade user data v2 to v3
    // Step 2: Upgrade user data v3 to v4
    // Step 3: Upgrade user data v4 to v5

    if (older_version == 1) // If upgrading version 1 to version 2
    {
        // DZSimulator doesn't upgrade from version 1 to version 2!
        // (To start fresh with new defaults for settings.)
        // -> Just return user data v2 with default values.
        gui::GuiState default_settings_v2; // Default-construct
        upgraded_user_data = ConvertUserSettingsToUserDataJson(default_settings_v2);
    }

    return upgraded_user_data;
}

Containers::Optional<Containers::String> SavedUserDataHandler::GetAbsoluteSaveFilePath()
{
    // @PORTING Make sure this works not just on Windows

#ifdef DZSIM_WEB_PORT
    return {};
#else
    Containers::Optional<Containers::String> cfg_dir =
        Utility::Path::configurationDirectory("DZSimulator");
    if (!cfg_dir)
        return {};
    return Utility::Path::join(*cfg_dir, "UserData.json");
#endif
}

void SavedUserDataHandler::OpenSaveFileDirectoryInFileExplorer()
{
    // @PORTING Make sure this works not just on Windows

#ifdef DZSIM_WEB_PORT
    return;
#else
    Containers::Optional<Containers::String> file_path = GetAbsoluteSaveFilePath();
    if (!file_path) {
        Utility::Debug{} << DEBUG_ID << "Failed to find save file directory";
        return;
    }

    Containers::StringView dir_path = Utility::Path::split(*file_path).first();
    Containers::String native_dir_path = Utility::Path::toNativeSeparators(dir_path);
    auto path_for_winapi = Utility::Unicode::widen(Containers::StringView(native_dir_path));
    auto operation_str_for_winapi = Utility::Unicode::widen("open");
    ShellExecuteW(NULL, operation_str_for_winapi, path_for_winapi, NULL, NULL,
                  SW_SHOWDEFAULT);
#endif
}

// Returns JSON null value if an error occurs
static json LoadJsonFromFile(Containers::StringView file_path) {
    Utility::Debug{} << DEBUG_ID << "Loading save file: \"" << file_path << "\"";

    Containers::Optional<Containers::String> file_contents =
        Utility::Path::readString(file_path);

    if (file_contents == Containers::NullOpt) {
        Utility::Debug{} << DEBUG_ID << "Failed to read save file.";
        return nullptr; // JSON null value
    }

    // While parsing JSON: Ignore comments and don't throw exceptions
    json root_obj = json::parse(*file_contents, nullptr, false, true);

    if (root_obj.is_discarded()) { // If JSON parse error occurred
        Utility::Debug{} << DEBUG_ID << "Save file is corrupted.";
        return nullptr; // JSON null value
    }

    return root_obj;
}

gui::GuiState SavedUserDataHandler::LoadUserSettingsFromFile()
{
#ifdef DZSIM_WEB_PORT
    return {};
#else
    Containers::Optional<Containers::String> file_path = GetAbsoluteSaveFilePath();
    if (!file_path) {
        Utility::Debug{} << DEBUG_ID << "Default user settings are used since "
                                        "save file location could not be found.";
        return {}; // loading failed -> use default settings
    }

    json save_file_root = LoadJsonFromFile(*file_path);
    if (save_file_root.is_null()) {
        Utility::Debug{} << DEBUG_ID << "Default user settings are used since "
                                        "loading the save file failed.";
        return {}; // loading failed -> use default settings
    }

    if (!save_file_root.contains("UserDataPerFormatVersion") ||
        !save_file_root["UserDataPerFormatVersion"].is_array())
        return {}; // loading failed -> use default settings

    // Find newest compatible user data from the array
    json* latest_compatible_user_data = nullptr;
    int latest_compatible_user_data_version = -1;

    for (auto& arr_entry : save_file_root["UserDataPerFormatVersion"]) {
        if (!arr_entry.contains("UserDataFormatVersion"))            continue;
        if (!arr_entry.contains("UserData"))                         continue;
        if (!arr_entry["UserDataFormatVersion"].is_number_integer()) continue;
        if (!arr_entry["UserData"].is_object())                      continue;

        // Is this entry the newest compatible one yet?
        int format_version = arr_entry["UserDataFormatVersion"].get<int>();
        if (format_version > CURRENT_USER_DATA_FORMAT_VERSION)
            continue; // Not compatible
        if (format_version < latest_compatible_user_data_version)
            continue; // Not newest entry from the array

        latest_compatible_user_data = &arr_entry["UserData"];
        latest_compatible_user_data_version = format_version;
    }

    if (latest_compatible_user_data == nullptr) { // No compatible user data found
        Utility::Debug{} << DEBUG_ID << "No compatible user data found. "
                                        "Default user settings are used.";
        return {}; // loading failed -> use default settings
    }

    // If user data is from an older version, bring it up to the current version
    if (latest_compatible_user_data_version < CURRENT_USER_DATA_FORMAT_VERSION) {
        json upgraded_user_data = UpgradeUserDataFromOlderToCurrentVersion(
            *latest_compatible_user_data,
            latest_compatible_user_data_version
        );
        return ParseUserDataFromCurrentVersion(upgraded_user_data);
    }
    else { // latest_compatible_user_data_version == CURRENT_USER_DATA_FORMAT_VERSION
        return ParseUserDataFromCurrentVersion(*latest_compatible_user_data);
    }
#endif
}

void SavedUserDataHandler::SaveUserSettingsToFile(const gui::GuiState& gui_state)
{
    ZoneScoped;
#ifdef DZSIM_WEB_PORT
    return;
#else
    // Each different version of the user data format gets its own array entry
    // in the save file.
    json save_file_array_entry = json::object();
    save_file_array_entry["UserDataFormatVersion"] = CURRENT_USER_DATA_FORMAT_VERSION;
    save_file_array_entry["UserData"] = ConvertUserSettingsToUserDataJson(gui_state);

    Containers::Optional<Containers::String> file_path = GetAbsoluteSaveFilePath();
    if (!file_path) {
        Utility::Debug{} << DEBUG_ID << "Failed to save user settings since "
            "save file location could not be found.";
        return;
    }

    Utility::Debug{} << DEBUG_ID << "Saving user settings...";

    json save_file_root = LoadJsonFromFile(*file_path);

    bool create_or_overwrite_save_file = false;
    if (save_file_root.is_null()
        || !save_file_root.contains("UserDataPerFormatVersion")
        || !save_file_root["UserDataPerFormatVersion"].is_array())
        create_or_overwrite_save_file = true;

    std::string new_save_file_contents = "";

    if (create_or_overwrite_save_file) {
        // Save data in newly created save file
        Utility::Debug{} << DEBUG_ID << "Save file is missing or corrupt. "
            "Creating a new one.";

        auto result = Utility::Path::split(*file_path);
        Containers::StringView parent_dir = result.first();
        Containers::StringView file_name = result.second();

        // Create parent directory
        if (!Utility::Path::make(parent_dir)) {
            Utility::Debug{} << DEBUG_ID << "Failed to save user settings since "
                "the save file's directory could not be created.";
            return;
        }

        json new_save_file_root = json::object();
        new_save_file_root["#warning1"] =
            "DO NOT MODIFY THIS FILE IF YOU DON'T KNOW WHAT YOU'RE DOING!";
        new_save_file_root["#warning2"] =
            "YOU MIGHT LOSE ALL YOUR SETTINGS AND DATA!";
        new_save_file_root["#description"] = "This file contains user data and "
            "settings used by the application 'Danger Zone Simulator'. For more"
            " info, see https://github.com/lacyyy/DZSimulator";
        new_save_file_root["UserDataPerFormatVersion"] = json::array();
        new_save_file_root["UserDataPerFormatVersion"].push_back(save_file_array_entry);
        new_save_file_contents = new_save_file_root.dump(2);
    }
    else {
        // Add data to "UserDataPerFormatVersion" array inside the save file.
        // If there is already an entry for the current user data format version,
        // overwrite only that entry. If there isn't one, add it.

        // Find array entry with the current user data format version
        json* arr_entry_of_current_version = nullptr;
        for (auto& arr_entry : save_file_root["UserDataPerFormatVersion"]) {
            if (!arr_entry.contains("UserDataFormatVersion"))            continue;
            if (!arr_entry.contains("UserData"))                         continue;
            if (!arr_entry["UserDataFormatVersion"].is_number_integer()) continue;
            if (!arr_entry["UserData"].is_object())                      continue;

            int format_version_of_entry = arr_entry["UserDataFormatVersion"].get<int>();
            if (format_version_of_entry == CURRENT_USER_DATA_FORMAT_VERSION) {
                arr_entry_of_current_version = &arr_entry;
                break;
            }
        }

        if (arr_entry_of_current_version == nullptr) {
            // Entry for current format version doesn't exist yet -> Add one.
            save_file_root["UserDataPerFormatVersion"].push_back(save_file_array_entry);
        }
        else {
            // Entry for current format version already exists -> Overwrite it.
            *arr_entry_of_current_version = save_file_array_entry;
        }
        new_save_file_contents = save_file_root.dump(2);
    }

    // Write changes to save file
    Containers::ArrayView<char> av = {
        new_save_file_contents.data(),
        new_save_file_contents.size(),
    };
    if (!Utility::Path::write(*file_path, av))
        Utility::Debug{} << DEBUG_ID << "Failed to write settings to file: \"" << *file_path << "\"";
    else
        Utility::Debug{} << DEBUG_ID << "Successfully wrote settings to file: \"" << *file_path << "\"";
#endif
}
