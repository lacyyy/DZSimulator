// DZSIM_MOD:
//
//   Added MemRefFileReadOnly, a modified copy of MemRefFile in order to read
//   in-memory files from const pointers.
//   The original MemRefFile is only usable with non-const pointers.

#pragma once
#include "fsal_common.h"
#include "FileInterface.h"

#include <cstdio>
#include <memory>

namespace fsal
{
	class MemRefFileReadOnly : public FileInterface
	{
	public:
        MemRefFileReadOnly();

        MemRefFileReadOnly(const uint8_t* data, size_t size);

        MemRefFileReadOnly(std::shared_ptr<const uint8_t> data, size_t size);

		~MemRefFileReadOnly() override;

		bool ok() const override;

		path GetPath() const override;

        // Does nothing and returns failure
		Status Open(path filepath, Mode mode) override;

		Status ReadData(uint8_t* dst, size_t size, size_t* bytesRead) override;

        // Does nothing and returns failure
		Status WriteData(const uint8_t* src, size_t size)  override;

		Status SetPosition(size_t position) const  override;

		size_t GetPosition() const  override;

		size_t GetSize() const  override;

        // Does nothing and returns success
		Status FlushBuffer() const override;

        // DZSIM_MOD: Commented out this function for C++20 compatibility
		// uint64_t GetLastWriteTime() const override { return 0; }

        // DZSIM_MOD: These return nullptr because we need to implement these
        //            virtual functions but we only have access to the const
        //            pointer. Both return nullptr to avoid ambigous usage.
        const uint8_t* GetDataPointer() const  override { return nullptr; };
        uint8_t* GetDataPointer()  override { return nullptr; };

	private:
		const uint8_t* m_data;
		std::shared_ptr<const uint8_t> m_sharedData;

		size_t m_size;
		mutable size_t m_offset;
	};
}
