#ifndef SIM_SERVER_H_
#define SIM_SERVER_H_

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <queue>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "sim/PlayerInputState.h"
#include "sim/Sim.h"
#include "sim/WorldState.h"

namespace sim {

// Just a listen server running in it's own thread, serving one client
class Server {
public:
    // All public method functions may only be called by the user/client thread

    Server(float tick_rate);
    ~Server();
    // Server is not copyable or movable to avoid handling of the std::thread member
    Server(const Server& other) = delete;
    Server& operator=(const Server& other) = delete;


    void Start();
    void Stop();

    void SendNewPlayerInput(const PlayerInputState& pis);
    void OverrideWorldState(const WorldState& newWorldState);

    void ChangeTickRate(double newTickRate);
    void ChangeSimulationTimeScale(double newSimTimeScale);

    double GetTickRate();
    double GetSimulationTimeScale();

    std::queue<WorldState> DequeueLatestWorldStates();

    class PerformanceStats {
    public:
        float tick_duration_ms; // milliseconds
        float tick_duration_deviation_ms; // milliseconds
        float tick_start_deviation_rel; // percent
        float max_sleep_deviation_ms; // milliseconds
    };

    PerformanceStats GetPerformanceStats();

private:
    // Account for usual minimum OS sleep inaccuracy when sleeping in the server thread
    // On Windows, usual sleep deviation is 2ms
    const Clock::duration USUAL_OS_SLEEP_DEVIATION = std::chrono::microseconds{ 2000 };

    // All 'sv_*' method functions may only be called by the internal server thread
    void sv_dequeue_player_input();

    // @return The time when the next simulation tick is or was supposed to start
    // @param simulationStart: The time when the next tick to be processed is or was supposed to start
    Clock::time_point sv_server_loop(const Clock::time_point& simulationStart=Clock::now());

    // -------------------------------------------------------------------------------------------
    // ---- Member variables used ONLY by the user/client thread ('m_cl' prefix):

    std::thread _cl_thread; // Internal game server thread, only accessed by client thread


    // -------------------------------------------------------------------------------------------
    // ---- Member variables used by BOTH the client and server thread ('m_sh' prefix):
    std::atomic<bool> _sh_stopped;

    class Config { // Settings that require a server loop restart
    public:
        double tick_rate;
        double simulation_time_scale; // Behaves like csgo cvar 'host_timescale'
    };
    std::atomic<Config> _sh_config;
    
    std::atomic<PerformanceStats> _sh_performanceStats;
    
    // std::queue is chosen for fast insertions and deletions
    std::mutex _sh_playerInputQueue_mutex;
    std::queue<PlayerInputState> _sh_playerInputQueue;
    
    std::mutex _sh_outputWorldStateQueue_mutex;
    std::queue<WorldState> _sh_outputWorldStateQueue;

    // World state overrides by the client (e.g., player teleportation)
    // This mutex protects both _sh_worldStateOverrideRequired and _sh_overrideWorldState
    std::mutex _sh_worldStateOverride_mutex;
    bool _sh_worldStateOverrideRequired; // If server has to override its current state in the next tick
    WorldState _sh_overrideWorldState; // The world state to override with

    // -------------------------------------------------------------------------------------------
    // ---- Member variables used ONLY by the internal server thread ('m_sv' prefix):

    // World state after last processed tick
    WorldState _sv_currentWorldState;

    // Player input dequeued from _sh_playerInputQueue gets queued into _sv_dequeuedPlayerInput
    // to shorten acquisition time of the _sh_playerInputQueue mutex
    std::queue<PlayerInputState> _sv_dequeuedPlayerInput;


    const size_t SERVER_FRAME_HISTORY_MAX_LEN = 128; // 2 seconds with a tickrate of 64
    size_t _sv_frameHistoryWritePos = 0; // Position of oldest history entry that is overwritten next
    size_t _sv_frameHistorySize = 0; // Number of frames the history is currently holding
    std::vector<Clock::duration> _sv_hist_tickDuration;
    std::vector<Clock::duration> _sv_hist_sleepDeviation;

};

} // namespace sim

#endif // SIM_SERVER_H_
