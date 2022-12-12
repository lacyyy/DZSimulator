#ifndef SIM_ENTITIES_BUMPMINEPROJECTILE_H_
#define SIM_ENTITIES_BUMPMINEPROJECTILE_H_

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

namespace sim::Entities {

    class BumpmineProjectile {
    public:
        Magnum::Vector3 position;
        Magnum::Vector3 velocity;
        Magnum::Vector3 angles; // pitch, yaw, roll

        // progress values ranging from 0.0 to 1.0
        float armProgress; // arm delay
        float detonateProgress; // detonate delay



        BumpmineProjectile() = default;
    };

} // namespace sim::Entities

#endif // SIM_ENTITIES_BUMPMINEPROJECTILE_H_
