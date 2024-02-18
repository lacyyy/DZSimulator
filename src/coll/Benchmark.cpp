// What we are primarily optimizing for:
//   - Minimizing the computation time of those traces that are performed
//     while the player is rampsliding or flying through the air.

// What we are NOT primarily optimizing for:
//   - Other trace types, e.g. when the player is walking/standing on ground.
//     (Reason: A trace-heavy rampslide route calculator feature is planned,
//     which ignores routes where a player stands on ground.)


#include "coll/Benchmark.h"
#if COLL_BENCHMARK_ENABLED // When disabled, don't waste time compiling this file

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <optional>
#include <random>

#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector3.h>

#include "coll/CollidableWorld_Impl.h"
#include "csgo_parsing/BspMap.h"
#include "GlobalVars.h"

using namespace coll;
using namespace csgo_parsing;
using namespace Corrade;
using namespace Corrade::Containers::Literals;
using namespace Magnum;
using Plane = csgo_parsing::BspMap::Plane;

struct SingleSPropBenchmark { // Info of benchmarking a single static prop
    size_t sprop_idx;    // idx into BspMap.static_props
    size_t bvh_leaf_idx; // idx into BVH.leaves

    struct UniqueTrace { // A unique, realistic trace against this sprop
        SweptTrace::Info realistic_trace_info; // Benchmark input
        SweptTrace::Results correct_results;

        std::vector<unsigned long long> mean_duration_ns_per_method; // Mean duration of each method
        std::vector<SweptTrace::Results> results_per_method; // Benchmark output of each method
    };
    std::vector<UniqueTrace> unique_traces;

    // Statistics across all unique traces, for each method
    std::vector<Benchmark::BenchmarkStatistics> stats_per_method;
};

