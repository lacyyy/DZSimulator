#pragma once
#include "fsal_common.h"
#include "Status.h"

namespace fsal
{
	class FileInterface
	{
	public:
		virtual ~FileInterface()
		{};

		virtual bool ok() const = 0;

		virtual path GetPath() const = 0;

		virtual Status Open(path filepath, Mode mode) = 0;

		virtual Status ReadData(uint8_t* dst, size_t size, size_t* bytesRead) = 0;

		virtual Status WriteData(const uint8_t* src, size_t size) = 0;
				
		virtual Status SetPosition(size_t position) const = 0;

		virtual size_t GetPosition() const = 0;

		virtual size_t GetSize() const = 0;

		virtual Status FlushBuffer() const = 0;

        // DZSIM_MOD: Commented out this function for C++20 compatibility
		// virtual uint64_t GetLastWriteTime() const = 0;

		virtual std::mutex* GetMutex() const { return nullptr; };
		
		virtual const uint8_t* GetDataPointer() const = 0;

		virtual uint8_t* GetDataPointer() = 0;
	};
}
