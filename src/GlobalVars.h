#ifndef GLOBALVARS_H_
#define GLOBALVARS_H_

#include <memory>
#include <mutex>

#include "coll/CollidableWorld.h"

#define ACQUIRE_COUT(x) {std::lock_guard<std::mutex> _cout_mtx(g_cout_mutex);x}

extern std::mutex g_cout_mutex; // Protects std::cout

// World data
extern std::shared_ptr<coll::CollidableWorld> g_coll_world;

#endif // GLOBALVARS_H_
