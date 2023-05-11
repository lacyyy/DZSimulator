#include "build_info.h"

#include <string>

// version headers of libraries
#include <Corrade/version.h>           // repo version of "corrade"
#include <Magnum/version.h>            // repo version of "magnum"
#include <Magnum/versionIntegration.h> // repo version of "magnum-integration"
#include <Magnum/versionPlugins.h>     // repo version of "magnum-plugins"
#include <asio/version.hpp>            // version of "asio"
#include <httplib.h>                   // version of "cpp-httplib" (Note: Include httplib.h before Windows.h or include Windows.h by defining WIN32_LEAN_AND_MEAN beforehand.)
#include <imgui.h>                     // version of "imgui"
#include <json.hpp>                    // version of "json"
#include <SDL_version.h>               // version of "SDL"

#include "cmake_build_settings.h"

#include <Corrade/Corrade.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/Magnum.h>

#define MACRO_TO_STR_(x) #x
#define MACRO_TO_STR(x) MACRO_TO_STR_(x)

using namespace Magnum;

const char* build_info::GetBuildTypeStr() { return MACRO_TO_STR(DZ_SIM_BUILD_TYPE); }
const char* build_info::GetBuildTimeStr() { return MACRO_TO_STR(DZ_SIM_BUILD_TIME); }
const char* build_info::GetVersionStr()   { return MACRO_TO_STR(DZ_SIM_VERSION); }

const char* build_info::thirdparty::GetMagnumVersionStr() {
#ifdef MAGNUM_VERSION_STRING
    return MAGNUM_VERSION_STRING;
#else
    return "version unknown";
#endif
}

const char* build_info::thirdparty::GetMagnumPluginsVersionStr() {
#ifdef MAGNUMPLUGINS_VERSION_STRING
    return MAGNUMPLUGINS_VERSION_STRING;
#else
    return "version unknown";
#endif
}

const char* build_info::thirdparty::GetMagnumIntegrationVersionStr() {
#ifdef MAGNUMINTEGRATION_VERSION_STRING
    return MAGNUMINTEGRATION_VERSION_STRING;
#else
    return "version unknown";
#endif
}

const char* build_info::thirdparty::GetCorradeVersionStr() {
#ifdef CORRADE_VERSION_STRING
    return CORRADE_VERSION_STRING;
#else
    return "version unknown";
#endif
}

const char* build_info::thirdparty::GetImGuiVersionStr() {
    return IMGUI_VERSION;
}
const char* build_info::thirdparty::GetAsioVersionStr()
{
    static std::string asio_ver_str =
        std::to_string(ASIO_VERSION / 100000) + "." +
        std::to_string(ASIO_VERSION / 100 % 1000) + "." +
        std::to_string(ASIO_VERSION % 100); // Make sure string stays in memory
    return asio_ver_str.c_str();
}
const char* build_info::thirdparty::GetCppHttpLibVersionStr() {
    return CPPHTTPLIB_VERSION;
}
const char* build_info::thirdparty::GetJsonVersionStr() {
    return MACRO_TO_STR(NLOHMANN_JSON_VERSION_MAJOR) "." MACRO_TO_STR(NLOHMANN_JSON_VERSION_MINOR) "." MACRO_TO_STR(NLOHMANN_JSON_VERSION_PATCH);
}

const char* build_info::thirdparty::GetFsalVersionStr()
{
    return "(modified from commit 43a10da)";
}

#ifndef DZSIM_WEB_PORT
const char* build_info::thirdparty::GetSdlVersionStr() {
    return MACRO_TO_STR(SDL_MAJOR_VERSION) "." MACRO_TO_STR(SDL_MINOR_VERSION) "." MACRO_TO_STR(SDL_PATCHLEVEL);
}

const char* build_info::thirdparty::GetOpenSSLVersionStr()
{
    return MACRO_TO_STR(DZ_SIM_OPENSSL_VERSION);
}
#endif

