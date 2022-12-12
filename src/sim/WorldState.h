#ifndef SIM_WORLDSTATE_H_
#define SIM_WORLDSTATE_H_

#include <vector>

#include "sim/Entities/BumpmineProjectile.h"
#include "sim/Entities/Player.h"
#include "sim/PlayerInputState.h"
#include "sim/Sim.h"

namespace sim {

class WorldState {
public:
    // What time this world state refers to
    Clock::time_point time;
    // What time the latest player input was created, which affected this world state
    // -> With this, the client knows which inputs to use for world state prediction
    Clock::time_point latest_player_input_time;

    // World state
    Entities::Player player;
    std::vector<Entities::BumpmineProjectile> bumpmine_projectiles;


    // ----------------------------------------

    static WorldState Interpolate(const WorldState& stateA,
        const WorldState& stateB, float phase);

    // Advance this world state with the given player input forward in time by
    // the given duration
    void DoTimeStep(double stepSize_sec, const std::vector<PlayerInputState>& playerInput);
    void DoTimeStep(double stepSize_sec,
        std::vector<PlayerInputState>::const_iterator playerInputBeginIt,
        std::vector<PlayerInputState>::const_iterator playerInputEndIt);


};

} // namespace sim

#endif // SIM_WORLDSTATE_H_
