#include "sim/Sim.h"

#include <cassert>

#include <Magnum/Math/Time.h>

using namespace sim;
using namespace Magnum;
using namespace Magnum::Math::Literals;

SimTimeDur sim::RoundToNearestSimTimeStep(float unrounded_duration_secs,
                                          SimTimeDur simtime_step_size)
{
    assert(unrounded_duration_secs >= 0.0f);
    assert(simtime_step_size > 0.0_sec);

    int nearest_step_cnt = (int)(
        unrounded_duration_secs / (float)Seconds{ simtime_step_size } + 0.5f
    );
    SimTimeDur rounded_duration = nearest_step_cnt * simtime_step_size;
    return rounded_duration;
}
