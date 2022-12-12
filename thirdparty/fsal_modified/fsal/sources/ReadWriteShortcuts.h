#pragma once
#include "FileStream.h"

#include <cstdint>
#include <cstring>

namespace fsal
{
	template<typename T>
	inline void operator << (File& f, const T& d)
	{
		::fsal::FileStream(f) << d;
	};

	template<typename T>
	inline void operator >> (File& f, T& d)
	{
		::fsal::FileStream(f) >> d;
	};

	/*
	template<>
	inline void operator << <std::string>(File& f, const std::string& s)
	{
		f.Write(reinterpret_cast<const uint8_t*>(s.c_str()), s.size());
	};

	template<>
	inline void operator >> <std::string> (File& f, std::string& s)
	{
		size_t size = f.GetSize();
		s.resize(size);
		// http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#530
		// mystring.c_str() is equivalent to mystring.data() is equivalent to &mystring[0],
		// mystring[mystring.size()] is guaranteed to be '\0'
		f.Read(reinterpret_cast<uint8_t*>(&s[0]), size);
	};
	*/
}
