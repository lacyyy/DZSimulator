#pragma once
#include "fsal_common.h"
#include "FileInterface.h"

#include <cstdio>
#include <memory>

namespace fsal
{
	class MemRefFile : public FileInterface
	{
	public:
		MemRefFile();

		MemRefFile(uint8_t* data, size_t size, bool copy);

		MemRefFile(std::shared_ptr<uint8_t> data, size_t size);

		~MemRefFile() override;

		bool ok() const override;

		path GetPath() const override;

		Status Open(path filepath, Mode mode) override;

		Status ReadData(uint8_t* dst, size_t size, size_t* bytesRead) override;

		Status WriteData(const uint8_t* src, size_t size)  override;

		Status SetPosition(size_t position) const  override;

		size_t GetPosition() const  override;

		size_t GetSize() const  override;

		Status FlushBuffer() const override;

        // DZSIM_MOD: Commented out this function for C++20 compatibility
		// uint64_t GetLastWriteTime() const override { return 0; }

		const uint8_t* GetDataPointer() const  override;

		uint8_t* GetDataPointer()  override;

		bool Resize(size_t newSize);

	private:
		uint8_t* m_data;
		std::shared_ptr<uint8_t> m_sharedData;

		size_t m_size;
		mutable size_t m_offset;

		bool m_hasOwnership;

		size_t m_reserved;
	};
}
