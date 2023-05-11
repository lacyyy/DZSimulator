#ifndef SIM_SIM_H
#define SIM_SIM_H

#include <chrono>

namespace sim {

    // Namespace alias, so all game code can agree on timings
    // Used clock must be monotonic, i.e. time never decreases
    using Clock = std::chrono::steady_clock;

}

#endif // SIM_SIM_H
