cmake_minimum_required(VERSION 3.20) 

# Project version MUST have format: X.Y.Z
set(DZSIM_VERSION 0.0.4) # Increase this on each release

# How much memory the web version can use.
# Currently, it needs at least 16.5 MiB with a very small embedded map (wrong?).
set(DZSIM_WEB_PORT_MAX_MEM_USAGE 33554432) # in bytes, must be multiple of 64 KiB

# Set convenience directory variables
set(DZSIM_DIR                    src)
set(DZSIM_CMAKE_MODULES          thirdparty/_cmake_modules)
set(DZSIM_CMAKE_INCLUDE_DIR      dzsim_cmake_inc) # folder in binary dir for CMake generated files

set(DZSIM_ASIO_DIR               thirdparty/asio/asio)
set(DZSIM_CORRADE_DIR            thirdparty/corrade)
set(DZSIM_CPP_HTTPLIB_DIR        thirdparty/cpp-httplib)
set(DZSIM_FSAL_DIR               thirdparty/fsal_modified/fsal)
set(DZSIM_IMGUI_DIR              thirdparty/imgui)
set(DZSIM_JSON_DIR               thirdparty/json)
set(DZSIM_MAGNUM_DIR             thirdparty/magnum)
set(DZSIM_MAGNUM_PLUGINS_DIR     thirdparty/magnum-plugins)
set(DZSIM_MAGNUM_INTEGRATION_DIR thirdparty/magnum-integration)
set(DZSIM_PFD_DIR                thirdparty/portable-file-dialogs)
set(DZSIM_SDL_DIR                thirdparty/SDL)
set(DZSIM_TRACY_DIR              thirdparty/tracy)

# Some of these error messages must occur before the project() command to aid
# the user with a helpful message before other unhelpful error messages pop up.
if(DZSIM_WEB_PORT) # Web build requirements
    # Require Emscripten to be installed and pointed to in CMakeUserPresets.json
    if(NOT EXISTS ${EMSCRIPTEN_PREFIX}) # If directory at path does not exist
        message(FATAL_ERROR "Failed to find Emscripten installation.\n"
            "Did you install it, with the correct version and point to its "
            "location in CMakeUserPresets.json through the 'EMSCRIPTEN_PREFIX' "
            "variable of the Emscripten preset?\n"
            "See BUILDING.md for more details about building for the web.")
    else()
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${EMSCRIPTEN_PREFIX}/system")
    endif()

    # Require 'corrade-rc' executable to exist (from a previous native build)
    if(NOT EXISTS ${CORRADE_RC_EXECUTABLE}) # If file at path does not exist
        message(FATAL_ERROR "Failed to find native `corrade-rc` executable.\n"
            "This is needed when compiling for the web. Simply build once using "
            "the Windows Release configuration (win-x64-release), then try "
            "building the web version again.")
    endif()
else() # Native build requirements
    # Require OpenSSL to be installed and the toolchain file pointing to its
    # installation location to exist.
    if(NOT EXISTS ${CMAKE_TOOLCHAIN_FILE}) # If file at path does not exist
        message(FATAL_ERROR "Failed to find conan_toolchain.cmake file that "
            "points to OpenSSL's installation. Did you forget to install "
            "OpenSSL for this specific build type, using Conan? See "
            "BUILDING.md for more details about building DZSimulator.")
    endif()
endif()

# Prepare the user for long configure times on Emscripten
message("\nConfiguring the project... (This can take a while, ESPECIALLY when configuring using the Web/Emscripten preset)")

# Set the project name and version
project(DZSimulatorProject VERSION ${DZSIM_VERSION})

set(CMAKE_CXX_STANDARD 20) # Specify the C++ standard
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/${DZSIM_CMAKE_MODULES}/" ${CMAKE_MODULE_PATH})

# Enable useful compiler warnings (breaks compile if disabled?!)
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

if(DZSIM_WEB_PORT)
    # Emscripten build: Set additional _compiler_ options. Additional _linker_
    # options are set further down. (Note: Some options are already set inside
    # "thirdparty/toolchains/generic/Emscripten-wasm.cmake")

    # Some options need to be passed to both compiler and linker, some just to
    # the linker: https://emscripten.org/docs/tools_reference/emcc.html

    # Enable Emscripten's multithreading support by setting "-pthread" during
    # compilation of _all_ .c/.cpp files (set here) and during final linking
    # (set further down). More info: https://emscripten.org/docs/porting/pthreads.html
    # To build everything with the same set of Emscripten flags (required),
    # add_compile_options() must be called before any add_subdirectory() call.
    add_compile_options("-pthread")

    # Enable exception catching (this worsens executable size and speed!)
    add_compile_options("-sDISABLE_EXCEPTION_CATCHING=0")