void Benchmark::StaticPropHullTracing()
{
    // Realistic properties of static-prop-hitting traces that need optimization:
    //   - Does not start inside a static prop (Impossible in regular play)
    //   - Depending on broad-phase filtering, one of the following:
    //     - Trace hits static prop's AABB
    //     - Trace-enclosing AABB intersects with static prop's AABB
    //   - One of these 2 trace types:
    //     - Player hull trace in random direction
    //       - half_extents = (16, 16, 36), delta.length = 0.01 up to 95
    //     - One of the player hull quadrant traces, straight downwards
    //       - half_extents = (8, 8, 36), delta = (0, 0, -x)

    if (!g_coll_world) return;

    unsigned int trng_val = std::random_device{}();
    unsigned int seed = trng_val;
    Debug{} << "[Benchmark::StaticPropHullTracing] Used seed:" << seed; // To let user reproduce this benchmark
    std::mt19937 gen{seed};

    // Benchmark settings
    constexpr bool ONLY_COMPARE_METHOD_OUTPUTS = false; // For when only output comparison matters. Makes benchmark fast.
    constexpr bool PRINT_MEAN_OF_ALL_UNIQUE_TRACES = false; // Verbose option

    const bool START_WITH_BIG_SPROPS = true; // false -> start with small sprops
    // For each unique/realistic trace, for each benchmarked method, do X iterations.
    constexpr size_t NUM_BENCHMARKED_METHODS = 1; // When increasing this, ensure to modify innermost switch statement too
    constexpr size_t NUM_REALISTIC_TRACES = 40;
    constexpr size_t NUM_ITERATIONS = ONLY_COMPARE_METHOD_OUTPUTS ? 1 : 100; // Set high for accuracy! How often to repeat trace per benchmark method
    assert(NUM_ITERATIONS > 0);

    std::vector<SingleSPropBenchmark> sprop_benchmarks;
    std::vector<size_t> method_num_incorrect(NUM_BENCHMARKED_METHODS, 0); // How often a method produced incorrect output
    size_t total_num_unique_traces = 0; // # of unique traces across all sprops

    // Go through every solid static prop
    std::vector<size_t> sprop_leaf_indices = GetBvhLeafIndicesOfStaticPropsByTriCount(START_WITH_BIG_SPROPS);
    for (size_t e = 0; e < sprop_leaf_indices.size(); e++) {
        const BVH::Leaf& leaf = g_coll_world->pImpl->bvh->leaves[sprop_leaf_indices[e]];

        sprop_benchmarks.push_back({});
        SingleSPropBenchmark& sprop_bench = sprop_benchmarks.back();
        sprop_bench.sprop_idx = leaf.sprop_idx;
        sprop_bench.bvh_leaf_idx = sprop_leaf_indices[e];

        // Generate realistic traces and corresponding trace results
        sprop_bench.unique_traces.reserve(NUM_REALISTIC_TRACES);
        size_t num_attempts = 0;
        while (sprop_bench.unique_traces.size() < NUM_REALISTIC_TRACES) {
            num_attempts++;
            std::optional<SweptTrace> r_tr = GenRealisticTrace(gen, leaf);
            if (!r_tr) continue; // Failed to generate realistic trace
            SingleSPropBenchmark::UniqueTrace ut = {
                // This trace is realistic, save its info and results
                .realistic_trace_info = r_tr->info,
                .correct_results = r_tr->results
            };
            sprop_bench.unique_traces.push_back(ut);
        }
        total_num_unique_traces += sprop_bench.unique_traces.size();

        Debug{Debug::Flag::NoSpace} << "sprop " << e+1 << " / " << sprop_leaf_indices.size()
            << " (Leaf #" << sprop_leaf_indices[e] << "): Generated "
            << sprop_bench.unique_traces.size() << " traces after " << num_attempts << " attempts";

        // For each unique trace
        for (size_t u_tr_idx = 0; u_tr_idx < sprop_bench.unique_traces.size(); u_tr_idx++) {
            SingleSPropBenchmark::UniqueTrace& u_tr = sprop_bench.unique_traces[u_tr_idx];

            // For each benchmark method
            for (size_t method_idx = 0; method_idx < NUM_BENCHMARKED_METHODS; method_idx++) {
                // Set up iterations
                std::vector<SweptTrace> iter_traces;
                iter_traces.reserve(NUM_ITERATIONS);
                for (size_t i = 0; i < NUM_ITERATIONS; i++) // Precreate traces with info and empty results
                    iter_traces.emplace_back(u_tr.realistic_trace_info);

                // Run iterations and measure CPU time precisely (Not wall time!) (If possible)
#ifndef _WIN32
#error [DZSimulator Benchmarking] This benchmark code was written only for Windows. To get precise benchmarks, you should use your OS's most precise CPU time methods in this place.
#endif
                // On Windows, std::chrono::high_resolution_clock is the most precise clock, but sadly wall time.
                auto method_iters_start = std::chrono::high_resolution_clock::now();
                for (SweptTrace& trace : iter_traces) {
                    switch (method_idx) {
                        case 0:
                            g_coll_world->DoSweptTrace_StaticProp(&trace, sprop_bench.sprop_idx); // not visualized, correct benchmark procedure
//                            g_coll_world->DoSweptTrace(&iter_trace); // visualized, incorrect benchmark procedure
                            break;
//                        case 1:
//                            g_coll_world->DoSweptTrace_StaticProp_New1(&trace, sprop_bench.sprop_idx);
//                            break;
//                        case 2:
//                            g_coll_world->DoSweptTrace_StaticProp_New2(&trace, sprop_bench.sprop_idx);
//                            break;
                    }
                }
                auto method_iters_end = std::chrono::high_resolution_clock::now();
                unsigned long long method_duration_sum_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(method_iters_end - method_iters_start).count();

                // Set method results
                unsigned long long method_mean_duration_ns = method_duration_sum_ns / NUM_ITERATIONS;
                u_tr.         results_per_method.push_back(iter_traces[0].results);
                u_tr.mean_duration_ns_per_method.push_back(method_mean_duration_ns);

                // Validate benchmark outputs
                if (!CompareTraceResults(u_tr.realistic_trace_info, u_tr.correct_results, iter_traces[0].results)) {
                    Debug{} << "-> Method" << method_idx << "produced incorrect results for trace" << u_tr_idx;
                    method_num_incorrect[method_idx] += 1;
                }
            }
            if (PRINT_MEAN_OF_ALL_UNIQUE_TRACES && !ONLY_COMPARE_METHOD_OUTPUTS) {
                for (size_t method_idx = 0; method_idx < NUM_BENCHMARKED_METHODS; method_idx++) {
                    unsigned long long mean_ns = u_tr.mean_duration_ns_per_method[method_idx];
                    bool is_fastest = true; // If this method has the lowest mean time for this trace
                    for (auto other_mean_ns : u_tr.mean_duration_ns_per_method) if (other_mean_ns < mean_ns) is_fastest = false;

                    Debug d{ Debug::Flag::NoSpace }; d << Debug::color(is_fastest ? Debug::Color::Green : Debug::Color::Default);
                    if (method_idx == 0) d << "TR " << u_tr_idx << " ";
                    else                 d << "     " << (u_tr_idx >= 10 ? " " : "") << (u_tr_idx >= 100 ? " " : "") << (u_tr_idx >= 1000 ? " " : "") << (u_tr_idx >= 10000 ? " " : "");
                    d << "Method " << method_idx << " " << GetDurationStr((float)mean_ns);
                }
            }
#if 0 // TEST CODE: Show difference of method 1's u_trace mean compared to method 0's u_trace mean in percentage points.
            double rel = ((double)u_tr.mean_duration_ns_per_method[1] - (double)u_tr.mean_duration_ns_per_method[0]) / (double)u_tr.mean_duration_ns_per_method[0];
            Debug{} << GetPercentStr((float)rel);
#endif
        }

        // Calc statistics of static prop, per method
        for (size_t method_idx = 0; method_idx < NUM_BENCHMARKED_METHODS; method_idx++) {
            std::vector<unsigned long long> durations; durations.reserve(sprop_bench.unique_traces.size());
            for (const SingleSPropBenchmark::UniqueTrace& u_tr : sprop_bench.unique_traces)
                durations.push_back(u_tr.mean_duration_ns_per_method[method_idx]);
            sprop_bench.stats_per_method.push_back(CalcDurationStats(durations));
        }

        // For each method, across all unique traces of static prop, print mean and stddev of their means
        for (size_t method_idx = 0; method_idx < NUM_BENCHMARKED_METHODS; method_idx++) {
            const BenchmarkStatistics& method_stats = sprop_bench.stats_per_method[method_idx];

            bool is_fastest = true; // If this method has the lowest mean time
            for (BenchmarkStatistics& s : sprop_bench.stats_per_method) if (s.mean < method_stats.mean) is_fastest = false;

            Debug::Color method_col = is_fastest ? Debug::Color::Green : Debug::Color::Default;
            Debug d{ Debug::Flag::NoSpace };
            d << Debug::color(method_col) << "- Method " << method_idx << Debug::resetColor << " ";

            if (method_idx == 0) {
                d << "         ";
            } else {
                // Print mean difference compared to method 0 in percent
                float sprop_mean_fraction = (method_stats.mean - sprop_bench.stats_per_method[0].mean) / sprop_bench.stats_per_method[0].mean;
                Debug::Color col = method_col;
                if (          sprop_mean_fraction >= 0.02f) d << Debug::color(Debug::Color::Red); // Red color when mean is 2% higher or more
                if (Math::abs(sprop_mean_fraction) < 0.01f) d << Debug::boldColor(Debug::Color::Black); // 'Bold Black' is actually gray on Windows!
                d << "(" << GetPercentStr(sprop_mean_fraction, true) << ") " << Debug::resetColor;
            }

            // Color depending on time thresholds? white, green, cyan, yellow, magenta, red
            d << Debug::color(method_col) << GetDurationStr(method_stats.mean) << Debug::resetColor;
            d << " ± ";
            d << Debug::color(method_col) << GetPercentStr(method_stats.stddev / method_stats.mean) << Debug::resetColor;
            d << " (max=" << Debug::color(Debug::Color::Cyan) << GetDurationStr(method_stats.max)              << Debug::resetColor;
            d << ",95%="  << Debug::color(Debug::Color::Cyan) << GetDurationStr(method_stats._95th_percentile) << Debug::resetColor;
            d << ",50%="  << Debug::color(Debug::Color::Cyan) << GetDurationStr(method_stats.median)           << Debug::resetColor;
            d << ",5%="   << Debug::color(Debug::Color::Cyan) << GetDurationStr(method_stats._5th_percentile)  << Debug::resetColor;
            d << ",min="  << Debug::color(Debug::Color::Cyan) << GetDurationStr(method_stats.min)              << Debug::resetColor;
            d << ")";
        }
    }

    Debug{} << "------------------------";

    // Print statistics across all static props
    for (size_t method_idx = 0; method_idx < NUM_BENCHMARKED_METHODS; method_idx++) {
        uint64_t overall_sum_ns = 0; // ns
        uint64_t overall_weighted_sum_nsm3 = 0; // ns * m^3
        double sprop_volume_sum_m3 = 0.0; // m^3 (Volume of all sprops)
        for (const SingleSPropBenchmark& sprop_b : sprop_benchmarks) {
            const BVH::Leaf& leaf = g_coll_world->pImpl->bvh->leaves[sprop_b.bvh_leaf_idx];
            double sprop_volume_m3 = (leaf.maxs - leaf.mins).product() / 61023.7; // cubic meters
            double sprop_mean_ns = sprop_b.stats_per_method[method_idx].mean;

            overall_sum_ns            += (uint64_t)(sprop_mean_ns);
            overall_weighted_sum_nsm3 += (uint64_t)(sprop_mean_ns * sprop_volume_m3);
            sprop_volume_sum_m3 += sprop_volume_m3;
        }
        Debug{} << "Method" << method_idx << "Overall mean:"
            << GetDurationStr(overall_sum_ns / sprop_benchmarks.size());
        Debug{} << "         Overall mean weighted by sprop volume:"
            << GetDurationStr(overall_weighted_sum_nsm3 / sprop_volume_sum_m3);
    }

    // Tell user if a method produced incorrect output
    for (size_t i = 0; i < NUM_BENCHMARKED_METHODS; i++) {
        if (method_num_incorrect[i] != 0)
            Debug{ Debug::Flag::NoSpace } << Debug::color(Debug::Color::Red)
                << "Method " << i << " produced incorrect trace results! ("
                << method_num_incorrect[i] << " / " << total_num_unique_traces
                << ", 1 in " << (total_num_unique_traces / method_num_incorrect[i]) << ")";
    }
    Debug{} << "[Benchmark::StaticPropHullTracing] Used seed:" << seed; // To let user reproduce this benchmark

    // Give user a reminder
    Debug{} << Debug::color(Debug::Color::Yellow) <<
        "If you're doing micro benchmarks, make sure you closed as many other "
        "desktop apps as possible and increased benchmark iterations to minimize"
        " timing errors!";

    // TODO Make amount of unique traces per sprop dependent on sprop's
    //      (extended) AABB volume? At what unique_trace per sprop volume ratio
    //      does the sprop mean stabilize?
}

