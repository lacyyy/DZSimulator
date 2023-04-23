#ifndef CSGO_PARSING_ASSETFILEREADER_H_
#define CSGO_PARSING_ASSETFILEREADER_H_

#include <memory>
#include <string>

#include <Corrade/Containers/ArrayView.h>

namespace csgo_parsing {

    // Mainly used to read game files that were made accessible by AssetFinder,
    // but can also read files from memory or an absolute path.
    // Most methods return true if they succeeded, false if they failed.
    class AssetFileReader {
    public:
        AssetFileReader(); // Precalculates float LUT
        ~AssetFileReader(); // must be defined after Implementation was defined

        // Opens file for reading from absolute file path that can contain UTF-8
        // Unicode chars, e.g.: "C:\\Program Files\\abc-Я.txt"
        bool OpenFileFromAbsolutePath(const std::string& abs_file_path);

        // Opens file (e.g. "materials/props/crate.phy") from currently detected
        // game files, which are the files under the current
        // AssetFinder::GetCsgoPath() and the files that were indexed by the
        // most recent call to AssetFinder::RefreshVpkArchiveIndex().
        // NOTE: Your desired file can only be opened if it didn't get filtered
        // out by that VPK indexing function!
        // NOTE: Files packed inside BSP map files can't be opened by this method!
        bool OpenFileFromGameFiles(const std::string& game_file_path);

        // The underlying memory of 'file_data' must remain valid and unchanged
        // throughout all file read operations!
        bool OpenFileFromMemory(Corrade::Containers::ArrayView<const uint8_t> file_data);

        // This method requires that the reader is currently opened in a file.
        // From the currently opened file, selects a subrange and treats and
        // reads it as if it was a separate file.
        bool OpenSubFileFromCurrentlyOpenedFile(size_t subfile_pos, size_t subfile_len);

        // If last OpenFileFromXXX() or OpenSubFileFromCurrentlyOpenedFile()
        // operation was successful.
        bool IsOpenedInFile();

        // Get and set read position relative to beginning of file
        size_t GetPos();
        bool SetPos(size_t pos); // aka seek operation

        bool ReadByteArray(uint8_t* out, size_t len);
        bool ReadCharArray(char* out, size_t len);
        bool ReadLine(std::string& out, uint8_t delim = '\n', size_t fail_len = 1048576);

        bool ReadUINT8(uint8_t& out);
        bool ReadUINT16_LE(uint16_t& out); // little-endian
        bool ReadUINT32_LE(uint32_t& out); // little-endian
        bool ReadINT16_LE(int16_t& out); // little-endian
        bool ReadINT32_LE(int32_t& out); // little-endian

        bool ReadFLOAT32_LE(float& out); // little-endian

    private:
        struct Implementation;
        std::unique_ptr<Implementation> _impl;
    };

}

#endif // CSGO_PARSING_ASSETFILEREADER_H_
