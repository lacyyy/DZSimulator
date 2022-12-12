#pragma once
#include "ArchiveInterface.h"
#include "FileListBinarySearch.h"
#include "FileSystem.h"

namespace fsal
{
	namespace VPK_SIGNATURES
	{
		enum
		{
			HEADER = 0x55aa1234,
			DIRECTORY_ENTRY_TERMINATOR = 0xffff,
		};
	}

#pragma pack(push,1)

	struct VPKHeader_v2
	{
		uint32_t Signature = VPK_SIGNATURES::HEADER;
		uint32_t Version = 2;
		uint32_t TreeSize;
		uint32_t FileDataSectionSize;
		uint32_t ArchiveMD5SectionSize;
		uint32_t OtherMD5SectionSize;
		uint32_t SignatureSectionSize;
	};

	struct VPKDirectoryEntry
	{
		uint32_t CRC;
		uint16_t PreloadBytes;
		uint16_t ArchiveIndex;
		uint32_t EntryOffset;
		uint32_t EntryLength;
		uint16_t Terminator = VPK_SIGNATURES::DIRECTORY_ENTRY_TERMINATOR;
	};

	struct VPK_ArchiveMD5SectionEntry
	{
		uint32_t ArchiveIndex;
		uint32_t StartingOffset;
		uint32_t Count;
		int8_t MD5Checksum[16];
	};

	struct VPK_OtherMD5Section
	{
		int8_t TreeChecksum[16];
		int8_t ArchiveMD5SectionChecksum[16];
		int8_t Unknown[16];
	};

	struct VpkEntryData
	{
		uint16_t PreloadBytes;
		uint16_t ArchiveIndex;
		uint32_t EntryOffset;
		uint32_t EntryLength;
		uint8_t* preloadData = nullptr;
	};

#pragma pack(pop)

	class VPKReader: ArchiveReaderInterface
	{
	public:
        // DZSIM_MOD: Added destructor to fix memory leak
        ~VPKReader();

        // DZSIM_MOD: Added fourth param 'ext_filter' for optimization that is a
        //            list of file extensions (e.g. 'mdl' or 'phy'). All files
        //            with one of those extensions get indexed and made
        //            available for the methods: Exists(), OpenFile() and
        //            ListDirectory(). Passing an empty list to ext_filter
        //            causes all file extensions to be indexed.
		Status OpenArchive(FileSystem fs, Location directory,
            const std::string& formatString = "pak01_%s.vpk",
            const std::vector<std::string>& ext_filter = {});

		File OpenFile(const fs::path& filepath) override;

		void* OpenFile(const fs::path& /*filepath*/, std::function<void* (size_t size)> alloc_func) override { return nullptr; };

		bool Exists(const fs::path& filepath, PathType type = kFile | kDirectory) override;

		std::vector<std::string> ListDirectory(const fs::path& path) override;

	private:
		File OpenPak(int index);

		FileList<VpkEntryData> filelist;
		File m_index;
		std::mutex m_fileMutex;
		FileSystem m_fs;
		std::string m_formatString;
		Location m_directory;
		std::map<int, File> m_pak_files;
	};

    // DZSIM_MOD: Added fourth param 'ext_filter' for optimization that is a
    //            list of file extensions (e.g. 'mdl' or 'phy'). All files
    //            with one of those extensions get indexed and made
    //            available for the methods: Exists(), OpenFile() and
    //            ListDirectory(). Passing an empty list to ext_filter
    //            causes all file extensions to be indexed.
	inline Archive OpenVpkArchive(FileSystem fs, Location directory,
        const std::string& formatString = "pak01_%s.vpk",
        const std::vector<std::string>& ext_filter = {})
	{
		auto* reader = new VPKReader();
		if (reader->OpenArchive(std::move(fs), std::move(directory), formatString, ext_filter))
		{
			Archive archiveReader(ArchiveReaderInterfacePtr((ArchiveReaderInterface*)reader));
			return archiveReader;
		}
		return Archive();
	}
}
