#ifndef COLL_COLLIDABLEWORLD_H_
#define COLL_COLLIDABLEWORLD_H_

#include <memory>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "coll/SweptTrace.h"
#include "csgo_parsing/BspMap.h"

// Forward-declare WorldCreator outside namespace to avoid ambiguity
class WorldCreator;

namespace coll {

// Test whether two axis-aligned bounding boxes (AABBs) intersect.
bool AabbIntersectsAabb(const Magnum::Vector3& mins0, const Magnum::Vector3& maxs0,
                        const Magnum::Vector3& mins1, const Magnum::Vector3& maxs1);

// Returns true if trace hits AABB, false if not.
// If AABB is hit, hit_fraction gets set to the fraction of the point in time of
// collision. hit_fraction is not modified otherwise!
bool IsAabbHitByFullSweptTrace(const Magnum::Vector3& ray_start,
    const Magnum::Vector3& inv_delta, const Magnum::Vector3& ray_extents,
    const Magnum::Vector3& aabb_mins, const Magnum::Vector3& aabb_maxs,
    float* hit_fraction = nullptr);


// Map-specific collision-related data container
class CollidableWorld {
public:
    // Tell CollidableWorld which BspMap it references
    CollidableWorld(std::shared_ptr<const csgo_parsing::BspMap> bsp_map);

    // Perform a swept trace against the entire world.
    // Does nothing if sweep distance is zero or nearly zero.
    // CAUTION: Not thread-safe yet!
    void DoSweptTrace(SweptTrace* trace);

    // Only displacements that don't have the NO_HULL_COLL flag are considered.
    bool DoesAabbIntersectAnyDisplacement(
        const Magnum::Vector3& aabb_mins,
        const Magnum::Vector3& aabb_maxs);

private:
    // Estimate trace cost of each object type
    uint64_t GetSweptTraceCost_Brush       (uint32_t      brush_idx); // idx into BspMap.brushes
    uint64_t GetSweptTraceCost_Displacement(uint32_t   dispcoll_idx); // idx into CDispCollTree array
    uint64_t GetSweptTraceCost_FuncBrush   (uint32_t func_brush_idx); // idx into BspMap.entities_func_brush
    uint64_t GetSweptTraceCost_StaticProp  (uint32_t      sprop_idx); // idx into BspMap.static_props
    uint64_t GetSweptTraceCost_DynamicProp (uint32_t      dprop_idx); // idx into BspMap.relevant_dynamic_props

    // Sweep trace against single objects
    void DoSweptTrace_Brush       (SweptTrace* trace, uint32_t      brush_idx); // idx into BspMap.brushes
    void DoSweptTrace_Displacement(SweptTrace* trace, uint32_t   dispcoll_idx); // idx into CDispCollTree array
    void DoSweptTrace_FuncBrush   (SweptTrace* trace, uint32_t func_brush_idx); // idx into BspMap.entities_func_brush
    void DoSweptTrace_StaticProp  (SweptTrace* trace, uint32_t      sprop_idx); // idx into BspMap.static_props
    void DoSweptTrace_DynamicProp (SweptTrace* trace, uint32_t      dprop_idx); // idx into BspMap.relevant_dynamic_props

private:
    // Use "pImpl" technique to keep this header file as light as possible.
    struct Impl;
    std::unique_ptr<Impl> pImpl;

    // Let some classes access private members:
    friend class ::WorldCreator; // WorldCreator initializes this class
    friend class BVH;            // BVH is heavily tied to this class
    friend class Debugger;       // Debugger needs to debug
    friend class Benchmark;      // Benchmarks need to benchmark
};

} // namespace coll

#endif // COLL_COLLIDABLEWORLD_H_
