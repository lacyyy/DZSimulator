#ifndef SIM_ENTITIES_PLAYER_H_
#define SIM_ENTITIES_PLAYER_H_

#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/BitVector.h>
#include <Magnum/Math/Tags.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector3.h>

#include "sim/Sim.h"

namespace sim::Entities {

class Player {
public:
    class Loadout {
    public:
        enum Weapon {
            Fists = 0,
            Knife,
            BumpMine,
            Taser,
            XM1014,
            TOTAL_COUNT // Must be the last enum value!
        };

        // Which weapon the player is currently holding.
        Weapon active_weapon;

        // Flags indicating which weapons are carried by the player, _excluding_
        // the active weapon.
        // A weapon's enum value signifies its bit position in this BitVector.
        using WeaponList = Magnum::Math::BitVector<Weapon::TOTAL_COUNT>;
        WeaponList non_active_weapons;

        bool has_exojump;

        // --------

        bool operator==(const Loadout& other) const {
            return this->active_weapon      == other.active_weapon &&
                   this->non_active_weapons == other.non_active_weapons &&
                   this->has_exojump        == other.has_exojump;
        }
        bool operator!=(const Loadout& other) const {
            return !operator==(other);
        }

        Loadout(bool has_exo, Weapon active,
                const std::vector<Weapon>& non_active_list)
            : active_weapon{ active }
            , non_active_weapons{ Magnum::Math::ZeroInit }
            , has_exojump{ has_exo }
        {
            for (Weapon non_active_weapon : non_active_list)
                if (non_active_weapon != active)
                    non_active_weapons.set(non_active_weapon, true);
        }
    };

    Loadout loadout = Loadout(false, Loadout::Weapon::XM1014, {});

    // Set to simulation time point 0 by default
    SimTimePoint next_primary_attack{ Magnum::Math::ZeroInit };
};

} // namespace sim::Entities

#endif // SIM_ENTITIES_PLAYER_H_
