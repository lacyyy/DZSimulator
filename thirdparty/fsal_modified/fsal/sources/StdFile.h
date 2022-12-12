#pragma once
#include "fsal_common.h"
#include "FileInterface.h"

#include <cstdio>

namespace fsal
{
	class StdFile : public FileInterface
	{
	public:
		StdFile();

		~StdFile() override;

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
		// uint64_t GetLastWriteTime() const override;

		const uint8_t* GetDataPointer() const  override;

		uint8_t* GetDataPointer()  override;

		void AssignFile(FILE* file);
	private:
		FILE* m_file;
		path m_path;
	};
}
