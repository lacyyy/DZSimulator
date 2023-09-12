#include "bfio.h"
#include "fsal_common.h"
#include "VpkArchive.h"
#include "FileStream.h"
#include "MemRefFile.h"
#include "SubFile.h"
#include <cassert>
/* #include <zlib.h> */ // DZSIM_MOD: Commented out zlib include


using namespace fsal;


static std::string read_string(const fsal::File& file)
{
	std::string buf;

    // DZSIM_MOD: Improved this function's speed in release builds
    uint8_t c;
    while (true) {
        if (file.Read(&c, 1).state != Status(true).state)
            return "";
        if (c == 0)
            return buf;
        buf += c;
    }

    // DZSIM_MOD: This is the original read_string() code
	/*char chunk[64];
	while (true)
	{
		if (file.Read((uint8_t*)chunk, 64).state != Status(true).state)
            return ""; // DZSIM_MOD Added fail check
		buf += chunk;
		int i = 0;
		for (; i < 64 && chunk[i] != 0; ++i);
		if (chunk[i] == 0)
		{
			file.Seek(file.Tell() - (64 - (i + 1)));
			break;
		}
	}*/

	return buf;
}

// DZSIM_MOD: Added destructor to fix memory leak
VPKReader::~VPKReader() {
    // preload data buffers were allocated with malloc
    for (const FileEntry<VpkEntryData>& entry : filelist.GetInternalFileList())
        free(entry.data.preloadData);
}

// DZSIM_MOD: Fixed memory leaks and added fail checks and extension filtering
Status VPKReader::OpenArchive(FileSystem fs, Location directory,
    const std::string& formatString, const std::vector<std::string>& ext_filter)
{
    Status::State success = Status(true).state;

	m_formatString = formatString;
	m_directory = std::move(directory);
	char buff[2048];
	sprintf(buff, m_formatString.c_str(), "dir");
	m_index = fs.Open(m_directory / buff);

    if (!m_index)
        return false;

    VPKHeader_v2 header;

    if (m_index.Read((uint8_t*)&header, sizeof(uint32_t) * 3).state != success)
        return false;
    if (header.Signature != VPK_SIGNATURES::HEADER)
        return false;
    if (header.Version != 1 && header.Version != 2)
        return false;

    if (header.Version == 2)
    {
        if (m_index.Seek(0)     .state != success) return false;
        if (m_index.Read(header).state != success) return false;
    }

	//printf( "Signature: 0x%08x\n", header.Signature);
	//printf( "Version: %d\n", header.Version);
	//printf( "Directory length: %d\n", header.TreeSize);

	size_t tree_begin_ptr = m_index.Tell();

    std::vector<uint8_t> throwaway_preload_buf;
    throwaway_preload_buf.resize(65535); // Max size of preload section of dir entries
    uint8_t* p_throwaway_preload_buf = throwaway_preload_buf.data();

	while (true)
	{
        if (m_index.Tell() - tree_begin_ptr >= header.TreeSize)
            return false;

		auto ext = read_string(m_index);

		if (ext.empty())
		{
			break;
		}

        // DZSIM_MOD: Determine if files with this extension should get indexed
        bool skip_this_ext;
        if (ext_filter.size() == 0) { // Empty filter list -> Index all files!
            skip_this_ext = false;
        }
        else {
            skip_this_ext = true;
            for (const std::string& desired_ext : ext_filter) {
                if (ext.compare(desired_ext) == 0) {
                    skip_this_ext = false;
                    break;
                }
            }
        }

		while (true)
		{
			auto path = read_string(m_index);

			if (path.empty())
			{
				break;
			}
			if (path == " ")
				path = "";

			while (true)
			{
				auto name = read_string(m_index);

				if (name.empty())
				{
					break;
				}
				VPKDirectoryEntry entryHeader;
                if (m_index.Read(entryHeader).state != success)
                    return false;

                if (entryHeader.Terminator != VPK_SIGNATURES::DIRECTORY_ENTRY_TERMINATOR)
                    return false;

				VpkEntryData entry = {0};
				entry.PreloadBytes = entryHeader.PreloadBytes;
				entry.ArchiveIndex = entryHeader.ArchiveIndex;
				entry.EntryOffset = entryHeader.EntryOffset;
				entry.EntryLength = entryHeader.EntryLength;
				entry.preloadData = nullptr;

				if (entryHeader.PreloadBytes > 0)
				{
                    // If preload data will be thrown away, save a malloc call
                    uint8_t* read_dst = p_throwaway_preload_buf;
                    
                    if (!skip_this_ext) { // If Preload data might be used
                        // These buffers get freed in VPKReader::~VPKReader()
                        entry.preloadData = (uint8_t*)malloc(entryHeader.PreloadBytes);
                        read_dst = entry.preloadData;
                    }

                    if (m_index.Read(read_dst, entryHeader.PreloadBytes).state != success) {
                        if (!skip_this_ext)
                            free(entry.preloadData);
                        return false;
                    }
				}

                // DZSIM_MOD: Files that don't have desired extensions don't get indexed
                if (skip_this_ext)
                    continue;

				sprintf(buff, "%s/%s.%s", path.c_str(), name.c_str(), ext.c_str());
				filelist.Add(entry, buff);
			}
		}
	}

	FileEntry<VpkEntryData> key("");
	filelist.GetIndex(key);

	return true;
}

