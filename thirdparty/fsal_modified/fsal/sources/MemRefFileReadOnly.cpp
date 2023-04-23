// DZSIM_MOD:
//
//   Added MemRefFileReadOnly, a modified copy of MemRefFile in order to read
//   in-memory files from const pointers.
//   The original MemRefFile is only usable with non-const pointers.


#include "fsal.h"
#include "MemRefFileReadOnly.h"

#include <cstdio>

using namespace fsal;

MemRefFileReadOnly::MemRefFileReadOnly()
    : m_data(nullptr), m_size(0), m_offset(0)
{}

MemRefFileReadOnly::MemRefFileReadOnly(const uint8_t* data, size_t size)
    : m_data(data), m_size(size), m_offset(0)
{}

MemRefFileReadOnly::MemRefFileReadOnly(std::shared_ptr<const uint8_t> data, size_t size)
    : m_data(data.get()), m_sharedData(data), m_size(size), m_offset(0)
{}

MemRefFileReadOnly::~MemRefFileReadOnly()
{}

bool MemRefFileReadOnly::ok() const
{
	return m_data != nullptr;
}

path MemRefFileReadOnly::GetPath() const
{
	return "memory";
}

Status MemRefFileReadOnly::Open(path filepath, Mode mode)
{
	return false;
}

Status MemRefFileReadOnly::ReadData(uint8_t* dst, size_t size, size_t* pbytesRead)
{
	Status status = true;
	if (m_size <= m_offset)
	{
		if (pbytesRead != nullptr)
		{
			*pbytesRead = 0;
		}
		return Status::kEOF;
	}
	else if (m_size < m_offset + size)
	{
		size = m_size - m_offset;
		status.state |= Status::kEOF;
	}

	memcpy(dst, m_data + m_offset, size);
	m_offset += size;

	if (pbytesRead != nullptr)
	{
		*pbytesRead = size;
	}
	return status;
}

Status MemRefFileReadOnly::WriteData(const uint8_t* src, size_t size)
{
	return false;
}

Status MemRefFileReadOnly::SetPosition(size_t position) const
{
	m_offset = position;
	return true;
}

size_t MemRefFileReadOnly::GetPosition() const
{
	return m_offset;
}

size_t MemRefFileReadOnly::GetSize() const
{
	return m_size;
}

Status MemRefFileReadOnly::FlushBuffer() const
{
	return true;
}
