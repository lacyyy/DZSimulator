#include "WorldState.h"

#include <chrono>
#include <iterator>
#include <thread> // FIXME remove this after debug code was removed

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Functions.h>
#include <Corrade/Utility/Debug.h>

#include "GlobalVars.h"
#include "CSGOConstants.h"

using namespace Magnum;

using namespace sim;

WorldState WorldState::Interpolate(const WorldState& stateA, const WorldState& stateB, float phase)
{
    if (phase <= 0.0f) return stateA;
    if (phase >= 1.0f) return stateB;
    
    WorldState interpState = stateA;

    interpState.time += std::chrono::duration_cast<std::chrono::nanoseconds>(phase * (stateB.time - stateA.time));
    interpState.latest_player_input_time = {}; // irrelevant, interpolated world states aren't used for server world state prediction

    interpState.player.position += phase * (stateB.player.position - stateA.player.position);

    // TODO Interpolate Bumpmines (do we need unique IDs for them?)

    return interpState;
}

void WorldState::DoTimeStep(double stepSize_sec, const std::vector<PlayerInputState>& playerInput)
{
    DoTimeStep(stepSize_sec, playerInput.begin(), playerInput.end());
}

void WorldState::DoTimeStep(double stepSize_sec,
        std::vector<PlayerInputState>::const_iterator playerInputBeginIt,
        std::vector<PlayerInputState>::const_iterator playerInputEndIt) {
    double& timeDelta = stepSize_sec; // in seconds

    // Caution: This time advancement does not account for simulation time scale!
    this->time += std::chrono::nanoseconds{ (long long)(1e9 * timeDelta) };

    // Conclusions drawn from player input
    bool tryJump = false;
    bool walkMode = false;

    // Parse player input, chronologically
    for (auto pisIt = playerInputBeginIt; pisIt != playerInputEndIt; ++pisIt) {
        for (auto cmd : pisIt->inputCommands) {
            // Get counter reference for this input cmd
            unsigned int& inputCmdActiveCount = this->player.inputCmdActiveCount(cmd);

            // Try to jump if cmd is '+jump' and the jump key wasn't pressed before
            if (cmd == PlayerInputState::Command::PLUS_JUMP && inputCmdActiveCount == 0) {
                tryJump = true;
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
    if (playerInputBeginIt != playerInputEndIt) { // If playerInput container isn't empty
        // The latest input decides the new viewing angle
        auto latestPis = std::prev(playerInputEndIt);
        this->latest_player_input_time = latestPis->time; // Save time of latest input that affected this world state
        this->player.angles = { latestPis->viewingAnglePitch, latestPis->viewingAngleYaw, 0.0f };
    }

    // W,A,S,D inputs only take effect if their state was 'pressed' at the start of the tick (i.e. at the end of this tick's input queue)
    bool tryMoveForward = this->player.inputCmdActiveCount_forward != 0;
    bool tryMoveBack = this->player.inputCmdActiveCount_back != 0;
    bool tryMoveLeft = this->player.inputCmdActiveCount_moveleft != 0;
    bool tryMoveRight = this->player.inputCmdActiveCount_moveright != 0;

    Vector3 forward_dir_xy{ Math::cos(Deg{this->player.angles.y()        }), Math::sin(Deg{this->player.angles.y()        }), 0.0f };
    Vector3 moveright_dir_xy{ Math::cos(Deg{this->player.angles.y() - 90.0f}), Math::sin(Deg{this->player.angles.y() - 90.0f}), 0.0f };

    Vector3 wish_dir_xy = { 0.0f, 0.0f, 0.0f };
    if (tryMoveForward && !tryMoveBack) wish_dir_xy += forward_dir_xy;
    else if (tryMoveBack && !tryMoveForward) wish_dir_xy -= forward_dir_xy;
    if (tryMoveRight && !tryMoveLeft) wish_dir_xy += moveright_dir_xy;
    else if (tryMoveLeft && !tryMoveRight) wish_dir_xy -= moveright_dir_xy;
    if (wish_dir_xy.x() == 0.0f && wish_dir_xy.y() == 0.0f) {
        this->player.velocity.x() = 0.0f;
        this->player.velocity.y() = 0.0f;
    }
    else {
        wish_dir_xy = wish_dir_xy.normalized();

        Float WALK_SPEED = 250.0f;
        if (this->player.inputCmdActiveCount_speed) WALK_SPEED *= 12;

        this->player.velocity.x() = WALK_SPEED * wish_dir_xy.x();
        this->player.velocity.y() = WALK_SPEED * wish_dir_xy.y();
    }

    if (this->player.inputCmdActiveCount_jump != 0) {
        if (this->player.inputCmdActiveCount_speed)
            this->player.velocity.z() = 6 * 300.0f;
        else
            this->player.velocity.z() = 300.0f;
    }
    else if (this->player.inputCmdActiveCount_duck != 0) {
        this->player.velocity.z() = 6 * -300.0f;
    }
    else {
        this->player.velocity.z() = 0.0f;
    }


    static Float baseZ = this->player.position.z(); // ground level

    // Temporary: If standing on ground
    //if (tryJump && this->player.position.z() <= baseZ) {
    //    if (this->player.inputCmdActiveCount_speed)
    //        this->player.velocity.z() = 4 * CSGO_CVAR_SV_JUMP_IMPULSE;
    //    else
    //        this->player.velocity.z() = CSGO_CVAR_SV_JUMP_IMPULSE;
    //}

    // CONFIRMED: CSGO first applies gravity to velocity, then applies velocity to position!
    //            Jump impulse velocity must not be set between these 2 steps!
    // 1. Apply gravity to vertical velocity
    //this->player.velocity.z() -= timeDelta * CSGO_CVAR_SV_GRAVITY;
    // 2. Apply velocity to position
    this->player.position += timeDelta * this->player.velocity;

    // Temporary: enforce test ground level
    //if (this->player.position.z() < baseZ) {
    //    this->player.position.z() = baseZ;
    //    this->player.velocity.z() = 0.0f;
    //}

    //ACQUIRE_COUT(Debug{} << wState.player.position.z();)

    //size_t x = 0;
    //auto target_sleep_end = Clock::now() + std::chrono::microseconds{ 500 };
    //while (Clock::now() < target_sleep_end) {
    //    std::this_thread::yield();
    //    //std::this_thread::sleep_for(std::chrono::microseconds{ 1 });
    //    //++x;
    //}
    //auto sleep_end = Clock::now();
    //auto sleep_deviation_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(sleep_end - target_sleep_end).count();
    //static long long max_yielddev_ns = -1000000;
    //if (sleep_deviation_ns > max_yielddev_ns)
    //    max_yielddev_ns = sleep_deviation_ns;
    //Debug{} << "maxyielddev =" << max_yielddev_ns / 1e6f << "ms";
    //Debug{} << "x =" << x;
}
