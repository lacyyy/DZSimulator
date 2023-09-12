#include "csgo_integration/Handler.h"

#include <regex>
#include <stdexcept>
#include <string>

#include <Corrade/Utility/Debug.h>
#include <Magnum/Magnum.h>

#include "csgo_parsing/utils.h"

using namespace Magnum;
using namespace std::chrono_literals;

using namespace csgo_integration;

// Note: CSGO's console command 'script' always works when it is hosting a local
//       server, even when sv_cheats is 0.

// CAUTION: If the CSGO client is connected to a matchmaking server and issues
//          more than ca. 40 commands per second, they get kicked with the
//          message "Issued too many commands to server."!
//          The problem gets real when we regularly send e.g. 'script' commands
//          to the CSGO client while they're connecting to a matchmaking server
//          and loading a map. Once the map has loaded, all our commands sent
//          during loading are then ALL EXECUTED AT ONCE, possibly meaning a
//          too high command rate resulting in an immediate kick!
//          -> Don't send console commands during loading screens

// Message and command to test if user is hosting a local server (listen server)
#define HOST_SERVER_CHECK_RESPONSE "dzs-hosting-server-check"
#define HOST_SERVER_CHECK_CMD "script printl(\"\\n" HOST_SERVER_CHECK_RESPONSE "\")"

// The data relay is assumed stopped after not sending data for this long
const auto DATA_RELAY_TIMEOUT = std::chrono::milliseconds(100);

// Console command to tell the data relay to keep running for a specified duration
static std::string DATA_RELAY_KEEP_ALIVE_CMD(float duration_secs) {
    return "script DZSDR1_KEEP_ALIVE_UNTIL<-Time()+" + std::to_string(duration_secs);
}
#define DATA_RELAY_STOP_CMD "script DZSDR1_KEEP_ALIVE_UNTIL<-0.0"


Handler::Handler(Corrade::Utility::Resource& res, RemoteConsole& con,
    gui::GuiState& gui_state)
    : _con(con)
    , _gui_state(gui_state)
{
    // Get CSGO console commands that create a "data relay" that runs inside
    // CSGO and continously prints game data to the console.
    
    // Caution! Due to the nature of editing text files on different operating
    // systems, lines in this file are terminated by either "\r\n" or just "\n".
    std::string data_relay_cfg = res.getString("csgo_integration/data_relay.cfg");

    std::vector<std::string> lines =
        csgo_parsing::utils::SplitString(data_relay_cfg, '\n');

    for (std::string line : lines) {
        if (line.empty())
            continue;

        // CSGO console doesn't accept leading whitespace
        size_t cmd_start_pos = line.find_first_not_of(" \r\n\t");
        if (cmd_start_pos == std::string::npos)
            continue;

        size_t cmd_len = line.length() - cmd_start_pos;

        // Exclude possible leftover of "\r\n" line ending
        if (line[line.length() - 1] == '\r')
            cmd_len--;

        std::string trimmed_line = line.substr(cmd_start_pos, cmd_len);

        if (trimmed_line.starts_with("//"))
            continue; // Ignore comments

        _data_relay_setup_cmds.push_back(std::move(trimmed_line));
    }
}

