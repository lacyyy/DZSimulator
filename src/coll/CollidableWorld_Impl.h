#ifndef COLL_COLLIDABLEWORLD_IMPL_H_
#define COLL_COLLIDABLEWORLD_IMPL_H_

#include <map>
#include <memory>
#include <string>

#include <Corrade/Containers/Optional.h>

#include "coll/BVH.h"
#include "coll/CollidableWorld.h"
#include "coll/CollidableWorld-xprop.h"
#include "coll/CollidableWorld-displacement.h"
#include "csgo_parsing/BspMap.h"

namespace coll {

struct CollidableWorld::Impl {
    Impl(std::shared_ptr<const csgo_parsing::BspMap> bsp_map)
        : origin_bsp_map{ bsp_map }
    {}

    // Original CSGO map file this CollidableWorld object was created from
    std::shared_ptr<const csgo_parsing::BspMap> origin_bsp_map;



    // Before using these collision structures, make sure they hold a value!
    // I.e.:  if (var != Corrade::Containers::NullOpt) { ... }
    template<class T> using Optional = Corrade::Containers::Optional<T>;

    // Collision structures of displacements without the NO_HULL_COLL flag.
    Optional< std::vector<CDispCollTree> > hull_disp_coll_trees =
                                               { Corrade::Containers::NullOpt };

    // Collision models used in at least one solid prop (solid or dynamic).
    // Keys are MDL paths, values are collision models.
    Optional< std::map<std::string, CollisionModel> > xprop_coll_models =
                                               { Corrade::Containers::NullOpt };

    // Collision caches of each solid *static* prop.
    // Keys are indices into BspMap::static_props, values are the caches.
    Optional< std::map<uint32_t, CollisionCache_XProp> > coll_caches_sprop =
                                               { Corrade::Containers::NullOpt };

    // Collision caches of each solid *dynamic* prop.
    // Keys are indices into BspMap::relevant_dynamic_props, values are the caches.
    Optional< std::map<uint32_t, CollisionCache_XProp> > coll_caches_dprop =
                                               { Corrade::Containers::NullOpt };

    // Bounding volume hierarchy (BVH) that accelerates traces.
    // NOTE: This BVH must only be created after all other collision data
    //       (collision models, caches, etc., see above) was created!
    Optional< BVH > bvh =
                                               { Corrade::Containers::NullOpt };
};

} // namespace coll

#endif // COLL_COLLIDABLEWORLD_IMPL_H_
