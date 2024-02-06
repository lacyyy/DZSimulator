#ifndef SIM_PLAYERINPUTSTATE_H_
#define SIM_PLAYERINPUTSTATE_H_

#include <vector>

#include "sim/Sim.h"

namespace sim {

class PlayerInputState {
public:
    // When changing this enum, make sure to update AllMinusCommands() and Player class!
    enum class Command {
        PLUS_FORWARD,    // csgo console command '+forward'
        PLUS_BACK,       // csgo console command '+back'
        PLUS_MOVELEFT,   // csgo console command '+moveleft'
        PLUS_MOVERIGHT,  // csgo console command '+moveright'
        PLUS_USE,        // csgo console command '+use'
        PLUS_JUMP,       // csgo console command '+jump'
        PLUS_DUCK,       // csgo console command '+duck'
        PLUS_SPEED,      // csgo console command '+speed'
        PLUS_ATTACK,     // csgo console command '+attack'
        PLUS_ATTACK2,    // csgo console command '+attack2'

        MINUS_FORWARD,   // csgo console command '-forward'
        MINUS_BACK,      // csgo console command '-back'
        MINUS_MOVELEFT,  // csgo console command '-moveleft'
        MINUS_MOVERIGHT, // csgo console command '-moveright'
        MINUS_USE,       // csgo console command '-use'
        MINUS_JUMP,      // csgo console command '-jump'
        MINUS_DUCK,      // csgo console command '-duck'
        MINUS_SPEED,     // csgo console command '-speed'
        MINUS_ATTACK,    // csgo console command '-attack'
        MINUS_ATTACK2,   // csgo console command '-attack2'
    };

    // ----------------------------------------------------------------

    sim::Clock::time_point time; // When this input was created

    std::vector<Command> inputCommands;
    unsigned int weaponSlot = 0; // = WEAPON_BUMPMINE; // TODO default init this
    float viewingAnglePitch = 0.0f;
    float viewingAngleYaw = 0.0f;

    // ----------------------------------------------------------------

    PlayerInputState() = default; // Default player inputs

    static std::vector<Command> AllMinusCommands() {
        return {
            Command::MINUS_FORWARD,
            Command::MINUS_BACK,
            Command::MINUS_MOVELEFT,
            Command::MINUS_MOVERIGHT,
            Command::MINUS_USE,
            Command::MINUS_JUMP,
            Command::MINUS_DUCK,
            Command::MINUS_SPEED,
            Command::MINUS_ATTACK,
            Command::MINUS_ATTACK2,
        };
    }

};

} // namespace sim

#endif // SIM_PLAYERINPUTSTATE_H_
