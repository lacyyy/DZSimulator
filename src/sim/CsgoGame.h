#ifndef SIM_CSGOGAME_H_
#define SIM_CSGOGAME_H_

#include <vector>

#include <Magnum/Math/Time.h>

#include "common.h"
#include "sim/PlayerInputState.h"
#include "sim/Sim.h"
#include "sim/WorldState.h"

namespace sim {

// Responsible for simulating game ticks of a CSGO game/match using the player's
// input.
// Also responsible for producing responsive worldstates suitable for being
// drawn to the screen, similar to how the actual CSGO client performs
// worldstate prediction to display a worldstate that feels responsive.
// In other words, this class represents a CSGO server and client simulating the
// game. No asynchronicity is utilized.
class CsgoGame {
public:
    // Initialize in "not started" state
    CsgoGame();

    // Whether a CSGO game has been started using the Start() method
    bool HasBeenStarted();

    // (Re-)Starts the game simulation at the given world state with the given
    // parameters. The given simulation time step size must be greater than 0 !
    void Start(SimTimeDur simtime_step_size, float simtime_scale,
               const WorldState& initial_worldstate);

    // Process newly generated player input and possibly advance the game
    // simulation. Given player input must be new (i.e. have a time point that
    // chronologically comes after all previously passed inputs' time point)!
    // This method must be called after this CSGO game was started!
    void ProcessNewPlayerInput(const PlayerInputState& new_player_input);

    // Returns the current actual (non-interpolated) state of the game
    // simulation, computed by a recent call to ProcessNewPlayerInput().
    // This method must be called after this CSGO game was started!
    // CAUTION: Returned reference stays valid until this CsgoGame instance is
    //          destroyed, or ProcessNewPlayerInput() or Start() is called!
    const WorldState& GetLatestActualWorldState();

    // Returns the current drawable (partially interpolated) state of the game
    // simulation, computed by the most recent call to ProcessNewPlayerInput().
    // It's intended to be used for drawing the game simulation to the screen.
    // This method must be called after this CSGO game was started!
    // CAUTION: Returned reference stays valid until this CsgoGame instance is
    //          destroyed, or ProcessNewPlayerInput() or Start() is called!
    const WorldState& GetLatestDrawableWorldState();

private:
    // Returns realtime time point of a game tick. Game must have been started!
    WallClock::time_point GetGameTickRealTimePoint(size_t tick_id);

private:
    SimTimeDur m_simtime_step_size; // Simulation time increase every game tick
    WallClock::duration m_realtime_game_tick_interval;

    // When the game was last (re-)started.
    // Realtime time point of tick 0's worldstate.
    WallClock::time_point m_realtime_game_start;

    // The most recently finalized game tick (The current state of the simulation)
    size_t     m_prev_finalized_game_tick_id; // Incremented each game tick
    WorldState m_prev_finalized_game_tick;

    // Player inputs since the most recently finalized game tick, in
    // chronological order.
    std::vector<PlayerInputState> m_inputs_since_prev_finalized_game_tick;

    // The most recent prediction of the next game tick (that comes after
    // m_prev_finalized_game_tick)
    WorldState m_prev_predicted_game_tick;

    // The most recent drawable worldstate and its realtime time point.
    WorldState            m_prev_drawable_worldstate;
    WallClock::time_point m_prev_drawable_worldstate_timepoint;
};

} // namespace sim

#endif // SIM_CSGOGAME_H_