static std::vector<Plane> GenAllBevelPlanesOfSPropSection(
    const CollisionModel&            sprop_coll_model,
    const CollisionCache_XProp& sprop_coll_cache,
    size_t idx_of_sprop_section)
{
    // This is a benchmarked method, intended to test correctness and measure
    // speed of generating all bevel planes of a static prop's section.
    XPropSectionBevelPlaneGenerator bevel_gen(sprop_coll_model, sprop_coll_cache,
                                              idx_of_sprop_section);
    std::vector<Plane> bevel_planes;

    Plane next_plane;
    while(bevel_gen.GetNext(&next_plane))
        bevel_planes.push_back(next_plane);
    return bevel_planes;
}

void Benchmark::StaticPropBevelPlaneGen()
{
    if (!g_coll_world || !g_coll_world->pImpl->sprop_coll_models) {
        assert(false);
        return;
    }

    constexpr size_t NUM_BENCHMARKED_METHODS = 1;
    constexpr size_t NUM_ITERATIONS = 200;
    std::vector<unsigned long long> method_durations_ns(NUM_BENCHMARKED_METHODS, 0);
    std::vector<size_t> method_num_planes(NUM_BENCHMARKED_METHODS, 0);
    std::vector<bool> method_incorrect(NUM_BENCHMARKED_METHODS, false); // Whether a method produced incorrect output

    size_t total_num_sprop_tris = 0; // # of triangles of all solid sprops in map
    size_t total_num_sprop_sections = 0; // # of sections of all solid sprops in map

    // For each static prop
    const bool START_WITH_BIG_SPROPS = true; //false;
    std::vector<size_t> sprop_leaf_indices = GetBvhLeafIndicesOfStaticPropsByTriCount(START_WITH_BIG_SPROPS);
    for (size_t e = 0; e < sprop_leaf_indices.size(); e++) {
        Debug{} << "sprop" << e << "/" << sprop_leaf_indices.size();
        const BVH::Leaf&          leaf     = g_coll_world->pImpl->bvh->leaves[sprop_leaf_indices[e]];
        const BspMap::StaticProp& sprop    = g_coll_world->pImpl->origin_bsp_map->static_props[leaf.sprop_idx];
        const std::string&        mdl_path = g_coll_world->pImpl->origin_bsp_map->static_prop_model_dict[sprop.model_idx];

        const auto& iter = g_coll_world->pImpl->sprop_coll_models->find(mdl_path);
        if (iter == g_coll_world->pImpl->sprop_coll_models->end())
            continue; // This static prop has no collision model, skip
        const CollisionModel& collmodel = iter->second;
        const size_t num_sections = collmodel.section_tri_meshes.size();

        auto coll_cache_it = g_coll_world->pImpl->coll_caches_sprop->find(leaf.sprop_idx);
        assert(coll_cache_it != g_coll_world->pImpl->coll_caches_sprop->end());
        const CollisionCache_XProp& coll_cache = coll_cache_it->second;

        // For each section
        for (size_t section_idx = 0; section_idx < num_sections; section_idx++) {
            total_num_sprop_tris += collmodel.section_tri_meshes[section_idx].tris.size();
            total_num_sprop_sections++;

            // For each method, generate section's bevel planes
            std::vector<std::vector<Plane>> method_results(NUM_BENCHMARKED_METHODS); // Results of each method
            for (size_t method_idx = 0; method_idx < NUM_BENCHMARKED_METHODS; method_idx++) {
                std::vector<Plane>& results = method_results[method_idx];

                auto gen_start = std::chrono::high_resolution_clock::now();
                // Run iterations
                for (size_t iteration = 0; iteration < NUM_ITERATIONS; iteration++) {
                    switch (method_idx) {
                        case 0:
                            results = GenAllBevelPlanesOfSPropSection(collmodel, coll_cache, section_idx);
                            break;
                        //case 1:
                        //    results = GenAllBevelPlanesOfSPropSection_New1(collmodel, coll_cache, section_idx);
                        //    break;
                        //case 2:
                        //    results = GenAllBevelPlanesOfSPropSection_New2(collmodel, coll_cache, section_idx);
                        //    break;
                    }
                }
                auto gen_end = std::chrono::high_resolution_clock::now();
                auto gen_duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(gen_end - gen_start).count();
                method_durations_ns[method_idx] += gen_duration_ns;
                method_num_planes[method_idx] += results.size();
            }

            // Validate output (bevel plane duplications don't invalidate output)
            // NOTE: This code is messy and ugly, but it does give useful warnings.
            const size_t reference_method_idx = 0; // Ground truth
            for (size_t method_idx = 0; method_idx < NUM_BENCHMARKED_METHODS; method_idx++) {
                if (method_idx == reference_method_idx) continue;
                // Each bevel plane of the reference output must occur in the tested output. If not -> Missing bevel planes
                for (Plane reference_plane : method_results[reference_method_idx]) {
                    bool found = false;
                    for (Plane p : method_results[method_idx])
                        if (Math::abs(p.normal[0] - reference_plane.normal[0]) < 0.01f &&
                            Math::abs(p.normal[1] - reference_plane.normal[1]) < 0.01f &&
                            Math::abs(p.normal[2] - reference_plane.normal[2]) < 0.01f &&
                            Math::abs(p.dist - reference_plane.dist) < 0.05f)
                            found = true;
                    if (!found) {
                        method_incorrect[method_idx] = true;
                        Error{} << "e =" << e << "s =" << section_idx << "Method" << method_idx << "is missing bevel planes from its output:";
                        Error{} << reference_plane.normal << reference_plane.dist;
                        float min_diff = FLT_MAX;
                        Plane min_diff_plane = { .normal={ 0.0f, 0.0f, 0.0f }, .dist = 0.0f };
                        for (Plane p : method_results[method_idx]) {
                            float diff = Math::abs(p.dist - reference_plane.dist) + (p.normal - reference_plane.normal).length();
                            if (diff < min_diff) {
                                min_diff = diff;
                                min_diff_plane = p;
                            }
                        }
                        Error{} << "Nearest match:" << min_diff_plane.normal << min_diff_plane.dist;
                        Error{} << "Test method plane count:" << method_results[method_idx].size();
                        break;
                    }
                }
                // Each bevel plane of the tested output must occur in the reference output. If not -> Erroneous extra bevel planes
                for (Plane p : method_results[method_idx]) {
                    //Debug{} << "Is plane" << p.normal << p.dist << "found in the reference plane list?";
                    bool found = false;
                    for (Plane reference_plane : method_results[reference_method_idx]) {
                        //Debug{} << "Is reference plane" << reference_plane.normal << reference_plane.dist << "similar?";
                        if (Math::abs(p.normal[0] - reference_plane.normal[0]) < 0.01f &&
                            Math::abs(p.normal[1] - reference_plane.normal[1]) < 0.01f &&
                            Math::abs(p.normal[2] - reference_plane.normal[2]) < 0.01f &&
                            Math::abs(p.dist - reference_plane.dist) < 0.05f) {
                            //Debug{} << "YES, SIMILAR! FOUND!";
                            found = true;
                        }
                    }
                    if (!found) {
                        method_incorrect[method_idx] = true;
                        Error{} << "e =" << e << "s =" << section_idx << "Method" << method_idx << "produced extra erroneous bevel planes!";
                        Error{} << p.normal << p.dist;
                        float min_diff = FLT_MAX;
                        Plane min_diff_plane = { .normal={ 0.0f, 0.0f, 0.0f }, .dist = 0.0f };
                        for (Plane reference_plane : method_results[reference_method_idx]) {
                            float diff = Math::abs(p.dist - reference_plane.dist) + (p.normal - reference_plane.normal).length();
                            if (diff < min_diff) {
                                min_diff = diff;
                                min_diff_plane = reference_plane;
                            }
                        }
                        Error{} << "Nearest match:" << min_diff_plane.normal << min_diff_plane.dist;
                        Error{} << "Reference plane count:" << method_results[reference_method_idx].size();
                        Error{} << "dist   diff:" << std::to_string(Math::abs(min_diff_plane.dist - p.dist));
                        Error{} << "normal diff:" << std::to_string((min_diff_plane.normal - p.normal).dot());
                        break;
                    }
                }
            }
        }
    }
    // Print time measurements
    for (size_t method_idx = 0; method_idx < NUM_BENCHMARKED_METHODS; method_idx++)
        Debug{} << "Method" << method_idx << "took" << GetDurationStr((float)method_durations_ns[method_idx]);
    // Calculate bevel plane LUT memory size
    size_t num_unique_edges = (total_num_sprop_tris * 3) / 2; // Every edge is used by 2 triangles
    // Print bevel plane details
    for (size_t method_idx = 0; method_idx < NUM_BENCHMARKED_METHODS; method_idx++) {
        Debug{} << "Method" << method_idx << "generated" << method_num_planes[method_idx] << "bevel planes";
        Debug{} << "  -> Total bevel plane array size:" << ((float)method_num_planes[method_idx] * 16.0f / 1048576.0f) << "MiB";
    }
    // Print other details
    Debug{} << "- Total # of solid sprops in map:" << sprop_leaf_indices.size();
    Debug{} << "- Total # of solid sprop sections in map:" << total_num_sprop_sections;

    // Tell user if a method produced incorrect output
    for (size_t i = 0; i < NUM_BENCHMARKED_METHODS; i++) {
        if (method_incorrect[i])
            Debug{} << Debug::color(Debug::Color::Red) << "Method" << i
                << "produced incorrect trace results!";
    }

    // Give user a reminder
    Debug{} << Debug::color(Debug::Color::Yellow) <<
        "If you're doing micro benchmarks, make sure you closed as many other "
        "desktop apps as possible and increased benchmark iterations to minimize"
        " timing errors!";
}

