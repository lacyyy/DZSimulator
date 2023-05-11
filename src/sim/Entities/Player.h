#ifndef SIM_ENTITIES_PLAYER_H_
#define SIM_ENTITIES_PLAYER_H_

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Corrade/Utility/Debug.h>

#include "CsgoConstants.h"
#include "GlobalVars.h"
#include "sim/PlayerInputState.h"

namespace sim::Entities {

class Player {
public:
    Magnum::Vector3 position;
    Magnum::Vector3 velocity;
    Magnum::Vector3 angles; // pitch, yaw, roll

    float stamina = CSGO_CVAR_SV_STAMINAMAX;

    bool crouched = false; // TODO: crouch progress?

    unsigned int weaponSlot = 0; // = WEAPON_BUMPMINE; // TODO default init this
    float timeSinceWeaponSwitch_sec = 100.0f; // seconds

    // ---- Player input command states
    // inputCmdActiveCount: Each time +cmd is issued, increment the count.
    //                      Each time -cmd is issued, decrement the count.
    // Only decrement if the count is greater than zero.

    unsigned int inputCmdActiveCount_forward   = 0; // default: W key
    unsigned int inputCmdActiveCount_back      = 0; // default: S key
    unsigned int inputCmdActiveCount_moveleft  = 0; // default: A key
    unsigned int inputCmdActiveCount_moveright = 0; // default: D key
    unsigned int inputCmdActiveCount_use       = 0; // default: E key
    unsigned int inputCmdActiveCount_jump      = 0; // default: Space key
    unsigned int inputCmdActiveCount_duck      = 0; // default: Ctrl key
    unsigned int inputCmdActiveCount_speed     = 0; // default: Shift key
    unsigned int inputCmdActiveCount_attack    = 0; // default: Mouse 1 button
    unsigned int inputCmdActiveCount_attack2   = 0; // default: Mouse 2 button
    
    // --------------------------------

    Player() = default;

    unsigned int& inputCmdActiveCount(PlayerInputState::Command cmd) {
        switch (cmd) {
        case PlayerInputState::Command::PLUS_FORWARD:
        case PlayerInputState::Command::MINUS_FORWARD:
            return inputCmdActiveCount_forward;
        case PlayerInputState::Command::PLUS_BACK:
        case PlayerInputState::Command::MINUS_BACK:
            return inputCmdActiveCount_back;
        case PlayerInputState::Command::PLUS_MOVELEFT:
        case PlayerInputState::Command::MINUS_MOVELEFT:
            return inputCmdActiveCount_moveleft;
        case PlayerInputState::Command::PLUS_MOVERIGHT:
        case PlayerInputState::Command::MINUS_MOVERIGHT:
            return inputCmdActiveCount_moveright;
        case PlayerInputState::Command::PLUS_USE:
        case PlayerInputState::Command::MINUS_USE:
            return inputCmdActiveCount_use;
        case PlayerInputState::Command::PLUS_JUMP:
        case PlayerInputState::Command::MINUS_JUMP:
            return inputCmdActiveCount_jump;
        case PlayerInputState::Command::PLUS_DUCK:
        case PlayerInputState::Command::MINUS_DUCK:
            return inputCmdActiveCount_duck;
        case PlayerInputState::Command::PLUS_SPEED:
        case PlayerInputState::Command::MINUS_SPEED:
            return inputCmdActiveCount_speed;
        case PlayerInputState::Command::PLUS_ATTACK:
        case PlayerInputState::Command::MINUS_ATTACK:
            return inputCmdActiveCount_attack;
        case PlayerInputState::Command::PLUS_ATTACK2:
        case PlayerInputState::Command::MINUS_ATTACK2:
            return inputCmdActiveCount_attack2;
        }
        ACQUIRE_COUT(Magnum::Error{} << "[ERROR] Player::inputCmdActiveCount() Unknown cmd! Forgot a switch case?";)
        std::terminate();
        return inputCmdActiveCount_forward; // irrelevant
    }
};

} // namespace sim::Entities

#endif // SIM_ENTITIES_PLAYER_H_
