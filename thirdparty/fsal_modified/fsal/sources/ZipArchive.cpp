// DZSIM_MOD: Commented out this entire file
/*

#include "bfio.h"
#include "fsal_common.h"
#include "ZipArchive.h"
#include "FileStream.h"
#include "MemRefFile.h"
#include "SubFile.h"
#include <cassert>
#include <stddef.h>
#include <lz4.h>
#include <lz4hc.h>
#include <zlib.h>


using namespace fsal;

namespace bfio
{
	template<class RW>
	inline void Serialize(RW& io, DataDescriptor& x)
	{
		io & x.CRC32;
		io & x.compressedSize;
		io & x.uncompressedSize;
	}
	template<class RW>
	inline void Serialize(RW& io, CentralDirectoryHeader& x)
	{
		io & x.centralFileHeaderSignature;
		io & x.versionMadeBy;
		io & x.versionNeededToExtract;
		io & x.generalPurposBbitFlag;
		io & x.compressionMethod;
		io & x.lastModFileTime;
		io & x.lastModFileDate;
		io & x.dataDescriptor;
		io & x.fileNameLength;
		io & x.extraFieldLength;
		io & x.fileCommentLength;
		io & x.diskNumberStart;
		io & x.internalFileAttributes;
		io & x.externalFileAttributes;
		io & x.relativeOffsetOfLocalHeader;
	}
	template<class RW>
	inline void Serialize(RW& io, EndOfCentralDirectoryRecord& x)
	{
		io & x.endOfCentralDirSignature;
		io & x.numberOfThisDisk;
		io & x.numberOfTheDiskWithTheStartOfTheCentralDirectory;
		io & x.totalNumberOfEntriesInTheCentralDirectoryOnThisDisk;
		io & x.totalNumberOfEntriesInTheCentralDirectory;
		io & x.sizeOfTheCentralDirectory;
		io & x.offsetOfStartOfCentralDirectory;
		io & x.ZIPFileCommentLength;
	}
	template<class RW>
	inline void Serialize(RW& io, LocalFileHeader& x)
	{
		io & x.localFileHeaderSignature;
		io & x.versionNeededToExtract;
		io & x.generalPurposeBitFlag;
		io & x.compressionMethod;
		io & x.lastModFileTime;
		io & x.lastModFileDate;
		io & x.dataDescriptor;
		io & x.fileNameLength;
		io & x.extraFieldLength;
	}
}

Status ZipReader::OpenArchive(File file_)
{
	file = std::move(file_);

	FileStream stream(file);

	ptrdiff_t sizeOfCDEND = bfio::SizeOf<EndOfCentralDirectoryRecord>();

	file.Seek(-sizeOfCDEND, File::End);

	EndOfCentralDirectoryRecord ecdr;
	stream >> ecdr;

	if (ecdr.endOfCentralDirSignature != ZIP_SIGNATURES::END_OF_CENTRAL_DIRECTORY_SIGN)
	{
		return false;
	}

	file.Seek(ecdr.offsetOfStartOfCentralDirectory, File::Beginning);

	std::string filename;

	CentralDirectoryHeader header = {0};
	LocalFileHeader fileHeader = {0};

	{
		bfio::SizeCalculator s;
		s << header;
		assert(s.GetSize() == sizeof(header));
	}
	{
		bfio::SizeCalculator s;
		s << fileHeader;
		assert(s.GetSize() == sizeof(fileHeader));
	}

	for (int i = 0; (int32_t)file.Tell() - ecdr.offsetOfStartOfCentralDirectory < ecdr.sizeOfTheCentralDirectory; ++i)
	{
		file.Read(header);

		assert(header.centralFileHeaderSignature == ZIP_SIGNATURES::CENTRAL_DIRECTORY_FILE_HEADER);

		size_t currPos = file.Tell();

		file.Seek(header.relativeOffsetOfLocalHeader, File::Beginning);

		file.Read(fileHeader);

		assert(fileHeader.localFileHeaderSignature == ZIP_SIGNATURES::LOCAL_HEADER);

		filename.resize(fileHeader.fileNameLength);

		file.Read((uint8_t*)&filename[0], fileHeader.fileNameLength);

		ZipEntryData entry;
		entry.compressionMethod = header.compressionMethod;
		entry.generalPurposeBitFlag = header.generalPurposBbitFlag;
		entry.sizeUncompressed = header.dataDescriptor.uncompressedSize;
		entry.sizeCompressed = header.dataDescriptor.compressedSize;
		entry.offset = header.relativeOffsetOfLocalHeader + sizeof(fileHeader) + fileHeader.fileNameLength + fileHeader.extraFieldLength;

		filelist.Add(entry, filename);

		file.Seek(currPos + header.fileNameLength + header.extraFieldLength + header.fileCommentLength, File::Beginning);
	}
	FileEntry<ZipEntryData> key("");
	filelist.GetIndex(key);

	return true;
}

File ZipReader::OpenFile(const fs::path& filepath)
{
	ZipEntryData entry = filelist.FindEntry(filepath);

	if (entry.offset != -1)
	{
		switch (entry.compressionMethod)
		{
			case ZIP_COMPRESSION::NONE:
			{
				return new SubFile(file.GetInterface(), entry.sizeUncompressed, entry.offset);
			}

			case ZIP_COMPRESSION::DEFLATE:
			{
				auto* memfile = new MemRefFile();
				memfile->Resize(entry.sizeUncompressed);
				auto* uncompressedBuffer = memfile->GetDataPointer();
				char* compressedBuffer = new char[entry.sizeCompressed];

				{
					File::LockGuard lock(file.GetInterface().get());
					file.Seek(entry.offset, File::Beginning);
					file.Read((uint8_t*)compressedBuffer, entry.sizeCompressed);
				}

				z_stream stream = {nullptr};
				int32_t err;
				stream.next_in = (Bytef*)compressedBuffer;
				stream.avail_in = (uInt)entry.sizeCompressed;
				stream.next_out = (Bytef*)uncompressedBuffer;
				stream.avail_out = entry.sizeUncompressed;
				stream.zalloc = (alloc_func)nullptr;
				stream.zfree = (free_func)nullptr;

				err = inflateInit2(&stream, -MAX_WBITS);
				if (err == Z_OK)
				{
					err = inflate(&stream, Z_FINISH);
					inflateEnd(&stream);
					if (err == Z_STREAM_END)
					{
						err = Z_OK;
					}
					inflateEnd(&stream);
				}

				if (err != Z_OK)
				{
					delete[] compressedBuffer;
					delete memfile;
					return File();
				}
				else
				{
					delete[] compressedBuffer;
					return memfile;
				}
			}

			case ZIP_COMPRESSION::LZ4:
			{
				auto* memfile = new MemRefFile();
				memfile->Resize(entry.sizeUncompressed);
				auto* uncompressedBuffer = memfile->GetDataPointer();

				char* compressedBuffer = new char[entry.sizeCompressed];

				{
					File::LockGuard lock(file.GetInterface().get());
					file.Seek(entry.offset, File::Beginning);
					file.Read((uint8_t*) compressedBuffer, entry.sizeCompressed);
				}

				int b = LZ4_decompress_fast((const char*)compressedBuffer, (char*)uncompressedBuffer, entry.sizeUncompressed);

				if (b <= 0)
				{
					delete[] compressedBuffer;
					delete memfile;
					return File();
				}
				else
				{
					delete[] compressedBuffer;
					return memfile;
				}
			}

			default:
			{
				return File();
			}
		}
	}

	return File();
}

void* ZipReader::OpenFile(const fs::path& filepath, std::function<void*(size_t size)> alloc)
{
	ZipEntryData entry = filelist.FindEntry(filepath);

	if (entry.offset != -1)
	{
		switch (entry.compressionMethod)
		{
			case ZIP_COMPRESSION::NONE:
			{
				auto* data = alloc(entry.sizeUncompressed);
				File::LockGuard lock(file.GetInterface().get());
				file.Seek(entry.offset, File::Beginning);
				file.Read((uint8_t*)data, entry.sizeUncompressed);
				return data;
			}

			case ZIP_COMPRESSION::DEFLATE:
			{
				auto* uncompressedBuffer = alloc(entry.sizeUncompressed);
				char* compressedBuffer = new char[entry.sizeCompressed];

				{
					File::LockGuard lock(file.GetInterface().get());
					file.Seek(entry.offset, File::Beginning);
					file.Read((uint8_t*) compressedBuffer, entry.sizeCompressed);
				}

				z_stream stream = {0};
				int32_t err;
				stream.next_in = (Bytef*)compressedBuffer;
				stream.avail_in = (uInt)entry.sizeCompressed;
				stream.next_out = (Bytef*)uncompressedBuffer;
				stream.avail_out = entry.sizeUncompressed;
				stream.zalloc = (alloc_func)nullptr;
				stream.zfree = (free_func)nullptr;

				err = inflateInit2(&stream, -MAX_WBITS);
				if (err == Z_OK)
				{
					err = inflate(&stream, Z_FINISH);
					inflateEnd(&stream);
					if (err == Z_STREAM_END)
					{
						err = Z_OK;
					}
					inflateEnd(&stream);
				}

				if (err != Z_OK)
				{
					delete[] compressedBuffer;
					return nullptr;
				}
				else
				{
					delete[] compressedBuffer;
					return uncompressedBuffer;
				}
			}
			default:
			{
				return nullptr;
			}
		}
	}

	return nullptr;
}

bool ZipReader::Exists(const fs::path& filepath, PathType type)
{
	std::string path = filepath.u8string();
	if (path.back() != '/' && type == PathType::kDirectory)
	{
		path += "/";
	}

	FileEntry<ZipEntryData> key(path);

	return filelist.Exists(key);
}

std::vector<std::string> ZipReader::ListDirectory(const fs::path& path)
{
	return filelist.ListDirectory(path);
}

ZipWriter::ZipWriter(const File& file): m_file(file), m_currOffset(0), m_sizeOfCD(0)
{
	file.Seek(0);
}

Status ZipWriter::AddFile(const fs::path& path, File file, int compression)
{
	int level = 9;
	bool encrypt = false;

	std::shared_ptr<uint8_t> dataPointer = std::shared_ptr<uint8_t>(file.GetDataPointer(), null_deleter<uint8_t>);

	int uncompressedSize = static_cast<int>(file.GetSize());
	int compressedSize = uncompressedSize;

	if (!dataPointer)
	{
		dataPointer.reset(new uint8_t[uncompressedSize]);
		file.Read(dataPointer.get(), uncompressedSize);
	}

	std::shared_ptr<uint8_t> dataPointerCompressed;

	if (compression == ZIP_COMPRESSION::LZ4)
	{
		int bound = LZ4_compressBound(uncompressedSize);

		dataPointerCompressed.reset(new uint8_t[bound]);
		memset(dataPointerCompressed.get(), 0, bound);
		if (uncompressedSize > 0)
		{
			compressedSize = LZ4_compressHC2((const char*)dataPointer.get(), (char*)dataPointerCompressed.get(), uncompressedSize, level);
		}
	}
	else if (compression == ZIP_COMPRESSION::DEFLATE)
	{
		z_stream stream;

		stream.next_in = (Bytef*)dataPointer.get();
		stream.avail_in = (uInt)uncompressedSize;
		stream.zalloc = (alloc_func)nullptr;
		stream.zfree = (free_func)nullptr;

		int err = deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, level, Z_DEFAULT_STRATEGY);
		if (err != Z_OK)
		{
			return false;
		}

		int bound = deflateBound(&stream, uncompressedSize);
		dataPointerCompressed.reset(new uint8_t[bound]);
		memset(dataPointerCompressed.get(), 0, bound);
		stream.next_out = (Bytef*)dataPointerCompressed.get();
		stream.avail_out = bound;
		err = deflate(&stream, Z_FINISH);
		if (err != Z_STREAM_END)
		{
			return false;
		}
		err = deflateEnd(&stream);
		if (err != Z_OK)
		{
			return false;
		}
		compressedSize = stream.total_out;
	}

	CentralDirectoryHeader header = {0};
	LocalFileHeader fileHeader = {0};

	fileHeader.localFileHeaderSignature = ZIP_SIGNATURES::LOCAL_HEADER;
	fileHeader.versionNeededToExtract = ZIP_SIGNATURES::VERSION;
	fileHeader.generalPurposeBitFlag = encrypt ? 1 : 0;
	fileHeader.compressionMethod = static_cast<uint16_t>(compression);

	uint64_t ctime = file.GetLastWriteTime();
	tm* tm_time = gmtime((time_t*)&ctime);

	fileHeader.lastModFileDate = 0U
			| (((uint32_t)tm_time->tm_mday & 0b00011111U) << 0U)
			| (((uint32_t)(tm_time->tm_mon + 1) & 0b00001111U) << 5U)
			| (((uint32_t)(tm_time->tm_year - 80) & 0b01111111U) << 9U);

	fileHeader.lastModFileTime = 0U
			| (((uint32_t)(tm_time->tm_sec / 2) & 0b00011111U) << 0U)
			| (((uint32_t)tm_time->tm_min & 0b00111111U) << 5U)
			| (((uint32_t)(tm_time->tm_hour - 80) & 0b00011111U) << 11U);

	fileHeader.dataDescriptor.CRC32 = crc32(0, dataPointer.get(), uncompressedSize);
	fileHeader.dataDescriptor.compressedSize = compressedSize;
	fileHeader.dataDescriptor.uncompressedSize = uncompressedSize;
	fileHeader.fileNameLength = path.string().size();
	fileHeader.extraFieldLength = 0;

	header.centralFileHeaderSignature = ZIP_SIGNATURES::CENTRAL_DIRECTORY_FILE_HEADER;

	header.versionMadeBy = ZIP_SIGNATURES::VERSION;
	header.versionNeededToExtract = ZIP_SIGNATURES::VERSION;
	header.generalPurposBbitFlag = encrypt ? 1 : 0;
	header.compressionMethod = static_cast<uint16_t>(compression);
	header.lastModFileDate = fileHeader.lastModFileDate;
	header.lastModFileTime = fileHeader.lastModFileTime;
	header.dataDescriptor = fileHeader.dataDescriptor;
	header.fileNameLength = fileHeader.fileNameLength;
	header.extraFieldLength = 0;
	header.fileCommentLength = 0;
	header.diskNumberStart = 0;
	header.internalFileAttributes = 0;
	header.externalFileAttributes = 0;
	header.relativeOffsetOfLocalHeader = m_currOffset;

	std::string filepathStr = path.string();
	m_headers.emplace_back(header, filepathStr);

	m_sizeOfCD += (int32_t)(sizeof(CentralDirectoryHeader) + filepathStr.size());
	m_currOffset += (int32_t)(sizeof(LocalFileHeader) + filepathStr.size() + compressedSize);

	m_file.Write(fileHeader);
	m_file.Write((uint8_t*)filepathStr.c_str(), filepathStr.size());

	uint8_t* data_ptr_to_write = nullptr;
	size_t data_size_to_write = 0;

	if (compression == ZIP_COMPRESSION::NONE)
	{
		data_ptr_to_write = dataPointer.get();
		data_size_to_write = uncompressedSize;
	}
	else
	{
		data_ptr_to_write = dataPointerCompressed.get();
		data_size_to_write = compressedSize;
	}
	m_file.Write(data_ptr_to_write, data_size_to_write);

	return true;
}

Status ZipWriter::CreateDirectory(const fs::path& path)
{
	CentralDirectoryHeader header = {0};
	LocalFileHeader fileHeader = {0};

	std::string dir_path = path.string();

	if (dir_path.size() == 0)
		return false;
	if (dir_path[dir_path.size()-1] == '\\')
		dir_path[dir_path.size()-1] = '/';
	dir_path = NormalizePath(dir_path);
	if (dir_path[dir_path.size()-1] != '/')
		dir_path += '/';

	fileHeader.localFileHeaderSignature = ZIP_SIGNATURES::LOCAL_HEADER;
	fileHeader.versionNeededToExtract = ZIP_SIGNATURES::VERSION;
	fileHeader.fileNameLength = dir_path.size();

	header.centralFileHeaderSignature = ZIP_SIGNATURES::CENTRAL_DIRECTORY_FILE_HEADER;
	header.versionMadeBy = ZIP_SIGNATURES::VERSION;
	header.versionNeededToExtract = ZIP_SIGNATURES::VERSION;
	header.dataDescriptor = fileHeader.dataDescriptor;
	header.fileNameLength = fileHeader.fileNameLength;
	header.relativeOffsetOfLocalHeader = m_currOffset;

	m_headers.emplace_back(header, dir_path);

	m_sizeOfCD += (int32_t)(sizeof(CentralDirectoryHeader) + dir_path.size());
	m_currOffset += (int32_t)(sizeof(LocalFileHeader) + dir_path.size());

	m_file.Write(fileHeader);
	m_file.Write((uint8_t*)dir_path.c_str(), dir_path.size());
	return true;
}

ZipWriter::~ZipWriter()
{
	for (auto& pair: m_headers)
	{
		m_file.Write(pair.first);
		m_file.Write((uint8_t*)pair.second.c_str(), pair.second.size());
	}

	EndOfCentralDirectoryRecord ecdr = {0};

	ecdr.endOfCentralDirSignature = ZIP_SIGNATURES::END_OF_CENTRAL_DIRECTORY_SIGN;
	ecdr.numberOfThisDisk = 0;
	ecdr.numberOfTheDiskWithTheStartOfTheCentralDirectory = 0;
	ecdr.totalNumberOfEntriesInTheCentralDirectory = (int16_t)(m_headers.size());
	ecdr.totalNumberOfEntriesInTheCentralDirectoryOnThisDisk = (int16_t)(m_headers.size());
	ecdr.sizeOfTheCentralDirectory = m_sizeOfCD;
	ecdr.offsetOfStartOfCentralDirectory = m_currOffset;
	ecdr.ZIPFileCommentLength = 0;

	FileStream stream(m_file);

	stream << ecdr;
}

*/
