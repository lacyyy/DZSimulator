#include <memory>
#include <mutex>

#include "coll/CollidableWorld.h"

// Protects std::cout
std::mutex g_cout_mutex;

// World data for easy access
std::shared_ptr<coll::CollidableWorld> g_coll_world;
