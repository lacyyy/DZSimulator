#include "CsgoGame.h"

#include <cassert>
#include <chrono>
#include <utility>
#include <vector>

#include <Tracy.hpp>

#include "sim/PlayerInputState.h"
#include "sim/Sim.h"
#include "sim/WorldState.h"

using namespace sim;
using namespace Magnum;
using microseconds = std::chrono::microseconds;
using nanoseconds  = std::chrono::nanoseconds;

// Should be enabled, toggleable for debugging purposes
const bool ENABLE_INTERPOLATION_OF_DRAWN_WORLDSTATE = true;

CsgoGame::CsgoGame()
    : m_sim_step_size_in_secs{ 0.0f } // Indicates that game hasn't been started
    , m_game_timescale{ 0.0f }
    , m_realtime_game_tick_interval{}
    , m_game_start{}
    , m_prev_finalized_game_tick_id{0 }
    , m_prev_finalized_game_tick{}
    , m_inputs_since_prev_finalized_game_tick{}
    , m_prev_predicted_game_tick{}
    , m_prev_drawn_worldstate{}
    , m_prev_drawn_worldstate_timepoint{}
{
}

bool CsgoGame::HasBeenStarted() {
    return m_sim_step_size_in_secs > 0.0f;
}

void CsgoGame::Start(float sim_step_size_in_secs, float game_timescale,
                     const WorldState& initial_worldstate)
{
    assert(sim_step_size_in_secs > 0.0f);
    assert(game_timescale > 0.0f);

    auto current_time = sim::Clock::now();

    long long realtime_game_tick_interval_us =
        (1e6f * sim_step_size_in_secs) / game_timescale;

    m_sim_step_size_in_secs = sim_step_size_in_secs;
    m_game_timescale = game_timescale;
    m_realtime_game_tick_interval = microseconds{ realtime_game_tick_interval_us };
    m_game_start = current_time;

    m_prev_finalized_game_tick_id = 0;
    m_prev_finalized_game_tick = initial_worldstate;
    m_inputs_since_prev_finalized_game_tick.clear();

    // Simulate one game tick to get a possible future game tick
    m_prev_predicted_game_tick = initial_worldstate;
    m_prev_predicted_game_tick.DoTimeStep(sim_step_size_in_secs, {});

    m_prev_drawn_worldstate = initial_worldstate;
    m_prev_drawn_worldstate_timepoint = current_time;
}

WorldState CsgoGame::ProcessNewPlayerInput(const PlayerInputState& new_input)
{
    ZoneScoped;

    if (!HasBeenStarted()) {
        assert(0);
        return WorldState();
    }

    // Note: We define that a player input affects a game tick if:
    //       (player_input_sample_timepoint <= game_tick_timepoint)

#ifndef NDEBUG
    // New input must have been sampled after all previously passed inputs.
    // New input is allowed to have been sampled at identical timepoints.
    for (const PlayerInputState& other : m_inputs_since_prev_finalized_game_tick)
        assert(new_input.time >= other.time);
#endif

    sim::Clock::time_point cur_time = new_input.time;

    // @Optimization We should drop game ticks if the user's machine struggles
    //               to keep up. How does the Source engine do it?

    // Step 1: Find ID of game tick that directly precedes the new player input.
    size_t directly_preceding_game_tick_id = m_prev_finalized_game_tick_id;
    while (GetTimePointOfGameTick(directly_preceding_game_tick_id+1) < cur_time)
        directly_preceding_game_tick_id++;

    // Step 2: Advance game simulation up to and including directly preceding
    //         game tick, if not already done.

    // Preliminary action for this: If we need to advance by one or more new
    // game ticks, advance m_prev_finalized_game_tick already to the first next
    // game tick by simply copying the previously predicted game tick!
    // This is possible because it's certain that no new player inputs relevant
    // to that first tick advancement were generated.
    if (m_prev_finalized_game_tick_id < directly_preceding_game_tick_id) {
        m_prev_finalized_game_tick = std::move(m_prev_predicted_game_tick);
        m_prev_finalized_game_tick_id++;
        m_inputs_since_prev_finalized_game_tick.clear();
    }

    // Next, possibly advance by additional # of game ticks.
    // These additional game ticks have passed completely without any calls to
    // ProcessNewPlayerInput(), so they receive no player input.
    while (m_prev_finalized_game_tick_id < directly_preceding_game_tick_id) {
        m_prev_finalized_game_tick.DoTimeStep(m_sim_step_size_in_secs, {});
        m_prev_finalized_game_tick_id++;
    }
    // NOTE: m_prev_predicted_game_tick has now become invalid if we advanced by
    //       one or more ticks.

    // Step 3: Predict the next future game tick using the new player input (and
    //         possibly previous inputs of the current unfinalized game tick).
    m_inputs_since_prev_finalized_game_tick.push_back(new_input);

    WorldState predicted_next_game_tick = m_prev_finalized_game_tick;
    predicted_next_game_tick.DoTimeStep(m_sim_step_size_in_secs,
                                        m_inputs_since_prev_finalized_game_tick);

    sim::Clock::time_point next_game_tick_timepoint =
        GetTimePointOfGameTick(m_prev_finalized_game_tick_id + 1);

    // Step 4: Determine current drawn world state by interpolating between
    //         previously drawn world state and the predicted next game tick.
    WorldState cur_drawn_worldstate;
    if (ENABLE_INTERPOLATION_OF_DRAWN_WORLDSTATE) {
        // @Optimization We could measure the current time again after the game
        //               tick simulations and use it for interpolation.
        //               This might help with responsiveness on low-end machines.
        //               CAUTION: This might lead to exceeding the interpolation
        //                        range! Handle that.
        auto interpRange = next_game_tick_timepoint - m_prev_drawn_worldstate_timepoint;
        auto interpStep  =                 cur_time - m_prev_drawn_worldstate_timepoint;
        float interpRange_ns = std::chrono::duration_cast<nanoseconds>(interpRange).count();
        float interpStep_ns  = std::chrono::duration_cast<nanoseconds>(interpStep ).count();
        if (interpRange_ns == 0.0f) {
            cur_drawn_worldstate = predicted_next_game_tick;
        } else {
            float phase = interpStep_ns / interpRange_ns;
            cur_drawn_worldstate = WorldState::Interpolate(m_prev_drawn_worldstate,
                                                           predicted_next_game_tick,
                                                           phase);
        }
    }
    else { // ENABLE_INTERPOLATION_OF_DRAWN_WORLDSTATE == false
        // Instead of interpolating, just draw the last finalized game tick
        cur_drawn_worldstate = m_prev_finalized_game_tick;
    }

    // Remember for future ProcessNewPlayerInput() calls
    m_prev_predicted_game_tick        = std::move(predicted_next_game_tick);
    m_prev_drawn_worldstate           = cur_drawn_worldstate;
    m_prev_drawn_worldstate_timepoint = cur_time;

    return cur_drawn_worldstate;
}

sim::Clock::time_point CsgoGame::GetTimePointOfGameTick(size_t tick_id)
{
    assert(HasBeenStarted());
    return m_game_start + tick_id * m_realtime_game_tick_interval;
}
