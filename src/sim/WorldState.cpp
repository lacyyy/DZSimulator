#include "sim/WorldState.h"

#include <Tracy.hpp>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Functions.h>

#include "GlobalVars.h"
#include "sim/CsgoConstants.h"
#include "sim/CsgoMovement.h"
#include "utils_3d.h"

using namespace Magnum;

using namespace utils_3d;
using namespace coll;
using namespace sim;

WorldState WorldState::Interpolate(const WorldState& stateA, const WorldState& stateB, float phase)
{
    if (phase <= 0.0f) return stateA;
    if (phase >= 1.0f) return stateB;

    WorldState interpState = stateA;

    // NOTE: Player movement state is only partially being interpolated.
    interpState.csgo_mv.m_vecAbsOrigin  += phase * (stateB.csgo_mv.m_vecAbsOrigin  - stateA.csgo_mv.m_vecAbsOrigin);
    interpState.csgo_mv.m_vecViewOffset += phase * (stateB.csgo_mv.m_vecViewOffset - stateA.csgo_mv.m_vecViewOffset);

    // TODO Interpolate Bumpmines (do we need unique IDs for them?)

    return interpState;
}

void WorldState::DoTimeStep(double step_size_sec,
                            std::span<const PlayerInputState> player_input)
{
    ZoneScoped;

    double& timeDelta = step_size_sec; // in seconds

    // Abort if no map is loaded
    if (!g_coll_world)
        return;

    // Conclusions drawn from player input
    bool tryAttack = false;

    // Parse player input, chronologically
    for (const PlayerInputState& pis : player_input) {
        for (auto cmd : pis.inputCommands) {
            // Get counter reference for this input cmd
            unsigned int& inputCmdActiveCount = this->player.inputCmdActiveCount(cmd);

            if (cmd == PlayerInputState::Command::PLUS_ATTACK && inputCmdActiveCount == 0) {
                tryAttack = true;
            }

            // Determine if the cmd is a '+cmd' or a '-cmd'
            bool is_plus_cmd;
            switch (cmd) {
            case PlayerInputState::Command::PLUS_FORWARD:
            case PlayerInputState::Command::PLUS_BACK:
            case PlayerInputState::Command::PLUS_MOVELEFT:
            case PlayerInputState::Command::PLUS_MOVERIGHT:
            case PlayerInputState::Command::PLUS_USE:
            case PlayerInputState::Command::PLUS_JUMP:
            case PlayerInputState::Command::PLUS_DUCK:
            case PlayerInputState::Command::PLUS_SPEED:
            case PlayerInputState::Command::PLUS_ATTACK:
            case PlayerInputState::Command::PLUS_ATTACK2:
                is_plus_cmd = true; break;
            default:
                is_plus_cmd = false; break;
            }

            // Increment or decrement input cmd counter
            if (is_plus_cmd) {
                ++inputCmdActiveCount;
            }
            else if (inputCmdActiveCount != 0) { // Only decrement if counter is greater than zero
                --inputCmdActiveCount;
            }
        }
    }

    // FIXME we want the angles when the bumpmine was thrown, exactly! Confirm that's in CSGO the same way.
    if (!player_input.empty()) {
        // The latest input decides the new viewing angle
        csgo_mv.m_vecViewAngles = {
            player_input.back().viewingAnglePitch,
            player_input.back().viewingAngleYaw,
            0.0f
        };
    }

    // W,A,S,D inputs only take effect if their state was 'pressed' at the start of the tick (i.e. at the end of this tick's input queue)
    bool tryMoveForward = this->player.inputCmdActiveCount_forward   != 0;
    bool tryMoveBack    = this->player.inputCmdActiveCount_back      != 0;
    bool tryMoveLeft    = this->player.inputCmdActiveCount_moveleft  != 0;
    bool tryMoveRight   = this->player.inputCmdActiveCount_moveright != 0;

    // Toggle CSGO and flying movement
    if (this->player.inputCmdActiveCount_attack2 != 0) {
        // ---- FLYING MOVEMENT ----
        Vector3 forward_dir_xy  { Math::cos(Deg{csgo_mv.m_vecViewAngles.y()        }), Math::sin(Deg{csgo_mv.m_vecViewAngles.y()        }), 0.0f };
        Vector3 moveright_dir_xy{ Math::cos(Deg{csgo_mv.m_vecViewAngles.y() - 90.0f}), Math::sin(Deg{csgo_mv.m_vecViewAngles.y() - 90.0f}), 0.0f };

        Vector3 wish_dir_xy = { 0.0f, 0.0f, 0.0f };
        if      (tryMoveForward && !tryMoveBack   ) wish_dir_xy += forward_dir_xy;
        else if (tryMoveBack    && !tryMoveForward) wish_dir_xy -= forward_dir_xy;
        if      (tryMoveRight   && !tryMoveLeft   ) wish_dir_xy += moveright_dir_xy;
        else if (tryMoveLeft    && !tryMoveRight  ) wish_dir_xy -= moveright_dir_xy;
        if (wish_dir_xy.x() == 0.0f && wish_dir_xy.y() == 0.0f) {
            csgo_mv.m_vecVelocity.x() = 0.0f;
            csgo_mv.m_vecVelocity.y() = 0.0f;
        }
        else {
            NormalizeInPlace(wish_dir_xy);

            float WALK_SPEED = 250.0f;
            if (this->player.inputCmdActiveCount_speed) WALK_SPEED *= 12;

            csgo_mv.m_vecVelocity.x() = WALK_SPEED * wish_dir_xy.x();
            csgo_mv.m_vecVelocity.y() = WALK_SPEED * wish_dir_xy.y();
        }

        if (this->player.inputCmdActiveCount_jump != 0) {
            if (this->player.inputCmdActiveCount_speed)
                csgo_mv.m_vecVelocity.z() = 6 * 300.0f;
            else
                csgo_mv.m_vecVelocity.z() = 300.0f;
        }
        else if (this->player.inputCmdActiveCount_duck != 0) {
            csgo_mv.m_vecVelocity.z() = 6 * -300.0f;
        }
        else {
            csgo_mv.m_vecVelocity.z() = 0.0f;
        }

        csgo_mv.m_vecAbsOrigin += timeDelta * csgo_mv.m_vecVelocity;
    }
    else {
        // ---- CSGO MOVEMENT ----

        csgo_mv.m_nButtons = 0;
        if (tryMoveForward)
            csgo_mv.m_nButtons |= IN_FORWARD;
        if (tryMoveBack)
            csgo_mv.m_nButtons |= IN_BACK;
        if (tryMoveLeft)
            csgo_mv.m_nButtons |= IN_MOVELEFT;
        if (tryMoveRight)
            csgo_mv.m_nButtons |= IN_MOVERIGHT;
        if (this->player.inputCmdActiveCount_jump != 0)
            csgo_mv.m_nButtons |= IN_JUMP;
        if (this->player.inputCmdActiveCount_speed != 0)
            csgo_mv.m_nButtons |= IN_SPEED;
        if (this->player.inputCmdActiveCount_duck != 0)
            csgo_mv.m_nButtons |= IN_DUCK;

        csgo_mv.m_flForwardMove = 0.0f;
        if (tryMoveForward) csgo_mv.m_flForwardMove += g_csgo_game_sim_cfg.cl_forwardspeed;
        if (tryMoveBack)    csgo_mv.m_flForwardMove -= g_csgo_game_sim_cfg.cl_backspeed;

        csgo_mv.m_flSideMove = 0.0f;
        if (tryMoveRight) csgo_mv.m_flSideMove += g_csgo_game_sim_cfg.cl_sidespeed;
        if (tryMoveLeft)  csgo_mv.m_flSideMove -= g_csgo_game_sim_cfg.cl_sidespeed;

        // Temporary: On attack input, boost player in looking direction
        if (tryAttack) {
            Vector3 forward;
            AngleVectors(csgo_mv.m_vecViewAngles, &forward);
            csgo_mv.m_vecVelocity += 1400 * forward;
        }

        // -------- start of source-sdk-2013 code --------
        // (taken and modified from source-sdk-2013/<...>/src/game/shared/gamemovement.cpp)
        // (Original code found in ProcessMovement() function)

        // Cropping movement speed scales mv->m_fForwardSpeed etc. globally
        // Once we crop, we don't want to recursively crop again, so we set the crop
        //  flag globally here once per usercmd cycle.
        csgo_mv.m_iSpeedCropped = SPEED_CROPPED_RESET;

        // Init max speed using type of weapon equipped by player
        // TODO: Distinguish between different equipped weapons here
        csgo_mv.m_flMaxSpeed = g_csgo_game_sim_cfg.WEAPON_KNIFE_MAX_PLAYER_SPEED;

        //Debug{} << "m_nButtons = " << csgo_mv.m_nButtons;
        //Debug{} << "m_flForwardMove = " << csgo_mv.m_flForwardMove;
        //Debug{} << "m_flSideMove    = " << csgo_mv.m_flSideMove;
        //Debug{} << "m_vecViewAngles = " << csgo_mv.m_vecViewAngles;
        //Debug{} << "m_vecAbsOrigin  = " << csgo_mv.m_vecAbsOrigin;
        //Debug{} << "m_vecVelocity   = " << csgo_mv.m_vecVelocity;

        csgo_mv.PlayerMove(timeDelta);
        csgo_mv.FinishMove();
        // --------- end of source-sdk-2013 code ---------
    }
}