File VPKReader::OpenPak(int index)
{
	auto it = m_pak_files.find(index);
	if (it != m_pak_files.end())
	{
		return it->second;
	}
	char buff[2048];
	char buff2[32];
	sprintf(buff2, "%03d", index);
	sprintf(buff, m_formatString.c_str(), buff2);
	File file = m_fs.Open(m_directory / buff, Mode::kRead, true);
	m_pak_files[index] = file;
	return file;
}

File VPKReader::OpenFile(const fs::path& filepath)
{
    // DZSIM_MOD: Just commenting, let's hope the entry is always found here!
	VpkEntryData entry = filelist.FindEntry(filepath);

	File file;
	uint32_t offset = entry.EntryOffset;

	if (entry.ArchiveIndex == 0x7fff)
	{
		file = m_index;
	}
	else
	{
		file = OpenPak(entry.ArchiveIndex);
	}

	if (entry.PreloadBytes != 0 || entry.EntryOffset != 0)
	{
		if (entry.PreloadBytes != 0 || entry.PreloadBytes + entry.EntryLength < 1024 * 16)
		{
			auto* memfile = new MemRefFile();
			memfile->Resize(entry.PreloadBytes + entry.EntryLength);
			auto* data = memfile->GetDataPointer();
			memcpy(data, entry.preloadData, entry.PreloadBytes);

			if (entry.EntryLength > 0)
			{
                if (!file) // DZSIM_MOD: Added fail check
                    return File();

				m_fileMutex.lock();
				file.Seek(offset, File::Beginning);
				file.Read((uint8_t*) data + entry.PreloadBytes, entry.EntryLength);
				m_fileMutex.unlock();
			}
			return memfile;
		}
		else
		{
            if (!file) // DZSIM_MOD: Added fail check
                return File();

			auto* subfile = new SubFile(file.GetInterface(), (size_t)entry.EntryLength, (size_t)entry.EntryOffset);
			return subfile;
		}
	}

	return File();
}


bool VPKReader::Exists(const fs::path& filepath, PathType type)
{
    // DZSIM_MOD: Changed method from u8string() to string() for C++20 compatibility
    std::string path = filepath.string();
	// std::string path = filepath.u8string();
	if (path.back() != '/' && type == PathType::kDirectory)
	{
		path += "/";
	}

	FileEntry<VpkEntryData> key(path);

	return filelist.Exists(key);
}

std::vector<std::string> VPKReader::ListDirectory(const fs::path& path)
{
	return filelist.ListDirectory(path);
}
