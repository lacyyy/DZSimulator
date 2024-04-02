#ifndef COLL_COLLIDABLEWORLD_H_
#define COLL_COLLIDABLEWORLD_H_

#include <memory>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "coll/Trace.h"
#include "csgo_parsing/BspMap.h"

// Forward-declare WorldCreator outside namespace to avoid ambiguity
class WorldCreator;

namespace coll {

// Test whether two axis-aligned bounding boxes (AABBs) intersect.
bool AabbIntersectsAabb(const Magnum::Vector3& mins0, const Magnum::Vector3& maxs0,
                        const Magnum::Vector3& mins1, const Magnum::Vector3& maxs1);


// Map-specific collision-related data container
class CollidableWorld {
public:
    // Tell CollidableWorld which BspMap it references
    CollidableWorld(std::shared_ptr<const csgo_parsing::BspMap> bsp_map);

    // Perform a swept or unswept trace against the entire world.
    // CAUTION: Not thread-safe yet!
    void DoTrace(Trace* trace);

private:
    // Estimate trace cost of each object type
    uint64_t GetTraceCost_Brush       (uint32_t      brush_idx); // idx into BspMap.brushes
    uint64_t GetTraceCost_Displacement(uint32_t   dispcoll_idx); // idx into CDispCollTree array
    uint64_t GetTraceCost_FuncBrush   (uint32_t func_brush_idx); // idx into BspMap.entities_func_brush
    uint64_t GetTraceCost_StaticProp  (uint32_t      sprop_idx); // idx into BspMap.static_props
    uint64_t GetTraceCost_DynamicProp (uint32_t      dprop_idx); // idx into BspMap.relevant_dynamic_props

    // Sweep trace against single objects
    void DoSweptTrace_Brush       (Trace* trace, uint32_t      brush_idx); // idx into BspMap.brushes
    void DoSweptTrace_Displacement(Trace* trace, uint32_t   dispcoll_idx); // idx into CDispCollTree array
    void DoSweptTrace_FuncBrush   (Trace* trace, uint32_t func_brush_idx); // idx into BspMap.entities_func_brush
    void DoSweptTrace_StaticProp  (Trace* trace, uint32_t      sprop_idx); // idx into BspMap.static_props
    void DoSweptTrace_DynamicProp (Trace* trace, uint32_t      dprop_idx); // idx into BspMap.relevant_dynamic_props

    // Non-moving trace (static intersection test) against single objects
    void DoUnsweptTrace_Brush       (Trace* trace, uint32_t      brush_idx); // idx into BspMap.brushes
    void DoUnsweptTrace_Displacement(Trace* trace, uint32_t   dispcoll_idx); // idx into CDispCollTree array
    void DoUnsweptTrace_FuncBrush   (Trace* trace, uint32_t func_brush_idx); // idx into BspMap.entities_func_brush
    void DoUnsweptTrace_StaticProp  (Trace* trace, uint32_t      sprop_idx); // idx into BspMap.static_props
    void DoUnsweptTrace_DynamicProp (Trace* trace, uint32_t      dprop_idx); // idx into BspMap.relevant_dynamic_props

private:
    // Use "pImpl" technique to keep this header file as light as possible.
    struct Impl;
    std::unique_ptr<Impl> pImpl;

    // Let some classes access private members:
    friend class ::WorldCreator; // WorldCreator initializes this class
    friend class BVH;            // BVH is heavily tied to this class
    friend class Debugger;       // Debugger needs to debug
    friend class Benchmark;      // Benchmarks need to benchmark

    // Let some functions access private members:
    friend void DoTrace_StaticProp(Trace* trace, uint32_t sprop_idx,
                                   CollidableWorld& c_world);
    friend void DoTrace_DynamicProp(Trace* trace, uint32_t dprop_idx,
                                    CollidableWorld& c_world);
};

} // namespace coll

#endif // COLL_COLLIDABLEWORLD_H_
