#ifndef WORLDCREATOR_H_
#define WORLDCREATOR_H_

#include <utility>
#include <memory>
#include <string>

#include <Magnum/GL/Mesh.h>

#include "coll/CollidableWorld.h"
#include "csgo_parsing/BspMap.h"
#include "ren/RenderableWorld.h"

class WorldCreator {
public:

    // Creates RenderableWorld and CollidableWorld objects from a parsed CSGO
    // '.bsp' map file.
    // Error messages are put into the string pointed to by dest_errors.
    static
    std::pair<
        std::shared_ptr<ren::RenderableWorld>,
        std::shared_ptr<coll::CollidableWorld>>
    InitFromBspMap(
        std::shared_ptr<const csgo_parsing::BspMap> bsp_map,
        std::string* dest_errors = nullptr);

    // Mesh of Bump Mines thrown/placed into the world
    static Magnum::GL::Mesh CreateBumpMineMesh();

};

#endif // WORLDCREATOR_H_
