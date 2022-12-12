#include "Server.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <windows.h> // For setting thread priority

#include <Magnum/Magnum.h>
#include <Corrade/Utility/Debug.h>

#include "GlobalVars.h"

using namespace Magnum;
using namespace std::chrono_literals;

using namespace sim;

// Helper functions
inline long long nanoseconds_count(const Clock::duration& duration) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}
inline long long microseconds_count(const Clock::duration& duration) {
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

Clock::duration calc_standard_deviation(Clock::duration* arr, size_t len)
{
    if (len == 0) return 0s;

    Clock::duration mean = 0s;
    for (size_t i = 0; i < len; ++i)
        mean += arr[i];
    mean /= len;

    double squaredDeviationSum = 0.0;
    for (size_t i = 0; i < len; ++i) {
        double deviation = microseconds_count( arr[i] - mean );
        squaredDeviationSum += deviation * deviation;
    }

    return std::chrono::microseconds{ (long long)std::sqrt(squaredDeviationSum / len) };
}



Server::Server(float tick_rate) :
    _sh_stopped(true), // Important initial state
    _sh_worldStateOverrideRequired(false),
    _sv_hist_tickDuration(SERVER_FRAME_HISTORY_MAX_LEN, 0s),
    _sv_hist_sleepDeviation(SERVER_FRAME_HISTORY_MAX_LEN, 0s)
{
    Config cfg;
    cfg.tick_rate = tick_rate;
    cfg.simulation_time_scale = 1.0;
    _sh_config = cfg;
}

Server::~Server()
{
    if (!_sh_stopped)
        Stop();
}

void Server::Start()
{
    if (!_sh_stopped) {
        throw std::invalid_argument("Server::Start() may not be called when the game server is running!");
    }

    ACQUIRE_COUT( Debug{} << "[Server] Starting thread..."; )

    _sh_stopped = false;
    _cl_thread = std::thread([this]() {
        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL)) {
            Error{} << "[ERR] [Thread-Server] SetThreadPriority() failed!";
        }

        Clock::time_point simulationStart = Clock::now();
        do {
            // sv_server_loop() returns if server was stopped or if configuration changed
            Clock::time_point nextTickStart = this->sv_server_loop(simulationStart);
            simulationStart = nextTickStart; // Tell next server loop to start where the previous loop ended
        } while (!_sh_stopped); // If server wasn't stopped, start server loop again
    });
}

void Server::Stop()
{
    if (_sh_stopped) {
        throw std::invalid_argument("Server::Stop() may not be called when the game server is not running!");
    }

    ACQUIRE_COUT(Debug{} << "[Server] Stopping thread...";)

    _sh_stopped = true;
    _cl_thread.join();

    ACQUIRE_COUT(Debug{} << "[Server] Stopped thread!";)
}

void Server::SendNewPlayerInput(const PlayerInputState& pis)
{
    std::lock_guard<std::mutex> guard(_sh_playerInputQueue_mutex);
    _sh_playerInputQueue.push(pis); // Append to end of queue
}

void Server::OverrideWorldState(const WorldState& newWorldState)
{
    std::lock_guard<std::mutex> guard(_sh_worldStateOverride_mutex);
    _sh_worldStateOverrideRequired = true;
    _sh_overrideWorldState = newWorldState;
}

void Server::ChangeTickRate(double newTickRate)
{
    Config newCfg = _sh_config;  // Atomically read
    newCfg.tick_rate = newTickRate;
    _sh_config = newCfg; // Atomically write
}

void Server::ChangeSimulationTimeScale(double newSimTimeScale)
{
    Config newCfg = _sh_config; // Atomically read
    newCfg.simulation_time_scale = newSimTimeScale;
    _sh_config = newCfg; // Atomically write
}

double Server::GetTickRate()            { return _sh_config.load().tick_rate; }
double Server::GetSimulationTimeScale() { return _sh_config.load().simulation_time_scale; }