endif()


# Set features/build-time options before doing add_subdirectory() calls
set(MAGNUM_WITH_DEBUGTOOLS       ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_IMGUI            ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_STBIMAGEIMPORTER ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_STBTRUETYPEFONT  ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_TEXT             ON CACHE BOOL "" FORCE)

if(DZSIM_WEB_PORT)
    set(MAGNUM_WITH_EMSCRIPTENAPPLICATION ON CACHE BOOL "" FORCE)
else()
    set(MAGNUM_WITH_SDL2APPLICATION       ON CACHE BOOL "" FORCE)

    # This is the most minimal set of features which still make Sdl2Application
    # work. If you need something from these, remove the setting. The SDL_AUDIO
    # and SDL_EVENT options should not be needed either as Magnum doesn't use
    # them, but if they're disabled they causes compiler or linker errors.
    # Either SDL_DLOPEN or SDL_LOADSO needs to be enabled depending on the
    # system to allow linking dependencies at runtime, so it's better to just
    # leave them both on. The SDL_TIMERS option is important for rendering
    # performance.
    set(SDL_ATOMIC     OFF CACHE BOOL "" FORCE)
    set(SDL_CPUINFO    OFF CACHE BOOL "" FORCE)
    set(SDL_FILE       OFF CACHE BOOL "" FORCE)
    set(SDL_FILESYSTEM OFF CACHE BOOL "" FORCE)
    set(SDL_HAPTIC     OFF CACHE BOOL "" FORCE)
    set(SDL_LOCALE     OFF CACHE BOOL "" FORCE)
    set(SDL_POWER      OFF CACHE BOOL "" FORCE)
    set(SDL_RENDER     OFF CACHE BOOL "" FORCE)
    set(SDL_SENSOR     OFF CACHE BOOL "" FORCE)
    set(SDL_THREADS    OFF CACHE BOOL "" FORCE)
    # This assumes you want to have SDL as a static library. If not, set
    # SDL_STATIC to OFF instead.
    set(SDL_SHARED     OFF CACHE BOOL "" FORCE)
    add_subdirectory(${DZSIM_SDL_DIR} EXCLUDE_FROM_ALL)
endif()

# Set Dear ImGui path
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/${DZSIM_IMGUI_DIR})

# Set options before add_subdirectory()
option(TRACY_ENABLE "Enable profiling with Tracy" OFF) # Disabled by default. Tracy has more options.

# Add subprojects
add_subdirectory(${DZSIM_CORRADE_DIR}            EXCLUDE_FROM_ALL)
add_subdirectory(${DZSIM_MAGNUM_DIR}             EXCLUDE_FROM_ALL)
add_subdirectory(${DZSIM_MAGNUM_PLUGINS_DIR}     EXCLUDE_FROM_ALL)
add_subdirectory(${DZSIM_MAGNUM_INTEGRATION_DIR} EXCLUDE_FROM_ALL)
add_subdirectory(${DZSIM_FSAL_DIR}               EXCLUDE_FROM_ALL)
add_subdirectory(${DZSIM_TRACY_DIR}              EXCLUDE_FROM_ALL)

# Add executable target (must be done after all add_subdirectory() calls,
# otherwise Debug builds might fail to find DLLs after CMake's first configure)
if(DZSIM_DISABLE_CONSOLE_WINDOW)
    add_executable(DZSimulator WIN32) # Without console window
else()
    add_executable(DZSimulator) # With console window
endif()

# Make the executable a default target to build & run in Visual Studio (does this work?)
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DZSimulator)

if(DZSIM_WEB_PORT)
    target_compile_definitions(DZSimulator PUBLIC DZSIM_WEB_PORT)
endif()

