#ifndef SIM_SIM_H
#define SIM_SIM_H

#include <chrono>

namespace sim {

    // Namespace alias, so all game code can agree on timings
    // Used clock must be monotonic, i.e. time never decreases
    using Clock = std::chrono::steady_clock;

    // Unique identifier of a game tick
    using TickID = size_t;

    
    size_t GetTimeIntervalInTicks(float interval_secs, float sim_step_size_secs);


}

#endif // SIM_SIM_H
