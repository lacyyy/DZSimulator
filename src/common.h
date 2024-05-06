#ifndef DZSIMULATOR_COMMON_H_
#define DZSIMULATOR_COMMON_H_

#include <chrono>

// Alias, so all DZSimulator code can agree on real-time time measurements.
// This clock is monotonic (time never decreases) and measures physical,
// real world time.
using WallClock = std::chrono::steady_clock;

#endif //DZSIMULATOR_COMMON_H_
