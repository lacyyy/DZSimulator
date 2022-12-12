#pragma once

// DZSIM_MOD: Commented out this entire file
/*

#include "ArchiveInterface.h"
#include "FileListBinarySearch.h"
#include "Archive.h"
// #include "FileListHashMap.h"


namespace fsal
{
#if defined(_WIN32) || defined(_WIN64)
#  if defined(_WIN64)
    typedef __int64 LONG_PTR;
#  else
    typedef long LONG_PTR;
#  endif
	typedef LONG_PTR SSIZE_T;
	typedef SSIZE_T ssize_t;
#endif 

	namespace ZIP_SIGNATURES
	{
		enum
		{
			CENTRAL_DIRECTORY_FILE_HEADER = 0x02014b50,
			END_OF_CENTRAL_DIRECTORY_SIGN = 0x06054b50,
			LOCAL_HEADER = 0x04034b50,
			VERSION = 46,
		};
	}

	namespace ZIP_COMPRESSION
	{
		enum Compression
		{
			NONE = 0,
			DEFLATE = 8,
			LZ4 = 30
		};
	}

#pragma pack(push,1)

	struct DataDescriptor
	{
		int32_t CRC32;
		int32_t compressedSize;
		int32_t uncompressedSize;
	};

	struct CentralDirectoryHeader
	{
		int32_t centralFileHeaderSignature;
		int16_t versionMadeBy;
		int16_t	versionNeededToExtract;
		int16_t	generalPurposBbitFlag;
		int16_t	compressionMethod;
		int16_t	lastModFileTime;
		int16_t	lastModFileDate;
		DataDescriptor dataDescriptor;
		int16_t	fileNameLength;
		int16_t	extraFieldLength;
		int16_t	fileCommentLength;
		int16_t	diskNumberStart;
		int16_t	internalFileAttributes;
		int32_t	externalFileAttributes;
		int32_t	relativeOffsetOfLocalHeader;
	};

	struct EndOfCentralDirectoryRecord
	{
		int32_t endOfCentralDirSignature;
		int16_t	numberOfThisDisk;
		int16_t	numberOfTheDiskWithTheStartOfTheCentralDirectory;
		int16_t	totalNumberOfEntriesInTheCentralDirectoryOnThisDisk;
		int16_t	totalNumberOfEntriesInTheCentralDirectory;
		int32_t	sizeOfTheCentralDirectory;
		int32_t	offsetOfStartOfCentralDirectory;
		int16_t ZIPFileCommentLength;
	};

	struct LocalFileHeader
	{
		int32_t localFileHeaderSignature;
		int16_t	versionNeededToExtract;
		int16_t	generalPurposeBitFlag;
		int16_t	compressionMethod;
		int16_t	lastModFileTime;
		int16_t	lastModFileDate;
		DataDescriptor dataDescriptor;
		int16_t	fileNameLength;
		int16_t	extraFieldLength;
	};

	struct ZipEntryData
	{
		size_t sizeCompressed = 0;
		size_t sizeUncompressed = 0;
		ssize_t offset = -1;
		int16_t	compressionMethod = 0;
		int16_t	generalPurposeBitFlag = 0;
	};

#pragma pack(pop)

	class ZipReader: public ArchiveReaderInterface
	{
	public:
		Status OpenArchive(File file);

		File OpenFile(const fs::path& filepath) override;

		void* OpenFile(const fs::path& filepath, std::function<void*(size_t size)> alloc_func) override;

		bool Exists(const fs::path& filepath, PathType type = kFile | kDirectory) override;

		std::vector<std::string> ListDirectory(const fs::path& path) override;
	private:
		FileList<ZipEntryData> filelist;
		File file;
	};

	inline Archive OpenZipArchive(const File& archive)
	{
		auto* zipReader = new ZipReader();
		if (zipReader->OpenArchive(archive))
		{
			Archive archiveReader(ArchiveReaderInterfacePtr((ArchiveReaderInterface*)zipReader));
			return archiveReader;
		}
		return Archive();
	}
	class ZipWriter : public ArchiveWriterInterface
	{
	public:
		ZipWriter(const File& file);
		~ZipWriter();

		Status AddFile(const fs::path& path, File file, int compression = ZIP_COMPRESSION::DEFLATE) override;

		Status CreateDirectory(const fs::path& path) override;

	private:
		FileList<ZipEntryData> filelist;
		File m_file;

		int32_t m_currOffset;
		int32_t m_sizeOfCD;

		std::vector<std::pair<CentralDirectoryHeader, std::string> > m_headers;
	};
}
*/
