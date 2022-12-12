#pragma once
#include "fsal_common.h"
#include <string>

namespace fsal
{
	fs::path NormalizePath(const fs::path& src);

	std::string NormalizePath(const char* src);

	std::string NormalizePath(const std::string& src);

	void NormalizePath(const std::string src, std::string& dst, int& filenamePos, int& depth);

	void NormalizePath(const fs::path src, fs::path& dst, int& filenamePos, int& depth);

	// No memory allocations version. The 'char* dst' assumed to be already allocated and as large as 'const char* src'.
	void NormalizePath(const char* src, char* dst, size_t len, char*& filename, int& depth);
}
