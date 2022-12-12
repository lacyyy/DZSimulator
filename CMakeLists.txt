cmake_minimum_required(VERSION 3.20) 

set(DZSIM_VERSION 0.0.1) # Change this on each release
string(TIMESTAMP DZSIM_BUILD_TIME "%Y-%m-%d %H:%M UTC" UTC)

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


# Set the project name and version
project(DZSimulatorProject VERSION ${DZSIM_VERSION})
configure_file(${DZSIM_DIR}/cmake_build_settings.h.in ${DZSIM_CMAKE_INCLUDE_DIR}/cmake_build_settings.h)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/${DZSIM_CMAKE_MODULES}/" ${CMAKE_MODULE_PATH})

# Set Dear ImGui path
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/${DZSIM_IMGUI_DIR})

# Enable features
set(MAGNUM_WITH_IMGUI            ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_SDL2APPLICATION  ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_STBIMAGEIMPORTER ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_STBTRUETYPEFONT  ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_TEXT             ON CACHE BOOL "" FORCE)

# This is the most minimal set of features which still make Sdl2Application
# work. If you need something from these, remove the setting. The SDL_AUDIO and
# SDL_EVENT options should not be needed either as Magnum doesn't use them, but
# if they're disabled they causes compiler or linker errors. Either SDL_DLOPEN
# or SDL_LOADSO needs to be enabled depending on the system to allow linking
# dependencies at runtime, so it's better to just leave them both on. The
# SDL_TIMERS option is important for rendering performance.
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
# This assumes you want to have SDL as a static library. If not, set SDL_STATIC
# to OFF instead.
set(SDL_SHARED     OFF CACHE BOOL "" FORCE)
add_subdirectory(${DZSIM_SDL_DIR}                EXCLUDE_FROM_ALL)

# Add subprojects
add_subdirectory(${DZSIM_CORRADE_DIR}            EXCLUDE_FROM_ALL)
add_subdirectory(${DZSIM_MAGNUM_DIR}             EXCLUDE_FROM_ALL)
add_subdirectory(${DZSIM_MAGNUM_PLUGINS_DIR}     EXCLUDE_FROM_ALL)
add_subdirectory(${DZSIM_MAGNUM_INTEGRATION_DIR} EXCLUDE_FROM_ALL)
add_subdirectory(${DZSIM_FSAL_DIR}               EXCLUDE_FROM_ALL)

# Enable useful compiler warnings (breaks compile if disabled?!)
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

find_package(Corrade REQUIRED
    Utility
    Main)
find_package(Magnum REQUIRED
    GL
    Primitives
    Sdl2Application
    Shaders
    Text
    Trade)
find_package(MagnumIntegration REQUIRED
    ImGui)
find_package(MagnumPlugins REQUIRED
    StbImageImporter
    StbTrueTypeFont)

# Take resource files and let Corrade put them into a cpp file that gets compiled in
corrade_add_resource(DZSIM_RESOURCES res/resources.conf)

# Add the executable
if(DZSIM_DISABLE_CONSOLE_WINDOW)
	# Without console window
    add_executable(DZSimulator WIN32 ${DZSIM_RESOURCES})
else()
	# With console window
    add_executable(DZSimulator ${DZSIM_RESOURCES})
endif()

target_link_libraries(DZSimulator PRIVATE
    Corrade::Main
    Corrade::Utility
    fsal
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::Primitives
    Magnum::Shaders
    Magnum::Text
    Magnum::Trade
    MagnumIntegration::ImGui
    MagnumPlugins::StbImageImporter
    MagnumPlugins::StbTrueTypeFont)

target_include_directories(DZSimulator PUBLIC
    "${PROJECT_BINARY_DIR}/${DZSIM_CMAKE_INCLUDE_DIR}" # Add generated CMake files(configure_file) from the binary dir

    "${PROJECT_SOURCE_DIR}/${DZSIM_DIR}" # Add our project dir
    "${PROJECT_SOURCE_DIR}/${DZSIM_ASIO_DIR}/include" # Add headers from header-only lib asio
    "${PROJECT_SOURCE_DIR}/${DZSIM_CPP_HTTPLIB_DIR}/include" # Add headers from header-only lib cpp-httplib
    "${PROJECT_SOURCE_DIR}/${DZSIM_FSAL_DIR}/sources" # Add sources from fsal lib
    "${PROJECT_SOURCE_DIR}/${DZSIM_JSON_DIR}/include" # Add headers from header-only lib json
    "${PROJECT_SOURCE_DIR}/${DZSIM_PFD_DIR}/include" # Add single-header library to include path
    "${PROJECT_SOURCE_DIR}/${DZSIM_SDL_DIR}/include" # Add SDL headers to access version macro
)

# Collect conditional sources
if(CORRADE_TARGET_WINDOWS)
    list(APPEND DZSIM_EXTRA_SOURCES "res/platforms/windows/executable_icon.rc")
    list(APPEND DZSIM_EXTRA_SOURCES "res/platforms/windows/DZSimulator.manifest")
endif()

# If you add new cpp-files to the project, add their path to this list!
target_sources(DZSimulator PRIVATE ${DZSIM_EXTRA_SOURCES} "src/main.cpp" "src/csgo_parsing/BrushSeparation.cpp" "src/GlobalVars.cpp" "src/InputHandler.cpp" "src/rendering/_unused_VertBufUpdateDemo.cpp" "src/rendering/WorldRenderer.cpp"  "src/csgo_parsing/BspMap.cpp"  "src/csgo_parsing/BspMapParsing.cpp" "src/sim/Server.cpp" "src/sim/WorldState.cpp"    "src/build_info.cpp" "src/gui/Gui.cpp" "src/rendering/BigTextRenderer.cpp" "src/utils_3d.cpp"   "src/csgo_integration/Gsi.cpp" "src/rendering/GlidabilityShader3D.cpp" "src/csgo_integration/RemoteConsole.cpp" "src/csgo_integration/Handler.cpp" "src/csgo_parsing/AssetFinder.cpp" "src/csgo_parsing/PhyModelParsing.cpp" "src/csgo_parsing/AssetFileReader.cpp"  "src/csgo_parsing/utils.cpp" "src/rendering/WorldCreation.cpp" "src/gui/MenuWindow.cpp" "src/gui/Popup.cpp")

# Make the executable a default target to build & run in Visual Studio (doesn't work?)
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DZSimulator)
