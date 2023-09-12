#ifndef COLL_COLLIDABLEWORLD_STATICPROP_H_
#define COLL_COLLIDABLEWORLD_STATICPROP_H_

#include <vector>

#include <Corrade/Containers/Optional.h>

#include "csgo_parsing/BspMap.h"

namespace coll {

// A collision model consists of a list of sections. A section is a list of
// triangles that describe a convex shape.
struct CollisionModel { // For the sake of precomputation, we store both planes and tris.

    // NOTE: Collision models might have *slightly* concave sections!
    //       Effects of this are unknown.

    // Section indexing is identical between section_planes and section_tris.
    // Inside a section, a triangle index corresponds to the associated plane index.

    // Planes of each (convex) section
    std::vector<std::vector<csgo_parsing::BspMap::Plane>> section_planes;

    // Triangles of each (convex) section.
    // @Optimization Compress triangle data(gives big memory savings).
    //               Make an array of unique vertices and define tris as uint16
    //               indices into the vertex array.
    //               Could half floats be precise enough for vert positions?
    //               If yes, look up available half float intrinsics.
    std::vector<std::vector<std::vector<Magnum::Vector3>>> section_tris;
};


// Precomputed data per static prop to speed up collision calculations
struct CollisionCache_StaticProp {
    // Exact, non-bloated AABB of static prop
    Magnum::Vector3 aabb_mins;
    Magnum::Vector3 aabb_maxs;
};

// Returns an empty Optional if collision cache creation fails.
Corrade::Containers::Optional<CollisionCache_StaticProp>
    Create_CollisionCache_StaticProp(
        const csgo_parsing::BspMap::StaticProp& sprop,
        const CollisionModel& cmodel);

} // namespace coll

#endif // COLL_COLLIDABLEWORLD_STATICPROP_H_
