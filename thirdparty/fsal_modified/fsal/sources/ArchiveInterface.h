#pragma once
#include "fsal_common.h"
#include "File.h"
#include <vector>
#include <functional>

namespace fsal
{
	class ArchiveReaderInterface
	{
	public:
        // DZSIM_MOD: Added virtual destructor to ensure derived classes and
        //            their members get destroyed!
        virtual ~ArchiveReaderInterface() {}

		virtual File OpenFile(const fs::path& filepath) = 0;

		virtual void* OpenFile(const fs::path& filepath, std::function<void*(size_t size)> alloc_func) = 0;

		virtual bool Exists(const fs::path& filepath, PathType type = kFile | kDirectory) = 0;

		virtual std::vector<std::string> ListDirectory(const fs::path& path) = 0;
	};

	typedef std::shared_ptr<ArchiveReaderInterface> ArchiveReaderInterfacePtr;

	class ArchiveWriterInterface
	{
	public:
		virtual Status AddFile(const fs::path& path, File file, int compression) = 0;

		virtual Status CreateDirectory(const fs::path& path) = 0;
	};

	typedef std::shared_ptr<ArchiveWriterInterface> ArchiveWriterInterfacePtr;
}
