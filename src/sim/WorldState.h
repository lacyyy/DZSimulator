#ifndef SIM_WORLDSTATE_H_
#define SIM_WORLDSTATE_H_

#include <vector>
#include <span>

#include "sim/CsgoMovement.h"
#include "sim/Entities/BumpmineProjectile.h"
#include "sim/Entities/Player.h"
#include "sim/PlayerInputState.h"

namespace sim {

class WorldState {
public:
    // World state
    CsgoMovement csgo_mv;
    Entities::Player player;
    std::vector<Entities::BumpmineProjectile> bumpmine_projectiles;


    // ----------------------------------------

    static WorldState Interpolate(const WorldState& stateA,
        const WorldState& stateB, float phase);

    // Advance this world state with the given player input forward in time by
    // the given duration
    void DoTimeStep(double step_size_sec,
                    std::span<const PlayerInputState> player_input);

};

} // namespace sim

#endif // SIM_WORLDSTATE_H_
