#pragma once
#include "ArchiveInterface.h"
#include <memory>
#include <functional>

namespace fsal
{
	class Archive
	{
	public:
		Archive();

		Archive(ArchiveReaderInterfacePtr reader);

		bool Valid() const;

		File OpenFile(const fs::path& filepath);

		void* OpenFile(const fs::path& filepath, std::function<void*(size_t size)> alloc_func);

		bool Exists(const fs::path& filepath, PathType type = kFile | kDirectory);

		Status AddFile(const fs::path& path, File file, int compression);

		Status CreateDirectory(const fs::path& path);

		std::vector<std::string> ListDirectory(const fs::path& path);
	private:
		ArchiveReaderInterfacePtr m_impl;
	};
}
