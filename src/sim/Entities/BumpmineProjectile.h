#ifndef SIM_ENTITIES_BUMPMINEPROJECTILE_H_
#define SIM_ENTITIES_BUMPMINEPROJECTILE_H_

#include <Magnum/Magnum.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/Tags.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector3.h>

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

        // Set to simulation time point 0 by default
        SimTimePoint next_think{ Magnum::Math::ZeroInit };

        bool detonates_on_next_think = false;
        bool has_detonated = false;

        // Advance this Bump Mine projectile forward in simulation time by the
        // given duration.
        void AdvanceSimulation(SimTimeDur simtime_delta,
                               WorldState& world_of_this_bm);

        BumpmineProjectile() = default;

    private:
        static float GetActivationCheckIntervalInSecs();

        // Returns time in seconds until the next Think() occurs.
        float Think(SimTimeDur simtime_delta, WorldState& world_of_this_bm);
    };

} // namespace sim::Entities
} // namespace sim

#endif // SIM_ENTITIES_BUMPMINEPROJECTILE_H_
