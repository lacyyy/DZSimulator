#ifndef CSGO_INTEGRATION_HANDLER_H_
#define CSGO_INTEGRATION_HANDLER_H_

#include <chrono>
#include <string>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "csgo_integration/RemoteConsole.h"

namespace csgo_integration {

    class Handler {
    public:
        Handler(RemoteConsole& con);

        void Update();
        void ParseGameData(const std::string& line);

        Magnum::Vector3 GetPlayerPosition();
        Magnum::Vector3 GetPlayerEyePosition();
        Magnum::Vector3 GetPlayerAngles();
        Magnum::Vector3 GetPlayerVelocity();
        std::vector<Magnum::Vector3> GetBumpMines();


    private:
        RemoteConsole& _con;

        std::chrono::steady_clock::time_point _last_command_time;

        bool _commands_enabled = false;

        bool _waiting_for_cheats_enable_response = false;
        std::chrono::steady_clock::time_point _enable_cheats_attempt_time;

        // game data
        Magnum::Vector3 _player_pos_feet;
        Magnum::Vector3 _player_pos_eye;
        Magnum::Vector3 _player_angles;
        Magnum::Vector3 _player_vel;
        std::vector<Magnum::Vector3> _bump_mines;


        std::vector<Magnum::Vector3> _new_bump_mine_buf;

    };

}

#endif // CSGO_INTEGRATION_HANDLER_H_

