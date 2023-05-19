#include "BulletPhysicsTest.h"

#include "Corrade/Utility/Debug.h"
#include "Magnum/Magnum.h"

using namespace Magnum;

void do_bullet_physics_test() {
    Debug{} << "physics test";
    // TODO print bullet version

    // Notes on potentially using Bullet Physics:
    //
    //   - There is an Emscripten port for Bullet: https://doc.magnum.graphics/magnum/building-integration.html#building-integration-features-emscripten-ports
    //   - Build options for Bullet in top-level CMakeLists.txt are not tested
}