// Format nanosecond duration. Examples: " 2.82s", "978.0ms", " 12.3µs", "811.9ns"
// TODO This function should be useful elsewhere too, move it out of here.
Containers::String Benchmark::GetDurationStr(float duration_ns) {
    float value;
    Containers::String unit;
    if      (duration_ns >= 1e9f) { value = duration_ns * 1e-9f; unit = "s"_s;  }
    else if (duration_ns >= 1e6f) { value = duration_ns * 1e-6f; unit = "ms"_s; }
    else if (duration_ns >= 1e3f) { value = duration_ns * 1e-3f; unit = "µs"_s; }
    else                          { value = duration_ns;         unit = "ns"_s; }

    int decimal_places;
    if      (value >= 99.95f ) decimal_places = 1; // XYZ.Qms
    else if (value >=  9.995f) decimal_places = 1; //  XY.Zms
    else                       decimal_places = 2; //  X.YZms

    constexpr size_t BUF_SIZE = 16;
    char buf[BUF_SIZE];
    int cnt = std::snprintf(buf, BUF_SIZE, "%5.*f%s", decimal_places, value, unit.cbegin());
    if (cnt >= 0 && cnt < BUF_SIZE) // Success
        return { buf };
    else // Error occurred
        return "<error>"_s;
}

