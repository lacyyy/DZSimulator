#ifndef COLL_BENCHMARK_H_
#define COLL_BENCHMARK_H_

// CAUTION: Remember to disable this when making a public release!
#define COLL_BENCHMARK_ENABLED 0 // Turn compilation of collision benchmarks on/off

#if COLL_BENCHMARK_ENABLED

#include <optional>
#include <vector>

#include <Corrade/Containers/String.h>
#include <Magnum/Math/Vector3.h>

#include "coll/SweptTrace.h"
#include "coll/BVH.h"

namespace coll {

class Benchmark {
public:

    // Benchmark swept hull trace performance against static props.
    // Performs tests using static props of currently loaded map.
    // NOTE: Other threads shouldn't be running, they might mess up measurements.
    static void StaticPropHullTracing();

    // Benchmark bevel plane generation of static props.
    // Performs tests using static props of currently loaded map.
    // NOTE: Other threads shouldn't be running, they might mess up measurements.
    static void StaticPropBevelPlaneGen();

    ////////////////////////////////////////////////////////////////////////////

    // TODO This function should be useful elsewhere too, move it out of here.
    static Corrade::Containers::String GetDurationStr(float duration_ns);
    // TODO This function should be useful elsewhere too, move it out of here.
    static Corrade::Containers::String GetPercentStr(float fraction,
                                                bool include_plus_sign = false);

    struct BenchmarkStatistics {
        float mean;
        float stddev;
        unsigned long long min, max;
        unsigned long long median;
        unsigned long long _5th_percentile;  // 5% of durations are less than or equal to this
        unsigned long long _95th_percentile; // 5% of durations are more than or equal to this
    };
    // TODO This function should be useful elsewhere too, move it out of here.
    static BenchmarkStatistics CalcDurationStats(
                                     std::vector<unsigned long long> durations);

    static std::vector<size_t> GetBvhLeafIndicesOfStaticPropsByTriCount(
                                                         bool big_sprops_first);

    template<class Generator>
    static Magnum::Vector3 GenRandomDir(Generator& gen);

    template<class Generator>
    static std::optional<SweptTrace> GenRealisticTrace(Generator& gen,
                                                       const BVH::Leaf& leaf);

    static bool CompareTraceResults(
        const SweptTrace::Info& trace_info,
        const SweptTrace::Results& ground_truth,
        const SweptTrace::Results& untested_results);
};

} // namespace coll
#endif // #if COLL_BENCHMARK_ENABLED

#endif // COLL_BENCHMARK_H_
