#include "fsal.h"
#include "MemRefFile.h"

#include <cstdio>

using namespace fsal;

enum
{
	min_size = 0x32
};

MemRefFile::MemRefFile(): m_data(nullptr), m_size(min_size), m_offset(0), m_hasOwnership(true)
{
	m_data = static_cast<uint8_t*>(malloc(min_size));
	m_reserved = min_size;
}

MemRefFile::MemRefFile(uint8_t* data, size_t size, bool copy): m_data(data), m_size(size), m_offset(0), m_hasOwnership(copy)
{
	if (copy && data != nullptr)
	{
		m_data = static_cast<uint8_t*>(malloc(size));
		memcpy(m_data, data, size);
		m_reserved = size;
	}
	else if (data == nullptr)
	{
		m_data = static_cast<uint8_t*>(malloc(size));
		m_reserved = size;
	}
}

MemRefFile::MemRefFile(std::shared_ptr<uint8_t> data, size_t size) : m_data(data.get()), m_sharedData(data), m_size(size), m_offset(0), m_hasOwnership(false), m_reserved(size)
{
}

MemRefFile::~MemRefFile()
{
	if (m_hasOwnership)
	{
		free(m_data);
	}
}

bool MemRefFile::ok() const
{
	return m_data != nullptr;
}

path MemRefFile::GetPath() const
{
	return "memory";
}

Status MemRefFile::Open(path filepath, Mode mode)
{
	return false;
}

Status MemRefFile::ReadData(uint8_t* dst, size_t size, size_t* pbytesRead)
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

Status MemRefFile::WriteData(const uint8_t* src, size_t size)
{
	if (Resize(size + m_offset))
	{
		memcpy(m_data + m_offset, src, size);
		m_offset += size;
		return true;
	}
	return false;
}

Status MemRefFile::SetPosition(size_t position) const
{
	m_offset = position;
	return true;
}

size_t MemRefFile::GetPosition() const
{
	return m_offset;
}

size_t MemRefFile::GetSize() const
{
	return m_size;
}

Status MemRefFile::FlushBuffer() const
{
	return true;
}

const uint8_t* MemRefFile::GetDataPointer() const
{
	return m_data;
}

uint8_t* MemRefFile::GetDataPointer()
{
	return m_data;
}

bool MemRefFile::Resize(size_t newSize)
{
	if (!m_hasOwnership)
	{
		return m_size >= newSize;
	}
	else if (newSize > m_reserved)
	{
		uint64_t v = newSize + 1;

		v--;
		v |= v >> 1U;
		v |= v >> 2U;
		v |= v >> 4U;
		v |= v >> 8U;
		v |= v >> 16U;
		v |= v >> 32U;
		v++;

		m_reserved = static_cast<size_t>(v);
		m_data = static_cast<uint8_t*>(realloc(m_data, m_reserved));
	}
	m_size = newSize;
	return m_data != nullptr;
}