// Format percentage duration. Examples: "  0.3%", "-68.1%", "  5.0%"
// TODO This function should be useful elsewhere too, move it out of here.
Containers::String Benchmark::GetPercentStr(float fraction, bool include_plus_sign) {
    float percentage_points = 100.0f * fraction;
    constexpr size_t BUF_SIZE = 16;
    char buf[BUF_SIZE];
    int cnt = std::snprintf(buf, BUF_SIZE, include_plus_sign ? "%+5.1f%%" : "%5.1f%%", percentage_points);
    if (cnt >= 0 && cnt < BUF_SIZE) // Success
        return { buf };
    else // Error occurred
        return "<error>"_s;
}

// TODO This function should be useful elsewhere too, move it out of here.
Benchmark::BenchmarkStatistics Benchmark::CalcDurationStats(
    std::vector<unsigned long long> durations)
{
    std::vector<unsigned long long> sorted_arr(durations); // Copy vector
    std::sort(sorted_arr.begin(), sorted_arr.end());
    size_t num_samples = sorted_arr.size();

    unsigned long long total_sum = 0;
    for(unsigned long long iter_dur : sorted_arr) {
        total_sum += iter_dur;
    }
    float mean = (float)total_sum / (float)num_samples;

    double variance = 0.0;
    for(unsigned long long iter_dur : sorted_arr) {
        float deviation = (float)iter_dur - mean;
        variance += (double)(deviation * deviation);
    }
    variance /= (double)num_samples;
    float stddev = (float)std::sqrt(variance);

    unsigned long long median = sorted_arr.empty() ? 0 : sorted_arr[num_samples / 2];
    unsigned long long min = sorted_arr.empty() ? 0 : sorted_arr.front();
    unsigned long long max = sorted_arr.empty() ? 0 : sorted_arr.back();
    unsigned long long _5th_percentile  = sorted_arr.empty() ? 0 : sorted_arr[(size_t)(0.05 * num_samples)];
    unsigned long long _95th_percentile = sorted_arr.empty() ? 0 : sorted_arr[(size_t)(0.95 * num_samples)];

    return {
        .mean = mean,
        .stddev = stddev,
        .min = min,
        .max = max,
        .median = median,
        ._5th_percentile = _5th_percentile,
        ._95th_percentile = _95th_percentile
    };
}

