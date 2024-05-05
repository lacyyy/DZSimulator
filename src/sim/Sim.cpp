#include "sim/Sim.h"

#include <cassert>

using namespace sim;

size_t sim::GetTimeIntervalInTicks(float interval_secs,
                                   float sim_step_size_secs)
{
    assert(interval_secs >= 0.0f);
    assert(sim_step_size_secs > 0.0f);
    return (size_t)(interval_secs / sim_step_size_secs + 0.5f);
}
