#ifndef GLOBALVARS_H_
#define GLOBALVARS_H_

#include <mutex>

#define ACQUIRE_COUT(x) {std::lock_guard<std::mutex> _cout_mtx(g_cout_mutex);x}

extern std::mutex g_cout_mutex; // Protects std::cout

#endif // GLOBALVARS_H_