// Returns BVH leaf indices of all static props, sorted by their triangle count,
// descending.
std::vector<size_t> Benchmark::GetBvhLeafIndicesOfStaticPropsByTriCount(bool big_sprops_first)
{
    auto GetSPropTriCount = [](size_t sprop_leaf_idx) -> size_t {
        const BVH::Leaf& leaf = g_coll_world->pImpl->bvh->leaves[sprop_leaf_idx];
        const BspMap::StaticProp& sprop =
            g_coll_world->pImpl->origin_bsp_map->static_props[leaf.sprop_idx];
        const std::string& mdlpath =
            g_coll_world->pImpl->origin_bsp_map->static_prop_model_dict[sprop.model_idx];
        const CollisionModel& collmodel =
            g_coll_world->pImpl->sprop_coll_models->at(mdlpath);

        size_t num_tris = 0;
        for (const auto& section_tri_mesh : collmodel.section_tri_meshes)
            num_tris += section_tri_mesh.tris.size();
        return num_tris;
    };

    std::vector<size_t> sprop_leaf_indices;
    for (size_t i = 1; i < g_coll_world->pImpl->bvh->leaves.size(); i++) {
        if (g_coll_world->pImpl->bvh->leaves[i].type == BVH::Leaf::Type::StaticProp)
            sprop_leaf_indices.push_back(i);
    }
    std::sort(sprop_leaf_indices.begin(), sprop_leaf_indices.end(),
        [&GetSPropTriCount, big_sprops_first](size_t leaf_idx_a, size_t leaf_idx_b) {
            // Static prop with more/less tris is ordered before the other
            if (big_sprops_first) return GetSPropTriCount(leaf_idx_a) > GetSPropTriCount(leaf_idx_b);
            else                  return GetSPropTriCount(leaf_idx_a) < GetSPropTriCount(leaf_idx_b);
        }
    );
    return sprop_leaf_indices;
}

