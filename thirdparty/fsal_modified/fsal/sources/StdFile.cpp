#include "fsal.h"
#include "StdFile.h"

#include <cstdio>

using namespace fsal;

StdFile::StdFile(): m_file(nullptr)
{

}

StdFile::~StdFile()
{
	if (m_file != nullptr)
	{
		std::fclose(m_file);
	}
	m_file = nullptr;
}

bool StdFile::ok() const
{
	return m_file != nullptr;
}

path StdFile::GetPath() const
{
	return m_path;
}

Status StdFile::Open(path filepath, Mode mode)
{
	const char* modeStr = nullptr;
	switch (mode)
	{
	case kRead:
		modeStr = "rb";
		break;
	case kWrite:
		modeStr = "wb";
		break;
	case kAppend:
		modeStr = "ab";
		break;
	case kReadUpdate:
		modeStr = "r+b";
		break;
	case kWriteUpdate:
		modeStr = "w+b";
		break;
	case kAppendUpdate:
		modeStr = "w+b";
		break;
	}

	m_path = fs::absolute(filepath);
#ifdef WIN32
	std::wstring wmodeStr;
	std::string smodeStr(modeStr);
	wmodeStr.assign(smodeStr.begin(), smodeStr.end());
	errno_t error = _wfopen_s(&m_file, m_path.wstring().c_str(), wmodeStr.c_str());
	if (error != 0)
	{
		return false;
	}
#else
	m_file = std::fopen(m_path.string().c_str(), modeStr);
#endif
	return m_file != nullptr;
}

Status StdFile::ReadData(uint8_t* dst, size_t size, size_t* bytesRead)
{
	size_t retSize = std::fread(dst, 1, size, m_file);
	if (bytesRead != nullptr)
	{
		*bytesRead = retSize;
	}
	bool eof = std::feof(m_file);
	bool error = std::ferror(m_file);
	return (error ? Status::kFailed: Status::kOk) | (eof ? Status::kEOF: Status::kOk);
}

Status StdFile::WriteData(const uint8_t* src, size_t size)
{
	size_t writeSize = std::fwrite(src, 1, size, m_file);
	bool error = std::ferror(m_file);
	return (writeSize == size) && !error;
}

Status StdFile::SetPosition(size_t position) const
{
#ifdef _WIN32
	return _fseeki64(m_file, position, SEEK_SET) == 0;
#elif __linux
	return fseeko64(m_file, position, SEEK_SET) == 0;
#else
	return std::fseek(m_file, position, SEEK_SET) == 0;
#endif
}

size_t StdFile::GetPosition() const
{
#ifdef _WIN32
	return static_cast<size_t>(_ftelli64(m_file));
#elif __linux
	return ftello64(m_file);
#else
	return std::ftell(m_file);
#endif
}

size_t StdFile::GetSize() const
{
	size_t currentPosition = GetPosition();
#ifdef _WIN32
	_fseeki64(m_file, 0, SEEK_END);
#elif __linux
	fseeko64(m_file, 0, SEEK_END);
#else
	std::fseek(m_file, 0, SEEK_END);
#endif
	size_t size = GetPosition();
	SetPosition(currentPosition);
	return size;
}

Status StdFile::FlushBuffer() const
{
	return std::fflush(m_file) == 0;
}

// DZSIM_MOD: Commented out this function for C++20 compatibility
/*
uint64_t StdFile::GetLastWriteTime() const
{
	return fs::file_time_type::clock::to_time_t(fs::last_write_time(m_path));
}
*/

const uint8_t* StdFile::GetDataPointer() const
{
	return nullptr;
}

uint8_t* StdFile::GetDataPointer()
{
	return nullptr;
}

void StdFile::AssignFile(FILE* file)
{
	m_file = file;
}
