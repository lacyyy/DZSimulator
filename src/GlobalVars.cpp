#include <memory>
#include <mutex>

#include <Magnum/Tags.h>

#include "coll/CollidableWorld.h"
#include "gui/GuiState.h"
#include "sim/CsgoConfig.h"

using namespace Magnum;

// -----------------------------

// Protects std::cout
std::mutex g_cout_mutex;

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