// Returns random vector that's normalized and uniformly distributed on the
// unit sphere. Non-deterministic computation time!
template<class Generator>
Vector3 Benchmark::GenRandomDir(Generator& gen) {
    Vector3 out = { 0.0f, 0.0f, 0.0f };

    // Standard normal distribution
    static std::normal_distribution<float> dis{ 0.0f, 1.0f };

    while(out.dot() < 1e-8f) { // Avoid floating point instability
        out.x() = dis(gen);
        out.y() = dis(gen);
        out.z() = dis(gen);
    }
    return out.normalized();
}

// Tries to generate a realistic trace against a BVH leaf.
// Returns nothing if unrealistic trace was generated.
template<class Generator>
std::optional<SweptTrace> Benchmark::GenRealisticTrace(
    Generator& gen, const BVH::Leaf& leaf)
{
    assert(leaf.type == BVH::Leaf::Type::StaticProp); // Other types not tested

    static Vector3 trace_extents = {16.0f, 16.0f, 36.0f}; // Traced hull's half extents
    //static Vector3 trace_extents = {8.0f, 8.0f, 36.0f}; // Traced hull's half extents

    static std::uniform_real_distribution<float> trace_len_dis(0.01f, 95.0f);
    float trace_len = trace_len_dis(gen);
    Vector3 trace_delta = trace_len * GenRandomDir(gen);

    // Determine AABB of possible trace start points
    Vector3 trace_start_dis_mins = leaf.mins - trace_extents - Vector3(trace_len);
    Vector3 trace_start_dis_maxs = leaf.maxs + trace_extents + Vector3(trace_len);

    // Pick random point in that AABB
    Vector3 trace_start;
    for (int axis = 0; axis < 3; axis++) {
        std::uniform_real_distribution<float> distr(
            trace_start_dis_mins[axis],
            trace_start_dis_maxs[axis]);
        trace_start[axis] = distr(gen);
    }

    SweptTrace tr{trace_start, trace_start + trace_delta, -trace_extents, +trace_extents};

    // Filter out traces that don't hit the static prop's AABB
    if (!IsAabbHitByFullSweptTrace(tr.info.startpos, tr.info.invdelta,
                                   tr.info.extents, leaf.mins, leaf.maxs))
        return std::nullopt;

    // Trace against static prop using known-good reference trace function
    g_coll_world->DoSweptTrace_StaticProp(&tr, leaf.sprop_idx);

    // Filter out traces that start inside the static prop
    if (tr.results.startsolid)
        return std::nullopt;

    // Return realistic trace with its correct results
    return tr;
}