void build_info::print() {

    Debug{} << "Build settings:" << GetBuildTypeStr();
    Debug{} << " - DZSimulator" << GetVersionStr()
        << "(build timestamp:" << GetBuildTimeStr() << ")";
    Debug{} << " - Magnum            " << thirdparty::GetMagnumVersionStr();
    Debug{} << " - Magnum Plugins    " << thirdparty::GetMagnumPluginsVersionStr();
    Debug{} << " - Magnum Integration" << thirdparty::GetMagnumIntegrationVersionStr();
    Debug{} << " - Corrade           " << thirdparty::GetCorradeVersionStr();
#ifndef DZSIM_WEB_PORT
    Debug{} << " - SDL" << thirdparty::GetSdlVersionStr();
#endif
    Debug{} << " - Dear ImGui" << thirdparty::GetImGuiVersionStr();
    Debug{} << " - Asio" << thirdparty::GetAsioVersionStr();
#ifndef DZSIM_WEB_PORT
    Debug{} << " - OpenSSL" << thirdparty::GetOpenSSLVersionStr();
#endif
    Debug{} << " - cpp-httplib" << thirdparty::GetCppHttpLibVersionStr();
    Debug{} << " - nlohmann/json" << thirdparty::GetJsonVersionStr();
    Debug{} << " - podgorskiy/fsal" << thirdparty::GetFsalVersionStr();

    
    // All CORRADE_* macros found inside <Corrade/Corrade.h>
    // All MAGNUM_* macros found inside <Magnum/Magnum.h>

#ifdef NDEBUG
    Debug{} << " - NDEBUG defined";
#else
    Debug{} << " - NDEBUG NOT defined";
#endif

#ifdef CORRADE_BUILD_STATIC
    Debug{} << " - CORRADE_BUILD_STATIC defined";
#else
    Debug{} << " - CORRADE_BUILD_STATIC NOT defined";
#endif

#ifdef MAGNUM_BUILD_STATIC
    Debug{} << " - MAGNUM_BUILD_STATIC defined";
#else
    Debug{} << " - MAGNUM_BUILD_STATIC NOT defined";
#endif

#ifdef CORRADE_TARGET_32BIT
    Debug{} << " - CORRADE_TARGET_32BIT";
#endif
#ifdef CORRADE_TARGET_WINDOWS
    Debug{} << " - CORRADE_TARGET_WINDOWS";
#endif
#ifdef CORRADE_TARGET_UNIX
    Debug{} << " - CORRADE_TARGET_UNIX";
#endif
#ifdef CORRADE_TARGET_APPLE
    Debug{} << " - CORRADE_TARGET_APPLE";
#endif
#ifdef CORRADE_TARGET_EMSCRIPTEN
    Debug{} << " - CORRADE_TARGET_EMSCRIPTEN";
#endif
#ifdef CORRADE_TARGET_X86
    Debug{} << " - CORRADE_TARGET_X86";
#endif
#ifdef CORRADE_TARGET_ARM
    Debug{} << " - CORRADE_TARGET_ARM";
#endif
#ifdef CORRADE_TARGET_WASM
    Debug{} << " - CORRADE_TARGET_WASM";
#endif
#ifdef MAGNUM_TARGET_WEBGL
    Debug{} << " - MAGNUM_TARGET_WEBGL";
#endif

#ifdef CORRADE_TARGET_GCC
    Debug{} << " - CORRADE_TARGET_GCC";
#endif
#ifdef CORRADE_TARGET_CLANG
    Debug{} << " - CORRADE_TARGET_CLANG";
#endif
#ifdef CORRADE_TARGET_APPLE_CLANG
    Debug{} << " - CORRADE_TARGET_APPLE_CLANG";
#endif
#ifdef CORRADE_TARGET_CLANG_CL
    Debug{} << " - CORRADE_TARGET_CLANG_CL";
#endif
#ifdef CORRADE_TARGET_MSVC
    Debug{} << " - CORRADE_TARGET_MSVC";
#endif
#ifdef CORRADE_TARGET_MINGW
    Debug{} << " - CORRADE_TARGET_MINGW";
#endif
#ifdef CORRADE_TARGET_LIBCXX
    Debug{} << " - CORRADE_TARGET_LIBCXX";
#endif
#ifdef CORRADE_TARGET_LIBSTDCXX
    Debug{} << " - CORRADE_TARGET_LIBSTDCXX";
#endif
#ifdef CORRADE_TARGET_DINKUMWARE
    Debug{} << " - CORRADE_TARGET_DINKUMWARE";
#endif
}
