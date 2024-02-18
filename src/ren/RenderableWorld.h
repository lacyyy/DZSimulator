#ifndef REN_RENDERABLEWORLD_H_
#define REN_RENDERABLEWORLD_H_

#include <map>
#include <vector>

#include <Magnum/GL/Mesh.h>
#include <Magnum/Magnum.h>
#include <Magnum/Tags.h>

#include "csgo_parsing/BrushSeparation.h"

// Forward-declare WorldCreator outside namespace to avoid ambiguity
class WorldCreator;

namespace ren {

class RenderableWorld { // Map-specific rendering-related data container
public:
    // Mesh for each brush category
    std::map<csgo_parsing::BrushSeparation::Category, Magnum::GL::Mesh>
        brush_category_meshes;

    // Mesh of all trigger_push entities that can push players
    Magnum::GL::Mesh trigger_push_meshes         { Magnum::NoCreate };

    // Displacement meshes
    Magnum::GL::Mesh mesh_displacements          { Magnum::NoCreate };
    Magnum::GL::Mesh mesh_displacement_boundaries{ Magnum::NoCreate };

    // Collision model meshes of solid props (static or dynamic)
    std::vector<Magnum::GL::Mesh> instanced_xprop_meshes;


private:
    // WorldCreator initializes this class, let it access private members.
    friend class ::WorldCreator;
};

} // namespace ren

#endif // REN_RENDERABLEWORLD_H_