// Returns true if the results are (near) identical, false otherwise.
bool Benchmark::CompareTraceResults(
    const SweptTrace::Info& trace_info,
    const SweptTrace::Results& ground_truth,
    const SweptTrace::Results& untested_results)
{
    bool are_identical = true;
    if (ground_truth.startsolid   != untested_results.startsolid  ) { are_identical = false; Debug{} << Debug::color(Debug::Color::Red) << "[BenchmarkDiscrepancy] startsolid ="   << untested_results.startsolid   << "!=" << ground_truth.startsolid;   }
    if (ground_truth.allsolid     != untested_results.allsolid    ) { are_identical = false; Debug{} << Debug::color(Debug::Color::Red) << "[BenchmarkDiscrepancy] allsolid ="     << untested_results.allsolid     << "!=" << ground_truth.allsolid;     }
    if (ground_truth.surface      != untested_results.surface     ) { are_identical = false; Debug{} << Debug::color(Debug::Color::Red) << "[BenchmarkDiscrepancy] surface ="      << untested_results.surface      << "!=" << ground_truth.surface;      }
    if (!(ground_truth.plane_normal - untested_results.plane_normal).isZero()) { are_identical = false; Debug{} << Debug::color(Debug::Color::Red) << "[BenchmarkDiscrepancy] plane_normal =" << untested_results.plane_normal << "!=" << ground_truth.plane_normal; }

    bool is_previous_stat_discrepant = (are_identical == false);
    float frac_diff = Math::abs(ground_truth.fraction - untested_results.fraction);
    float frac_deltadiff = frac_diff * trace_info.delta.length();
    if (is_previous_stat_discrepant || frac_deltadiff > 0.05f) {
        are_identical = false;
        Debug{} << Debug::color(Debug::Color::Red) << "[BenchmarkDiscrepancy] fraction ="
            << untested_results.fraction     << "!=" << ground_truth.fraction
            << " --> deltadiff =" << frac_deltadiff;
    }
    return are_identical;
}

#endif // COLL_BENCHMARK_ENABLED
