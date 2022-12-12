#pragma once
#include <stdint.h>


// DZSIM_MOD: Commented entire preprocessor conditionals block and just added
//            the following filesystem include and namespace/type assignments.
//            The previous conditionals caused linker errors in C++20 because
//            some .cpp files used the experimental filesystem include while
//            other .cpp files used the non-experimental ones. We stay in C++20 now.
#include <filesystem>
namespace fsal
{
    namespace fs = std::filesystem;
    typedef std::filesystem::path path;
}

/*
#ifndef USING_BOOST_FOR_PATHS
#define USING_CPP11_EXPERIMENTAL_FOR_PATHS 1
#endif

#ifdef USING_BOOST_FOR_PATHS

#include <boost/filesystem.hpp>

namespace fsal
{
    namespace fs = boost::filesystem;
    typedef boost::filesystem::path path;
}

#elif USING_CPP11_EXPERIMENTAL_FOR_PATHS

#if (defined(__cpp_lib_experimental_filesystem) || (__cplusplus == 201402L ||  _MSC_VER >= 1900)) && !defined(__cpp_lib_filesystem)
#ifdef _MSC_VER
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif
#include <experimental/filesystem>

namespace fsal
{
    namespace fs =  std::experimental::filesystem;
    typedef std::experimental::filesystem::path path;
}

#elif (__cplusplus >= 201703L) || defined(__cpp_lib_filesystem)
#include <filesystem>

namespace fsal
{
    namespace fs =  std::filesystem;
    typedef std::filesystem::path path;
}

#endif

#endif
*/

namespace fsal
{
	enum Mode : unsigned char
	{
		kRead = 0x0,
		kWrite = 0x1,
		kAppend = 0x2,
		kReadUpdate = 0x3,
		kWriteUpdate = 0x4,
		kAppendUpdate = 0x5,
	};

	enum PathType : unsigned char
	{
		kFile      = 0x1,
		kDirectory = 0x2,
	};

	enum LinkType : unsigned char
	{
		kSymlink = 0x1,
		kNotSymlink = 0x2
	};

	inline PathType operator | (PathType a, PathType b)
	{
		return (PathType)((unsigned char)a | (unsigned char)b);
	}

	inline LinkType operator | (LinkType a, LinkType b)
	{
		return (LinkType)((unsigned char)a | (unsigned char)b);
	}

	template<typename T>
	inline void null_deleter(T*) {}
}
