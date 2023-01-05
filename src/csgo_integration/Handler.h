#ifndef CSGO_INTEGRATION_HANDLER_H_
#define CSGO_INTEGRATION_HANDLER_H_

#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include <Corrade/Utility/Resource.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "csgo_integration/RemoteConsole.h"
#include "gui/GuiState.h"

namespace csgo_integration {

    // Communicates with the ingame console of a running CSGO process.
    // Sends console commands and receives their output in order to attain game
    // data such as player position, speed, etc...
    class Handler {
    public:

        // Positional data of a server tick in CSGO, gets received at fixed rate
        struct CsgoServerTickData {
            size_t tick_id = SIZE_MAX;

            bool is_player_crouched = false;
            Magnum::Vector3 player_pos_feet; // from CSGO VScript's GetOrigin()
            Magnum::Vector3 player_pos_eye;  // from CSGO VScript's EyePosition()
            Magnum::Vector3 player_vel;      // from CSGO VScript's GetVelocity()

            struct BumpMineData {
                Magnum::Vector3 pos;
                Magnum::Vector3 angles; // pitch, yaw, roll
            };
            // Keys are unique IDs of bump mines, values are their data
            // Caution: IDs of no longer existing bump mines may get reused for
            //          new bump mines!
            std::map<int, BumpMineData> bump_mines;
        };

        // Client-side positional data in CSGO, receive rate is variable 
        struct CsgoClientsideData {
            // Both values from client-side CSGO command 'getpos'
            
            Magnum::Vector3 player_angles;

            // This eye position is subject to client-side prediction.
            // It can differ substantially from server-side eye position!
            Magnum::Vector3 player_pos_eye;
        };

        Handler(
            Corrade::Utility::Resource& res,
            RemoteConsole& con,
            gui::GuiState& gui_state);

        // Reads and writes from/to CSGO's console if it's connected.
        // Attains new csgo client and server data.
        void Update();

        std::deque<CsgoServerTickData> DequeNewCsgoServerTicksData();
        std::deque<CsgoClientsideData> DequeNewCsgoClientsideData();

    private:
        void ParseConsoleOutput(const std::string& line);

        RemoteConsole& _con;
        gui::GuiState& _gui_state;

        // If console was connected during the last call to Update()
        bool _prev_is_console_connected = false;

        enum CsgoUiState {
            UNKNOWN,
            LOADINGSCREEN,    // CSGO_GAME_UI_STATE_LOADINGSCREEN
            NOT_LOADINGSCREEN // any other CSGO_GAME_UI_STATE_*
        } _csgo_ui_state = UNKNOWN;
        bool _is_csgo_ui_state_unknown_and_unchecked = true;

        // Ordered list of CSGO console commands that set up and start a "data
        // relay" running inside CSGO that continously prints game data to the
        // console.
        std::vector<std::string> _data_relay_setup_cmds;

        // Queue of newly received CSGO game data
        std::deque<CsgoServerTickData> _new_server_ticks_data_q;
        std::deque<CsgoClientsideData> _new_client_side_data_q;

        // Data of next server tick data still being received
        CsgoServerTickData _incomplete_next_server_tick_data;


        using Clock = std::chrono::steady_clock;

        // The last time we tested if user is hosting a local server
        Clock::time_point _last_host_server_check_time;


        // The last time we tried to set up a data relay that runs inside CSGO
        // and continously prints game data to the console.
        Clock::time_point _last_data_relay_setup_time;

        // The last time we received game data from a data relay
        Clock::time_point _last_data_relay_receive_time;

        // The last time we sent a keep-alive signal to the data relay
        Clock::time_point _last_data_relay_keep_alive_signal_time;

        // The last time we sent a stop signal to the data relay
        Clock::time_point _last_data_relay_stop_signal_time;

    };

}

#endif // CSGO_INTEGRATION_HANDLER_H_