if(DZSIM_WEB_PORT)
    # Emscripten build: Set additional _linker_ options. Additional _compiler_
    # options are set further up. (Note: Some options are already set inside
    # "thirdparty/toolchains/generic/Emscripten-wasm.cmake")

    # Enable Emscripten's multithreading support
    target_link_options(DZSimulator PUBLIC "-pthread")
    # Prematurely create pool of threads that can then immediately be utilized
    target_link_options(DZSimulator PUBLIC "-sPTHREAD_POOL_SIZE=2")

    # Preallocate appropriate amount of memory required by the web app (It
    # crashes when it runs out of memory and cannot grow it).
    target_link_options(DZSimulator PUBLIC "-sINITIAL_MEMORY=${DZSIM_WEB_PORT_MAX_MEM_USAGE}")
    # Avoid ALLOW_MEMORY_GROWTH if possible to avoid performance penalties
    #target_link_options(DZSimulator PUBLIC "-sALLOW_MEMORY_GROWTH=1")

    # Enable exception catching (this worsens executable size and speed!)
    target_link_options(DZSimulator PUBLIC "-sDISABLE_EXCEPTION_CATCHING=0")

    # Enable runtime assertions (can incur overheads). Implicitly enables stack
    # overflow checks.
    target_link_options(DZSimulator PUBLIC "-sASSERTIONS=1")

    # Set stack size. Actual required stack size is not tested yet.
    # Note: TOTAL_STACK was renamed to STACK_SIZE in Emscripten 3.1.25
    target_link_options(DZSimulator PUBLIC "-sTOTAL_STACK=5242880") # 5 MiB
    target_link_options(DZSimulator PUBLIC "-sDEFAULT_PTHREAD_STACK_SIZE=2097152") # 2 MiB
endif()


find_package(Corrade REQUIRED
    Utility
    Main)
find_package(Magnum REQUIRED
    DebugTools
    GL
    Primitives
    Shaders
    Text
    Trade)
find_package(MagnumIntegration REQUIRED
    ImGui)
find_package(MagnumPlugins REQUIRED
    StbImageImporter
    StbTrueTypeFont)

if(DZSIM_WEB_PORT)
    find_package(Magnum REQUIRED EmscriptenApplication)
else()
    find_package(Magnum REQUIRED Sdl2Application)

    # Find OpenSSL
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL REQUIRED)
endif()


# Determine build time (must happen before configure_file() !)
string(TIMESTAMP DZSIM_BUILD_TIME "%Y-%m-%d %H:%M UTC" UTC)

# Configure must happen after (potentially) finding OpenSSL
configure_file(
    ${DZSIM_DIR}/cmake_build_settings.h.in
    ${DZSIM_CMAKE_INCLUDE_DIR}/cmake_build_settings.h
)


# Take resource files and let Corrade put them into a cpp file that gets compiled in
corrade_add_resource(DZSIM_RESOURCES res/resources.conf)
target_sources(DZSimulator PRIVATE ${DZSIM_RESOURCES})


target_link_libraries(DZSimulator PRIVATE
    Corrade::Main
    Corrade::Utility
    fsal
    Magnum::DebugTools
    Magnum::GL
    Magnum::Magnum
    Magnum::Primitives
    Magnum::Shaders
    Magnum::Text
    Magnum::Trade
    MagnumIntegration::ImGui
    MagnumPlugins::StbImageImporter
    MagnumPlugins::StbTrueTypeFont
    TracyClient
)

if(DZSIM_WEB_PORT)
    target_link_libraries(DZSimulator PRIVATE Magnum::EmscriptenApplication)
else()
    target_link_libraries(DZSimulator PRIVATE
        Magnum::Sdl2Application
        openssl::openssl
    )

    # The cpp-httplib library requires CPPHTTPLIB_OPENSSL_SUPPORT to be defined
    # before all occurrences of its "httplib.h" being included!
    # OpenSSL is required to access HTTPS-only APIs.
    target_compile_definitions(DZSimulator PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)

    target_include_directories(DZSimulator PUBLIC
        "${PROJECT_SOURCE_DIR}/${DZSIM_SDL_DIR}/include" # Add SDL headers to access version macro
    )
endif()