void Handler::Update()
{
    auto time_now = Clock::now();

    // Parse received CSGO console output
    size_t pre_parse_server_q_len = _new_server_ticks_data_q.size();
    bool received_host_server_check_response = false;
    auto received_lines = _con.ReadLines();
    for (const std::string& line : received_lines) {
        if (line.compare(HOST_SERVER_CHECK_RESPONSE) == 0) {
            received_host_server_check_response = true;
            continue;
        }

        // e.g. "ChangeGameUIState: CSGO_GAME_UI_STATE_MAINMENU -> CSGO_GAME_UI_STATE_LOADINGSCREEN\n"
        if (line.starts_with("ChangeGameUIState:")) { // CSGO UI state transition
            std::vector<std::string> tokens =
                csgo_parsing::utils::SplitString(line, ' ');
            if (tokens.size() == 4 && tokens[2].compare("->") == 0) {
                std::string& new_ui_state = tokens[3];
                if (new_ui_state.compare("CSGO_GAME_UI_STATE_LOADINGSCREEN") == 0) {
                    _csgo_ui_state = LOADINGSCREEN;
                }
                else {
                    _csgo_ui_state = NOT_LOADINGSCREEN;
                    // Other states we don't distinguish between are:
                    // CSGO_GAME_UI_STATE_INGAME
                    // CSGO_GAME_UI_STATE_INTROMOVIE
                    // CSGO_GAME_UI_STATE_INVALID
                    // CSGO_GAME_UI_STATE_MAINMENU
                    // CSGO_GAME_UI_STATE_PAUSEMENU
                }
                _is_csgo_ui_state_unknown_and_unchecked = false;
            }
            continue;
        }

        ParseConsoleOutput(line);
    }
    size_t post_parse_server_q_len = _new_server_ticks_data_q.size();

    // Remember when we last received something from a data relay running inside CSGO
    if (post_parse_server_q_len > pre_parse_server_q_len)
        _last_data_relay_receive_time = time_now;

    auto time_without_relay_data = time_now - _last_data_relay_receive_time;
    bool is_data_relay_running = time_without_relay_data < DATA_RELAY_TIMEOUT;

    // Need to be connected to send CSGO console commands
    if (!_con.IsConnected()) {
        _csgo_ui_state = UNKNOWN;
        _is_csgo_ui_state_unknown_and_unchecked = true;
        _prev_is_console_connected = false;
        return;
    }

    // If console was disconnected last time, meaning we have just connected
    if (!_prev_is_console_connected) {
        _csgo_ui_state = UNKNOWN;
        _is_csgo_ui_state_unknown_and_unchecked = true;
        _last_host_server_check_time = {}; // Ensure next check is immediate
        _last_data_relay_setup_time = {}; // Ensure setup can be immediate
    }

    // Receiving data implies we're not in the loading screen
    if (is_data_relay_running) {
        _csgo_ui_state = NOT_LOADINGSCREEN;
        _is_csgo_ui_state_unknown_and_unchecked = false;
    }

    // Decide if we currently want game data from CSGO
    bool want_csgo_data = _gui_state.vis.IN_geo_vis_mode ==
        gui::GuiState::VisualizationSettings::GLID_OF_CSGO_SESSION;

    // Determine console commands to send
    if (want_csgo_data) {
        if (!is_data_relay_running) {
            // Determine if "host server" check is forbidden
            bool host_server_check_forbidden = true;
            if (_csgo_ui_state == LOADINGSCREEN) {
                // Checking during loading screens makes all checks get executed
                // once loading is finished. The high rate of commands can get
                // the user kicked from servers, AVOID THAT!
                host_server_check_forbidden = true;
            }
            else if (_csgo_ui_state == NOT_LOADINGSCREEN) {
                host_server_check_forbidden = false;
            }
            else if(_csgo_ui_state == UNKNOWN) {
                // When unknown, only a single check is allowed!
                // (Because if it's LOADINGSCREEN, we must not check further)
                if (_is_csgo_ui_state_unknown_and_unchecked) {
                    host_server_check_forbidden = false;
                    _is_csgo_ui_state_unknown_and_unchecked = false;
                }
                else {
                    host_server_check_forbidden = true;
                }
            }

            if (!host_server_check_forbidden) {
                // Periodically send command to test if user has permission to
                // issue 'script' console commands. (i.e. test if they're
                // hosting a local server)
                // CAUTION: Sending the check cmd too frequently can get the
                //          client kicked from an online server!
                auto time_since_last_check = time_now - _last_host_server_check_time;
                if (time_since_last_check > std::chrono::milliseconds(1500)) {
                    _con.WriteLine(HOST_SERVER_CHECK_CMD);
                    Debug{} << "[CsgoHandler] Host server check";
                    _last_host_server_check_time = time_now;
                }
            }

            // If CSGO confirmed it's hosting a local server
            if (received_host_server_check_response) {
                // Run commands that set up a "data relay" inside CSGO. It will
                // continously print game data to the console for us to read.
                auto time_since_last_setup = time_now - _last_data_relay_setup_time;
                if (time_since_last_setup > std::chrono::milliseconds(3000)) {
                    _con.WriteLines(_data_relay_setup_cmds);
                    Debug{} << "[CsgoHandler] Data relay setup";
                    _last_data_relay_setup_time = time_now;
                }
            }
        }
        else { // is_data_relay_running
            // Periodically send keep-alive signal to data relay.
            // The data relay would stop itself without it.
            const float SEND_INTERVAL_SECS = 1.0f;
            const auto SEND_INTERVAL =
                std::chrono::milliseconds((long long)(1000 * SEND_INTERVAL_SECS));
            auto time_since_last_signal =
                time_now - _last_data_relay_keep_alive_signal_time;
            if (time_since_last_signal > SEND_INTERVAL) {
                // Tell data relay to stay alive for slightly longer than our
                // send interval.
                //  - We don't want the data relay to spam the console after use
                //  - We don't want the data relay to stop when we send the
                //    next keep-alive signal slightly too late.
                _con.WriteLine(DATA_RELAY_KEEP_ALIVE_CMD(SEND_INTERVAL_SECS + 0.3f));
                _last_data_relay_keep_alive_signal_time = time_now;
            }
        }
    }
    else { // !want_csgo_data
        if (is_data_relay_running) {
            // Periodically send stop signal to data relay until we receive no
            // more game data from it.
            auto time_since_last_signal =
                time_now - _last_data_relay_stop_signal_time;
            if (time_since_last_signal > std::chrono::milliseconds(300)) {
                _con.WriteLine(DATA_RELAY_STOP_CMD);
                Debug{} << "[CsgoHandler] Stopping data relay";
                _last_data_relay_stop_signal_time = time_now;
            }
        }
    }

    _prev_is_console_connected = true; // Remember for next call to Update()
}

