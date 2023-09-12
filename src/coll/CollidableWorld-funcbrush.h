#ifndef COLL_COLLIDABLEWORLD_FUNCBRUSH_H_
#define COLL_COLLIDABLEWORLD_FUNCBRUSH_H_

#include <Magnum/Math/Vector3.h>

#include "csgo_parsing/BspMap.h"

namespace coll {

    // func_brush_idx is an index into bsp_map.entities_func_brush .
    // If func_brush is invalid, false is returned and aabb_mins and aabb_maxs
    // do not get set.
    bool CalcAabb_FuncBrush(size_t func_brush_idx,
        const csgo_parsing::BspMap& bsp_map,
        Magnum::Vector3* aabb_mins,
        Magnum::Vector3* aabb_maxs);


    // ... (Add further func_brush-related collision code here)


} // namespace coll

#endif // COLL_COLLIDABLEWORLD_FUNCBRUSH_H_
