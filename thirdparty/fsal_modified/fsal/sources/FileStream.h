#pragma once
#include "bfio.h"
#include "File.h"

namespace fsal
{
	class FileStream : public bfio::Stream<FileStream>
	{
	public:
		FileStream(File f) : file(f)
		{};

		bool Write(const char* src, size_t size)
		{
			return file.Write((const uint8_t*)src, size);
		}
		bool Read(char* dst, size_t size)
		{
			return file.Read((uint8_t*)dst, size);
		}

	private:
		File file;
	};
}