target_include_directories(DZSimulator PUBLIC
    "${PROJECT_BINARY_DIR}/${DZSIM_CMAKE_INCLUDE_DIR}" # Add generated CMake files(configure_file) from the binary dir

    "${PROJECT_SOURCE_DIR}/${DZSIM_DIR}" # Add our project dir
    "${PROJECT_SOURCE_DIR}/${DZSIM_ASIO_DIR}/include" # Add headers from header-only lib asio
    "${PROJECT_SOURCE_DIR}/${DZSIM_CPP_HTTPLIB_DIR}/include" # Add headers from header-only lib cpp-httplib
    "${PROJECT_SOURCE_DIR}/${DZSIM_FSAL_DIR}/sources" # Add sources from fsal lib
    "${PROJECT_SOURCE_DIR}/${DZSIM_JSON_DIR}/include" # Add headers from header-only lib json
    "${PROJECT_SOURCE_DIR}/${DZSIM_PFD_DIR}/include" # Add single-header library to include path
    "${PROJECT_SOURCE_DIR}/${DZSIM_TRACY_DIR}/public/tracy"
)

# For Emscripten copy the boilerplate next to the executable so it can be run
# directly from the build dir; provide an install target as well
if(DZSIM_WEB_PORT)
    add_custom_command(TARGET DZSimulator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
            ${MAGNUM_WEBAPPLICATION_CSS}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/web/DZSimulator.html
            $<TARGET_FILE_DIR:DZSimulator>)

    # Maybe this is useful later for webpage deployment?
    #install(FILES src/web/DZSimulator.html DESTINATION ${MAGNUM_DEPLOY_PREFIX} RENAME index.html)

    # CHECK: Replace MAGNUM_DEPLOY_PREFIX with CMAKE_INSTALL_PREFIX ?
    install(TARGETS DZSimulator DESTINATION ${MAGNUM_DEPLOY_PREFIX})
    install(FILES
        src/web/DZSimulator.html
        ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
        ${MAGNUM_WEBAPPLICATION_CSS}
        DESTINATION ${MAGNUM_DEPLOY_PREFIX})
    install(FILES
        $<TARGET_FILE_DIR:DZSimulator>/DZSimulator.js.mem
        $<TARGET_FILE_DIR:DZSimulator>/DZSimulator.wasm
        $<TARGET_FILE_DIR:DZSimulator>/DZSimulator.worker.js
        DESTINATION ${MAGNUM_DEPLOY_PREFIX} OPTIONAL)
endif()

# If you add new source files to DZSimulator, add them to this list
target_sources(DZSimulator PRIVATE
    # Conditional source files (generator expressions must be absolute paths here):
    "$<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/res/platforms/windows/executable_icon.rc>"
    "$<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/res/platforms/windows/DZSimulator.manifest>"

    # Regular source files:
    "src/main.cpp"

    "src/build_info.cpp"
    "src/GitHubChecker.cpp"
    "src/GlobalVars.cpp"
    "src/InputHandler.cpp"
    "src/SavedUserDataHandler.cpp"
    "src/utils_3d.cpp"
    "src/WorldCreator.cpp"

    "src/coll/Benchmark.cpp"
    "src/coll/BVH.cpp"
    "src/coll/CollidableWorld.cpp"
    "src/coll/CollidableWorld-brush.cpp"
    "src/coll/CollidableWorld-displacement.cpp"
    "src/coll/CollidableWorld-funcbrush.cpp"
    "src/coll/CollidableWorld-xprop.cpp"
    "src/coll/Debugger.cpp"
    "src/coll/SweptTrace.cpp"

    "src/csgo_integration/Gsi.cpp"
    "src/csgo_integration/Handler.cpp"
    "src/csgo_integration/RemoteConsole.cpp"

    "src/csgo_parsing/AssetFileReader.cpp"
    "src/csgo_parsing/AssetFinder.cpp"
    "src/csgo_parsing/BrushSeparation.cpp"
    "src/csgo_parsing/BspMap.cpp"
    "src/csgo_parsing/BspMapParsing.cpp"
    "src/csgo_parsing/PhyModelParsing.cpp"
    "src/csgo_parsing/utils.cpp"

    "src/gui/Gui.cpp"
    "src/gui/MenuWindow.cpp"
    "src/gui/Popup.cpp"

    "src/ren/_unused_VertBufUpdateDemo.cpp"
    "src/ren/BigTextRenderer.cpp"
    "src/ren/GlidabilityShader3D.cpp"
    "src/ren/RenderableWorld.cpp"
    "src/ren/WideLineRenderer.cpp"
    "src/ren/WorldRenderer.cpp"

    "src/sim/CsgoGame.cpp"
    "src/sim/CsgoMovement.cpp"
    "src/sim/WorldState.cpp"
)
# Add new .cpp files for DZSimulator to the list above to get them compiled in!
