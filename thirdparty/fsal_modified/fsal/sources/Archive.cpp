#include "Archive.h"

using namespace fsal;


Archive::Archive()
{}

Archive::Archive(ArchiveReaderInterfacePtr reader): m_impl(reader)
{}

bool Archive::Valid() const
{
	return m_impl != nullptr;
}

File Archive::OpenFile(const fs::path& filepath)
{
	return m_impl->OpenFile(filepath);
}

void* Archive::OpenFile(const fs::path& filepath, std::function<void*(size_t size)> alloc_func)
{
	return m_impl->OpenFile(filepath, std::move(alloc_func));
}

bool Archive::Exists(const fs::path& filepath, PathType type)
{
	return m_impl->Exists(filepath, type);
}

Status Archive::AddFile(const fs::path& path, File file, int compression)
{
	return false;// m_impl->AddFile(path, file, compression);
}

Status Archive::CreateDirectory(const fs::path& path)
{
	return false;// m_impl->CreateDirectory(path);
}

std::vector<std::string> Archive::ListDirectory(const fs::path& path)
{
	return m_impl->ListDirectory(path);
}
