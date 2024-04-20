#include "GlobalVars.h"

#include <Magnum/Tags.h>

using namespace Magnum;

// -----------------------------

// World data for easy access
std::shared_ptr<coll::CollidableWorld> g_coll_world;

// Parameters of the CSGO game simulation (game mode, ConVars, ...)
sim::CsgoConfig g_csgo_game_sim_cfg( NoInit ); // Must be initialized later!

// -----------------------------

// This is called during app startup
void PerformLateGlobalVarsInit(const gui::GuiState& gui_state)
{
    // Init game config depending on user's settings
    switch(gui_state.game_cfg.IN_game_mode) {
        case sim::CsgoConfig::GameMode::DANGER_ZONE:
            g_csgo_game_sim_cfg = sim::CsgoConfig(InitWithDzDefaults);
            break;
        case sim::CsgoConfig::GameMode::COMPETITIVE:
            g_csgo_game_sim_cfg = sim::CsgoConfig(InitWithCompDefaults);
            break;
        default:
            assert(0);
            break;
    }
}