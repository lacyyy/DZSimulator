#ifndef GLOBALVARS_H_
#define GLOBALVARS_H_

#include <memory>

#include "coll/CollidableWorld.h"
#include "gui/GuiState.h"
#include "sim/CsgoConfig.h"

// -----------------------------

// For initialization, some global variables need info that isn't immediately
// available. This function must be called at some point during app startup.
void PerformLateGlobalVarsInit(const gui::GuiState& gui_state);

// -----------------------------

// World data
extern std::shared_ptr<coll::CollidableWorld> g_coll_world;

extern sim::CsgoConfig g_csgo_game_sim_cfg;


#endif // GLOBALVARS_H_
