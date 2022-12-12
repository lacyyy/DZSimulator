#include "Handler.h"

#include <regex>

#include <Corrade/Utility/Debug.h>
#include <Magnum/Magnum.h>

using namespace Magnum;
using namespace std::chrono_literals;

using namespace csgo_integration;

#define UPDATE_RATE 64
#define UPDATE_INTERVAL_US 1'000'000 / UPDATE_RATE

#define MSG_HOST_NEW_GAME "---- Host_NewGame ----"
#define MSG_CHEATS_OFF "Can't use cheat command getpos in multiplayer, unless "\
    "the server has sv_cheats set to 1."
#define MSG_NOT_HOST "Can't change replicated ConVar sv_cheats from console of"\
    " client, only server operator can change its value"

#define RESPONSE_TIMEOUT_MS 500

// Magic number that marks the end of delayed game data, specifically marks the
// end of bumpmine position data
#define DATA_SENTINEL 980886351


// Can't use cheat command getpos in multiplayer, unless the server has sv_cheats set to 1.
// Can't change replicated ConVar sv_cheats from console of client, only server operator can change its value

// 'echo' always works
// 'status' always works (map name info)
// When connected to a remote server, 'script' and 'ent_fire' never work.
// When connected to a remote server with sv_cheats 1, 'getpos' works.
// When hosting a server:
//    with sv_cheats 0: script works, ent_fire and getpos don't work
//    with sv_cheats 1: script ent_fire and getpos work
// Only usable locally and with sv_cheats 1

Handler::Handler(RemoteConsole& con)
    : _con (con) {
}

void Handler::Update()
{
    auto time_now = std::chrono::steady_clock::now();

    if (!_con.IsConnected()) {
        _commands_enabled = true;
        _waiting_for_cheats_enable_response = false;
        return;
    }

    if (_commands_enabled) {
        auto time_since_last_cmd = time_now - _last_command_time;
        
        if (time_since_last_cmd > std::chrono::microseconds(UPDATE_INTERVAL_US)) {
            _last_command_time = time_now;
            std::string cmds = "";
            cmds += "echo;";
            cmds += "getpos;";
            cmds += "getpos_exact;";
            cmds += "ent_fire !self RunScriptCode \"print(9876.54);printl(self.GetVelocity())\";";
            cmds += "ent_fire bumpmine_projectile RunScriptCode \"print(1234.67); printl(self.GetOrigin())\";";
            cmds += "ent_fire !self RunScriptCode \"printl(" + std::to_string(DATA_SENTINEL) + ")\"";
            _con.WriteLine(cmds);
        }
    }

    auto received_lines = _con.ReadLines();
    for (const std::string& line : received_lines) {
        if (line.compare(MSG_HOST_NEW_GAME) == 0) {
            _commands_enabled = true;
            _waiting_for_cheats_enable_response = false;
            break;
        }

        if (_commands_enabled && !_waiting_for_cheats_enable_response) {
            if (line.compare(MSG_CHEATS_OFF) == 0) {
                _con.WriteLine("sv_cheats 1");
                _commands_enabled = false;
                _waiting_for_cheats_enable_response = true;
                _enable_cheats_attempt_time = time_now;
                break;
            }
        }

        if (_waiting_for_cheats_enable_response) {
            if (line.compare(MSG_NOT_HOST) == 0) {
                _waiting_for_cheats_enable_response = false;
                break;
            }
        }

        ParseGameData(line);
    }

    if (_waiting_for_cheats_enable_response) {
        auto time_since_attempt = time_now - _enable_cheats_attempt_time;
        if (time_since_attempt > std::chrono::milliseconds(RESPONSE_TIMEOUT_MS)) {
            // Assume that cheats werer´successfully enabled!
            _commands_enabled = true;
            _waiting_for_cheats_enable_response = false;
        }
    }

}

void Handler::ParseGameData(const std::string& line)
{
    //setpos 925.000000 -1100.000000 -199.906189;setang 0.000000 0.000000 0.000000
    //setpos_exact 925.000000 - 1100.000000 - 199.906189; setang_exact 0.000000 0.000000 0.000000

    bool is_getpos = line.starts_with("setpos ");
    bool is_getpos_exact = line.starts_with("setpos_exact ");
    bool is_player_vel_cmd = line.starts_with("9876.54");
    bool is_bump_mine = line.starts_with("1234.67");
    bool is_bump_mine_data_finished = line.starts_with(std::to_string(DATA_SENTINEL));

    if (is_bump_mine_data_finished) {
        _bump_mines = std::move(_new_bump_mine_buf);
        _new_bump_mine_buf = {};
    }

    // 1234.67(vector : (2594.770752, 1953.623657, 1004.210754))
    //1234.67(vector : (-1429.364136, 2984.269775, 528.048950))

    if (is_getpos || is_getpos_exact || is_player_vel_cmd || is_bump_mine) {
        // Match separate float values with ONLY the following formats:
        // -0.14
        // 1.0
        // -782.1314
        std::regex regex(R"(-?[0-9]+\.[0-9]+)");
        auto vals_begin = std::sregex_iterator(line.begin(), line.end(), regex);
        auto vals_end = std::sregex_iterator(); // end-of-sequence iterator

        std::vector<float> vals;
        for (std::sregex_iterator iter = vals_begin; iter != vals_end; ++iter) {
            // std::stof can't fail with used regex
            vals.push_back(std::stof(iter->str()));
        }

        if (is_getpos) {
            if (vals.size() >= 6) {
                _player_pos_eye = { vals[0], vals[1], vals[2] };
                _player_angles = { vals[3], vals[4], vals[5] };
            }
        }
        else if (is_getpos_exact) {
            if (vals.size() >= 6) {
                _player_pos_feet = { vals[0], vals[1], vals[2] };
            }
        }
        else if (is_player_vel_cmd) {
            if (vals.size() >= 4) {
                _player_vel = { vals[1], vals[2], vals[3] };
            }
        }
        else if (is_bump_mine) {
            if (vals.size() >= 4) {
                _new_bump_mine_buf.push_back({ vals[1], vals[2], vals[3] });
            }
        }
    }
}

Magnum::Vector3 Handler::GetPlayerPosition()
{
    return _player_pos_feet;
}

Magnum::Vector3 Handler::GetPlayerEyePosition()
{
    return _player_pos_eye;
}

Magnum::Vector3 Handler::GetPlayerAngles()
{
    return _player_angles;
}

Magnum::Vector3 Handler::GetPlayerVelocity()
{
    return _player_vel;
}

std::vector<Magnum::Vector3> Handler::GetBumpMines()
{
    return _bump_mines;
}