void Handler::ParseConsoleOutput(const std::string& line)
{
    // Server-side data
    bool is_server_data_start_marker = line.starts_with("dzs-dr-v1-tick-start ");
    bool is_server_player_data       = line.starts_with("player ");
    bool is_server_bump_mine_data    = line.starts_with("bm ");
    bool is_server_data_end_marker   = line.starts_with("dzs-dr-v1-tick-end");

    // Client-side data
    bool is_client_player_data       = line.starts_with("setpos ");
    
    if (is_server_data_start_marker || is_server_player_data || is_server_bump_mine_data) {
        // Get values separated by spaces
        std::vector<std::string> tokens =
            csgo_parsing::utils::SplitString(line, ' ');

        if (is_server_data_start_marker) { // e.g. "dzs-dr-v1-tick-start 85\n"
            if (tokens.size() < 2) return;
            try {
                int tick_id = std::stoi(tokens[1]);
                _incomplete_next_server_tick_data.tick_id = tick_id;
            }
            catch (const std::invalid_argument&) {}
            catch (const std::out_of_range&) {}
            return;
        }
        if (is_server_player_data) { // e.g. "player -4634.38 271.376 1571.43 1635.43 false -322.528 -979.985 -442.433\n"
            if (tokens.size() < 9) return;
            bool crouched = tokens[5].compare("true") == 0;
            try {
                Vector3 feet_pos = {
                    std::stof(tokens[1]), // x
                    std::stof(tokens[2]), // y
                    std::stof(tokens[3])  // z
                };
                Vector3 eye_pos = {
                    feet_pos.x(), // x, same as feet_pos.x
                    feet_pos.y(), // y, same as feet_pos.y
                    std::stof(tokens[4])  // z, different from feet_pos.z
                };
                Vector3 vel = {
                    std::stof(tokens[6]), // x
                    std::stof(tokens[7]), // y
                    std::stof(tokens[8])  // z
                };

                // Only accept and set data if nothing went wrong during parsing
                _incomplete_next_server_tick_data.player_pos_feet = feet_pos;
                _incomplete_next_server_tick_data.player_pos_eye = eye_pos;
                _incomplete_next_server_tick_data.player_vel = vel;
                _incomplete_next_server_tick_data.is_player_crouched = crouched;
            }
            catch (const std::invalid_argument&) {}
            catch (const std::out_of_range&) {}
            return;
        }
        if (is_server_bump_mine_data) { // e.g. "bm 920 -1153.88 -1005.98 997.602 -3.27541 -2.71377 5.52694\n"
            if (tokens.size() < 8) return;
            try {
                int bm_id = std::stoi(tokens[1]);
                Handler::CsgoServerTickData::BumpMineData bm_data;
                bm_data.pos = {
                    std::stof(tokens[2]), // x
                    std::stof(tokens[3]), // y
                    std::stof(tokens[4])  // z
                };
                bm_data.angles = {
                    std::stof(tokens[5]), // pitch
                    std::stof(tokens[6]), // yaw
                    std::stof(tokens[7])  // roll
                };

                // Only accept and set data if nothing went wrong during parsing
                _incomplete_next_server_tick_data.bump_mines[bm_id] = std::move(bm_data);
            }
            catch (const std::invalid_argument&) {}
            catch (const std::out_of_range&) {}
            return;
        }
    }
    
    if (is_server_data_end_marker) { // e.g. "dzs-dr-v1-tick-end\n"
        // Data of next server tick is complete, put it on the queue
        _new_server_ticks_data_q.push_back(std::move(_incomplete_next_server_tick_data));
        _incomplete_next_server_tick_data = CsgoServerTickData(); // construct for reuse
        return;
    }

    // Client-side data that is parsed and handled separately from server data.
    // Parse response of the 'getpos' command.
    if (is_client_player_data) { // e.g. "setpos -1338.681030 -1089.463989 1070.809204;setang 21.053974 -10.723428 0.000000\n"
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

        if (vals.size() < 6)
            return;

        CsgoClientsideData new_client_data;
        new_client_data.player_pos_eye = { vals[0], vals[1], vals[2] };
        new_client_data.player_angles  = { vals[3], vals[4], vals[5] };

        // Next client-side data is complete, put it on the queue
        _new_client_side_data_q.push_back(std::move(new_client_data));
        return;
    }
}

std::deque<Handler::CsgoServerTickData> Handler::DequeNewCsgoServerTicksData()
{
    std::deque<Handler::CsgoServerTickData> ret;
    ret.swap(_new_server_ticks_data_q);
    return ret;
}

std::deque<Handler::CsgoClientsideData> Handler::DequeNewCsgoClientsideData()
{
    std::deque<Handler::CsgoClientsideData> ret;
    ret.swap(_new_client_side_data_q);
    return ret;
}
