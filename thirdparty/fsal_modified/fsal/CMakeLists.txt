# DZSIM_MOD: Increased minimum version from 3.1 to 3.5
cmake_minimum_required (VERSION 3.5 FATAL_ERROR)
#project (fsal)

# DZSIM_MOD: Commented out these 2 options
# option(FSAL_EXAMPLES "Build fsal examples" OFF)
# option(FSAL_TESTS "Build fsal tests" OFF)

# DZSIM_MOD: Changed CMAKE_CXX_STANDARD value from 14 to 20
set (CMAKE_CXX_STANDARD 20)
#set (CMAKE_CXX_STANDARD 14)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message("Clang detected. Asking for cxx 17. Reason: #include <experimental/filesystem> seems to not work on cxx 14 on Clang")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)#"$ENV{MACOSX_DEPLOYMENT_TARGET}")
    message("CMAKE_OSX_DEPLOYMENT_TARGET is '${CMAKE_OSX_DEPLOYMENT_TARGET}' ")
    set (CMAKE_CXX_STANDARD 17)
endif()

# DZSIM_MOD: Commented out this if statement
#[[
if (FSAL_EXAMPLES OR FSAL_TESTS)
	configure_file(thirdparty/CMakeLists.txt.in thirdparty/CMakeLists.txt)
	execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/thirdparty")
	execute_process(COMMAND "${CMAKE_COMMAND}" --build .
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/thirdparty" )

    include_directories(thirdparty/src/doctest/doctest)
	include_directories(thirdparty/src/zlib)
	include_directories(thirdparty/src/lz4/lib)

    #####################################################################
    # lz4
    #####################################################################
    set(LZ4_DIR thirdparty/src/lz4/lib/)
    set(SOURCES_LZ4 ${LZ4_DIR}lz4.c ${LZ4_DIR}lz4hc.c ${LZ4_DIR}lz4.h ${LZ4_DIR}lz4hc.h ${LZ4_DIR}xxhash.c ${LZ4_DIR}xxhash.h)
    add_library(lz4 ${SOURCES_LZ4})
    #####################################################################


    #####################################################################
    # zlib
    #####################################################################
    file(GLOB ZLIB_SOURCES thirdparty/src/zlib/*.c thirdparty/src/zlib/*.h)
    add_library(zlib_static STATIC ${ZLIB_SOURCES})
    #####################################################################
endif()
]]

include_directories(sources/)

file(GLOB_RECURSE SOURCES sources/*.cpp *.h)

add_library(fsal STATIC ${SOURCES})


if(MSVC)
else()
    target_compile_options(fsal PRIVATE -lstdc++fs -Wall -Wno-switch)
endif()

# DZSIM_MOD: Commented out this if statement
#[[
if (FSAL_TESTS)
    add_executable(tests tests/main.cpp)
    target_link_libraries(tests PRIVATE fsal stdc++fs zlib_static lz4)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(tests PRIVATE -lc++experimental -lc++fs)
    endif()

    if(MSVC)
    else()
        target_compile_options(fsal PRIVATE -lstdc++fs -Wall -Wno-switch)
        target_compile_options(tests PRIVATE -lstdc++fs -Wall -Wno-switch)
    endif()
endif()
]]
