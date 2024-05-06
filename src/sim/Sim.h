#ifndef SIM_SIM_H
#define SIM_SIM_H

#include <chrono>

#include <Magnum/Magnum.h>

namespace sim {

    // ---- Terminology: "Simulation Time" ---
    // Simulation time refers to the virtual time that progresses inside a
    // simulated Counter-Strike game. Simulation time only progresses in
    // discrete steps as game ticks are computed.
    // Simulation time is separate from physical, wall-clock time.
    // Not computing game ticks results in a halt of simulation time.

    using SimTimePoint = Magnum::Nanoseconds; // A time point in simulation time
    using SimTimeDur   = Magnum::Nanoseconds; // A duration in simulation time

    // Round a duration in simulation time to the nearest multiple of the
    // given simulation time step size.
    SimTimeDur RoundToNearestSimTimeStep(float unrounded_duration_secs,
                                         SimTimeDur simtime_step_size);

}

#endif // SIM_SIM_H