std::queue<WorldState> Server::DequeueLatestWorldStates()
{
    std::queue<WorldState> out;
    {
        std::lock_guard<std::mutex> guard(_sh_outputWorldStateQueue_mutex);
        out.swap(_sh_outputWorldStateQueue);
    } // Release mutex
    return out;
}

Server::PerformanceStats Server::GetPerformanceStats()
{
    PerformanceStats stat = _sh_performanceStats; // Atomic access
    return stat;
}

void Server::sv_dequeue_player_input()
{
    std::queue<PlayerInputState> freshlyDequeued;
    {
        std::lock_guard<std::mutex> guard(_sh_playerInputQueue_mutex);
        freshlyDequeued.swap(_sh_playerInputQueue);
    } // Release mutex

    while (!freshlyDequeued.empty()) { // Append new elements to the server's own queue
        _sv_dequeuedPlayerInput.push(freshlyDequeued.front());
        freshlyDequeued.pop();
    }
}

Clock::time_point Server::sv_server_loop(const Clock::time_point& simulationStart)
{
    Config cfg = _sh_config; // Atomic access
    Clock::duration targetFrameDuration = std::chrono::microseconds{ (long long)(1000'000 / (cfg.tick_rate * cfg.simulation_time_scale)) };
    double simulationStepSize_sec = 1.0 / cfg.tick_rate; // acccurate simulation delta (in seconds)

    size_t tickCount = 0;

    Clock::duration prev_maxSleepDeviation = 0s; // Biggest sleep deviation of the last couple server frames

    while (!_sh_stopped) {
        // Start next tick/server frame computation
        auto tick_start = Clock::now();

        // When this tick was supposed to start, exactly
        auto scheduled_tick_start = simulationStart + tickCount * targetFrameDuration;

        // Check if configuration changed
        Config newCfg = _sh_config;
        if (newCfg.tick_rate != cfg.tick_rate || newCfg.simulation_time_scale != cfg.simulation_time_scale)
            return scheduled_tick_start; // Tell next server loop where to start again with the new config

        // Quickly dequeue input from multithreaded queue into server's own queue
        sv_dequeue_player_input();
        // Only use player input that was created before the current tick/server frame was supposed to start
        std::vector<PlayerInputState> playerInput;
        while (!_sv_dequeuedPlayerInput.empty()) {
            if (_sv_dequeuedPlayerInput.front().time > scheduled_tick_start)
                break;
            playerInput.push_back(_sv_dequeuedPlayerInput.front());
            _sv_dequeuedPlayerInput.pop();
        }

        // Check if client wants to override the server world state
        {
            std::lock_guard<std::mutex> guard(_sh_worldStateOverride_mutex);
            if (_sh_worldStateOverrideRequired) {
                _sv_currentWorldState = std::move(_sh_overrideWorldState);
                _sh_worldStateOverrideRequired = false;
            }
        } // Release mutex of _sh_worldStateOverrideRequired and _sh_overrideWorldState

        // Simulate the next tick with user input
        _sv_currentWorldState.DoTimeStep(simulationStepSize_sec, playerInput);
        // Adjust current tick's world state time to account for simulation time scale
        _sv_currentWorldState.time = scheduled_tick_start;

        // Make new world state available to the client
        {
            std::lock_guard<std::mutex> guard(_sh_outputWorldStateQueue_mutex);
            _sh_outputWorldStateQueue.push(_sv_currentWorldState);
        } // Release _sh_outputWorldStateQueue mutex
        
        auto tick_duration = Clock::now() - tick_start;

        // TODO if tick_duration_ns > m_targetTickLength_ns -> give user warning!

        long long frameStartDeviation_ns = nanoseconds_count( (tick_start - simulationStart) - (tickCount * targetFrameDuration) );
        float frameStartDeviation_rel = (float)frameStartDeviation_ns / nanoseconds_count(targetFrameDuration);

        auto target_nextFrameStart = simulationStart + ((tickCount + 1) * targetFrameDuration);

        // Determine sleep duration for imprecise sleep method
        auto target_sleep_duration = target_nextFrameStart - Clock::now();
        // Sleep less by the amount of past sleep deviation
        if (prev_maxSleepDeviation < USUAL_OS_SLEEP_DEVIATION) target_sleep_duration -= USUAL_OS_SLEEP_DEVIATION;
        else                                                   target_sleep_duration -= prev_maxSleepDeviation;

        // Sleep a fraction of the target frame duration less to stay under the target more often
        //target_sleep_duration -= targetFrameDuration / 10;

        Clock::duration sleep_deviation;
        if (target_sleep_duration > 0s) {
            auto sleep_start = Clock::now();
            std::this_thread::sleep_for(target_sleep_duration); // fairly imprecise
            auto sleep_duration = Clock::now() - sleep_start;
            sleep_deviation = sleep_duration - target_sleep_duration;
        }
        else {
            // It's important to save a deviation of zero in history, even if no sleep happened:
            // With this, a very large sleep deviation slowly gets pushed out of history. After that,
            // prev_maxSleepDeviation might become much smaller and normal sleeps can be made again.
            sleep_deviation = 0s;
        }

        //Debug{} << "sleep deviation =" << sleep_deviation_us << "us";

        /*if (sleep_deviation_us > biggestSleepDeviation_us)
            biggestSleepDeviation_us = sleep_deviation_us;
        Debug{} << "maxsleepdev =" << biggestSleepDeviation_us;*/

        /*auto sleep_end = tick_end + sleep_length;
        do {
            std::this_thread::yield();
        } while (Clock::now() < sleep_end);*/

        // TODO time this code outside of timing code, does it have a tick duration effect?

        // Save current server frame performance statistics in history
        _sv_hist_tickDuration  [_sv_frameHistoryWritePos] = tick_duration;
        _sv_hist_sleepDeviation[_sv_frameHistoryWritePos] = sleep_deviation;

        ++_sv_frameHistoryWritePos;
        if (_sv_frameHistoryWritePos == SERVER_FRAME_HISTORY_MAX_LEN)
            _sv_frameHistoryWritePos = 0;

        if (_sv_frameHistorySize < SERVER_FRAME_HISTORY_MAX_LEN)
            ++_sv_frameHistorySize;

        // Calculate standard deviation of the last couple server frames' tick durations
        auto tickDurationStandardDeviation = calc_standard_deviation(_sv_hist_tickDuration.data(), _sv_frameHistorySize);

        // Get biggest sleep deviation of the last couple server frames
        Clock::duration maxSleepDeviation = 0s;
        for (size_t i = 0; i < _sv_frameHistorySize; ++i)
            if (_sv_hist_sleepDeviation[i] > maxSleepDeviation)
                maxSleepDeviation = _sv_hist_sleepDeviation[i];

        //Debug{} << "maxSleepDeviation =" << maxSleepDeviation_us << "us";


        PerformanceStats perfStat;
        perfStat.tick_duration_ms          = microseconds_count(tick_duration) / 1000.0f;
        perfStat.tick_duration_deviation_ms = microseconds_count(tickDurationStandardDeviation) / 1000.0f;
        perfStat.tick_start_deviation_rel   = frameStartDeviation_rel;
        perfStat.max_sleep_deviation_ms     = microseconds_count(maxSleepDeviation) / 1000.0f;
        _sh_performanceStats = perfStat; // Atomically overwrite member

        // Sleep until the next tick start with a more precise busy-sleep method
        while (Clock::now() < target_nextFrameStart)
            std::this_thread::yield();

        // Give info to the following server frame about the current frame
        prev_maxSleepDeviation = maxSleepDeviation;
        // Increment tick count
        ++tickCount;
    }

    return simulationStart + tickCount * targetFrameDuration; // When the next tick is or was supposed to start
}
