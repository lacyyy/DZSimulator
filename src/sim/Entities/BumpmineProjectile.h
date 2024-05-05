#ifndef SIM_ENTITIES_BUMPMINEPROJECTILE_H_
#define SIM_ENTITIES_BUMPMINEPROJECTILE_H_

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Quaternion.h>

#include "sim/Sim.h"

namespace sim {

class WorldState;

namespace Entities {

    class BumpmineProjectile {
    public:
        static size_t GenerateNewUniqueID();
        size_t unique_id = -1;

        bool is_on_surface = false;
        Magnum::Quaternion inv_rotation_on_surface; // Must be normalized!

        Magnum::Vector3 position;
        Magnum::Vector3 velocity;
        Magnum::Vector3 angles; // pitch, yaw, roll

        TickID next_think = 0;

        bool detonates_on_next_think = false;
        bool has_detonated = false;

        // progress values ranging from 0.0 to 1.0
        float armProgress; // arm delay
        float detonateProgress; // detonate delay

        // subsequent_tick_id is the ID of the game tick that is reached _after_
        // performing this time step!
        void DoTimeStep(
            WorldState& world_of_this_bm,
            double step_size_sec,
            size_t subsequent_tick_id);

        BumpmineProjectile() = default;

    private:
        float GetActivationCheckIntervalInSecs();

        // Returns time in seconds until the next Think() occurs.
        float Think(
            WorldState& world_of_this_bm,
            double step_size_sec,
            size_t cur_tick_id);
    };

} // namespace sim::Entities
} // namespace sim

#endif // SIM_ENTITIES_BUMPMINEPROJECTILE_H_
