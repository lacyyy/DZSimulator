#include "fsal.h"
#include "SubFile.h"

#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <cstdio>
#include <assert.h>

using namespace fsal;

SubFile::SubFile(std::shared_ptr<FileInterface> file, size_t size, size_t offset): m_file(std::move(file)), m_size(size), m_offset(offset), m_pointer(0)
{
	assert(m_file->GetMutex() != nullptr);
}

SubFile::~SubFile()
{}

bool SubFile::ok() const
{
	return m_file->ok();
}

path SubFile::GetPath() const
{
	return m_file->GetPath();
}

Status SubFile::ReadData(uint8_t* dst, size_t size, size_t* bytesRead)
{
	if (m_pointer >= m_size)
	{
		return Status::kEOF;
	}
	File::LockGuard guard(m_file.get());
	m_file->SetPosition(m_pointer + m_offset);
	size_t _bytesRead = 0;
	size_t* read = bytesRead == nullptr ? &_bytesRead : bytesRead;
	ssize_t _size = std::min(m_pointer + size, m_size) - m_pointer;
	auto tmp = m_file->ReadData(dst, _size, read);
	m_pointer += *read;
	tmp.state |= (m_pointer + size > m_size) ? Status::kEOF: Status::kOk;
	return tmp;
}

Status SubFile::WriteData(const uint8_t* src, size_t size)
{
	if (m_pointer >= m_size)
	{
		return Status::kEOF;
	}
	File::LockGuard guard(m_file.get());
	m_file->SetPosition(m_pointer + m_offset);
	ssize_t _size = std::min(m_pointer + size, m_size) - m_pointer;
	auto tmp = m_file->WriteData(src, _size);
	m_pointer += _size;
	tmp.state |= (m_pointer + size > m_size) ? (Status::kFailed | Status::kEOF): Status::kOk;
	return tmp;
}

Status SubFile::SetPosition(size_t position) const
{
	m_pointer = std::min(position, m_size);
	return true;
}

size_t SubFile::GetPosition() const
{
	return m_pointer;
}

size_t SubFile::GetSize() const
{
	return m_size;
}

Status SubFile::FlushBuffer() const
{
	return m_file->FlushBuffer();
}
