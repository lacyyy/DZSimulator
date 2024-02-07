#include "csgo_parsing/AssetFinder.h"

#ifndef DZSIM_WEB_PORT
// Here we only use Windows API calls to read registry entries. This is always
// done through the non-Unicode Windows code page versions of the API calls
// (function names ending in 'A') because we assume Steam's registry keys, value
// names and install path string are all ASCII and do not contain Unicode.
// Hence no '#define UNICODE' before the windows header include here.
#include <windows.h>
#endif

#include <cstring>
#include <fstream>
#include <string>
#include <utility>

#include <Tracy.hpp>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StringStl.h>
#include <Corrade/Utility/Debug.h>
#include <Corrade/Utility/Path.h>
#include <fsal.h>
#include <Magnum/Magnum.h>

#include "csgo_parsing/utils.h"

using namespace csgo_parsing;
using namespace csgo_parsing::AssetFinder;
using namespace Magnum;
namespace CorrPath = Corrade::Utility::Path;

#ifndef DZSIM_WEB_PORT
// We're looking for a registry key under HKEY_CURRENT_USER\Software\Valve\Steam
// Steam's install directory is saved under that key's value with name "SteamPath"
#define STEAM_REGISTRY_KEY_ENTRY_KEY HKEY_CURRENT_USER
#define STEAM_REGISTRY_KEY_PATH "SOFTWARE\\VALVE\\STEAM"
#define STEAM_INSTALL_VALUE_NAME "SteamPath"
#define STEAM_INSTALL_VALUE_TYPE REG_SZ // indicates a NULL-terminated string

// Sub-path in Steam's install dir leading to the file that lists all Steam
// library folders
#define STEAM_DIR_LIBRARYFOLDERS_VDF_PATH "steamapps/libraryfolders.vdf"
// Sub-path in a Steam library folder leading to CSGO's install dir
#define STEAM_LIB_FOLDER_CSGO_PATH "steamapps/common/Counter-Strike Global Offensive/"
#endif

// ---- INTERNAL VARIABLES ----

// Currently detected path to the 'csgo/' game folder.
// Path is in UTF-8 with forward slash directory separators.
static std::string s_csgo_path = "";

// Currently detected paths to '.bsp' files relative to '<csgo_path>/maps/' .
// Paths are in UTF-8 with forward slash directory separators.
static std::vector<std::string> s_map_files = {};

#ifndef DZSIM_WEB_PORT
// Get error message for a system-defined error
std::string GetSystemErrorMsg(const std::string& what_failed, DWORD err_code)
{
    LPVOID lp_msg_buf = NULL;

    bool fail = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lp_msg_buf, // output, null-terminated string
        0,
        NULL) == 0;

    if (fail) {
        LocalFree(lp_msg_buf);
        return what_failed + " failed with error code "
            + std::to_string(err_code)
            + ". Also failed to get error description.";
    }

    // Copy null-terminated string
    std::string sys_err_msg = std::string((const char *)lp_msg_buf);
    LocalFree(lp_msg_buf); // Free the buffer allocated by FormatMessageA

    // Remove trailing line-breaks
    while (!sys_err_msg.empty()) {
        if (sys_err_msg.back() == '\r' || sys_err_msg.back() == '\n')
            sys_err_msg.erase(sys_err_msg.length() - 1);
        else
            break;
    }

    return what_failed + " failed with error code " + std::to_string(err_code)
        + ": " + sys_err_msg;
}

// Attempt to read Steam's install path, which is assumed to be ASCII-only, from
// the given steam registry key's value. The received path is written into the
// given std::string if the operation succeeds.
// The code of the returned RetCode object is one of: SUCCESS, ERROR_STEAM_REGISTRY
utils::RetCode QuerySteamPathValue(HKEY hkey_steam, std::string *out_path) {
    DWORD ret;

    // Number of values for this key
    DWORD value_count;
    // Size of longest key value name, in Unicode chars, without NULL-terminator
    DWORD value_name_max_len;
    // Size of longest key value data, in bytes (!)
    DWORD value_data_max_len;

    ret = RegQueryInfoKeyA(hkey_steam, NULL, NULL, NULL, NULL, NULL, NULL,
        &value_count, &value_name_max_len, &value_data_max_len, NULL, NULL);

    if (ret != ERROR_SUCCESS)
        return {
            utils::RetCode::ERROR_STEAM_REGISTRY,
            GetSystemErrorMsg("RegQueryInfoKeyA", ret)
        };

    // Create value's name and data buffer
    // Make sure name buffer can hold the NULL-terminator
    // Make sure data buffer's size in bytes is at least value_data_max_len
    std::string value_name_buf(value_name_max_len + 1, '\0');
    std::string value_data_buf(value_data_max_len + 1, '\0');

    DWORD value_name_len; // Size of the name buffer, in Unicode chars
    DWORD value_data_len; // Size of the data buffer, in bytes (!)
    DWORD value_type;
    
    for (DWORD i = 0; i < value_count; i++) {
        value_name_buf[0] = '\0';

        // Tell RegEnumValueA the buffers' sizes
        value_name_len = value_name_buf.size(); // in Unicode chars
        value_data_len = value_data_buf.size(); // in bytes
            
        ret = RegEnumValueA(hkey_steam, i,
            (LPSTR)value_name_buf.data(), // out, get NULL-terminated string
            &value_name_len, // in and out
            NULL,
            &value_type, // out
            (LPBYTE)value_data_buf.data(), // out
            &value_data_len  // in and out
        );

        if (ret != ERROR_SUCCESS)
            return {
                utils::RetCode::ERROR_STEAM_REGISTRY,
                GetSystemErrorMsg("RegEnumValueA", ret)
            };

        // value_name_len now holds the length of this value's name without
        // the NULL-terminator.
        // 
        // value_data_len now holds the number of bytes stored in the data
        // buffer. If value_type is REG_SZ, REG_MULTI_SZ or REG_EXPAND_SZ,
        // value_data_len includes any terminating null character or characters.

        // Skip if value's type and name is not what we are looking for
        if (value_type != STEAM_INSTALL_VALUE_TYPE)
            continue;
        if (std::strcmp(value_name_buf.data(), STEAM_INSTALL_VALUE_NAME) != 0)
            continue;

        // Received value data string might not be NULL-terminated!
        // Append a NULL-terminator to be sure.
        if (value_data_len < value_data_buf.size())
            value_data_buf[value_data_len] = '\0';
        else
            value_data_buf.push_back('\0');

        // Copy value data up until the first NULL-terminator
        if(out_path)
            *out_path = std::string(value_data_buf.data());
        return { utils::RetCode::SUCCESS };
    }

    return {
        utils::RetCode::ERROR_STEAM_REGISTRY,
        "Steam registry key doesn't have a \"" STEAM_INSTALL_VALUE_NAME "\" value"
    };
}

// Gets Steam's installation path from the Windows registry. Uninstalling Steam
// doesn't remove it from the registry. The path is assumed to be ASCII-only.
// It is written into the given std::string if the operation succeeds.
// The code of the returned RetCode object is one of:
// SUCCESS, STEAM_NOT_INSTALLED, ERROR_STEAM_REGISTRY
utils::RetCode GetSteamInstallPath(std::string* out_path)
{
    HKEY h_key;
    DWORD ret_open = RegOpenKeyExA(STEAM_REGISTRY_KEY_ENTRY_KEY,
        STEAM_REGISTRY_KEY_PATH, 0, KEY_READ, &h_key);

    // Registry key was found and opened
    if (ret_open == ERROR_SUCCESS) {
        utils::RetCode ret_query = QuerySteamPathValue(h_key, out_path);

        DWORD ret_close = RegCloseKey(h_key);
        if (ret_close != ERROR_SUCCESS) // Printed, but ignored error
            Debug{} << "IGNORED ERROR: \""
            << GetSystemErrorMsg("RegCloseKey", ret_close).c_str() << "\"";

        if (ret_query.code != utils::RetCode::SUCCESS)
            return ret_query; // ERROR_STEAM_REGISTRY with description
        return { utils::RetCode::SUCCESS };
    }

    // Registry key doesn't exist
    if (ret_open == ERROR_FILE_NOT_FOUND)
        return { utils::RetCode::STEAM_NOT_INSTALLED };

    // Other error
    return {
        utils::RetCode::ERROR_STEAM_REGISTRY,
        GetSystemErrorMsg("RegOpenKeyExA", ret_open)
    };
}

// Tries to extract Steam's library folder paths from a file found in Steam's
// files under "<STEAM-INSTALL-DIR>/steamapps/libraryfolders.vdf". It has
// Valve's 'KeyValues' text file format. The file's encoding must be UTF-8 and
// the file path must not contain Unicode characters. This function extracts key
// values from it in a hacky and dodgy fashion. The returned library folder
// paths are in UTF-8 and can contain Unicode. The code of the returned RetCode
// object is one of: SUCCESS, ERROR_FILE_OPEN_FAILED
utils::RetCode GetSteamLibraryFolderPaths(
    const std::string& libraryfolders_vdf_file_path,
    std::vector<std::string>* out_paths)
{
    // The given text file is assumed to have Valve's KeyValues format.
    // https://developer.valvesoftware.com/wiki/KeyValues
    // This function was made specifically to extract certain key values from
    // a file in Steam's installation: "Steam\steamapps\libraryfolders.vdf"
    // We are assuming the "path" key's name and value are always together in
    // a line on their own in this format:
    //   "path" "C:\\the\\path\\we\\want"
    // Any whitespace chars might be before, after or in between the key's name
    // and value. Backslashes in the key value are always doubled and need to be
    // turned into a single backslash. The key value is always enclosed with
    // double quotes, but must not contain one.

    if (out_paths)
        *out_paths = {};

    // We assume the file is always encoded in UTF-8, we must return the
    // extracted paths in UTF-8 too. We also assume this file path does not have
    // Unicode characters, otherwise we might not find it with std::ifstream().
    // Luckily, Steam itself seems to require to be installed under an
    // ASCII-only path in order to run at all.
    std::ifstream ifs = std::ifstream(libraryfolders_vdf_file_path);
    if (!ifs.is_open())
        return {
            utils::RetCode::ERROR_FILE_OPEN_FAILED,
            "Failed to open \"" + libraryfolders_vdf_file_path + "\""
    };

    const std::string key_name_target = "\"path\""; // key name: "path"

    for (std::string line; std::getline(ifs, line); ) {
        // Find first non-whitespace char position (key name start)
        long key_name_start = -1;
        for (size_t i = 0; i < line.length(); i++) {
            if (!std::isspace(line[i])) {
                key_name_start = i;
                break;
            }
        }
        if (key_name_start == -1) // No non-whitespace char in line
            continue;

        // Check if line starts with the key name we want
        std::string key_name =
            line.substr(key_name_start, key_name_target.length());
        if (key_name.length() != key_name_target.length()
            || key_name.compare(key_name_target) != 0)
            continue;

        // Following char must be whitespace
        size_t cpos = key_name_start + key_name.length();
        if (cpos >= line.length() || !std::isspace(line[cpos]))
            continue;

        // Find first non-whitespace char position after that (key value start)
        long key_value_start = -1;
        for (size_t i = cpos + 1; i < line.length(); i++) {
            if (!std::isspace(line[i])) {
                key_value_start = i;
                break;
            }
        }
        if (key_value_start == -1) // No non-whitespace char after key name
            continue;
        if (line[key_value_start] != '\"') // Value must start with double quote
            continue;

        // Find end of key value -> find next double quote char
        long key_value_end = -1;
        for (size_t i = key_value_start + 1; i < line.length(); i++) {
            if (line[i] == '\"') {
                key_value_end = i;
                break;
            }
        }
        if (key_value_end == -1) // Key value does not end with double quote
            continue;

        // All chars after that may only be whitespace
        bool ignore_line = false;
        for (size_t i = key_value_end + 1; i < line.length(); i++)
            if (!std::isspace(line[i]))
                ignore_line = true;
        if (ignore_line) // There are non-whitespace chars after key value
            continue;

        // Extract key value enclosed by double quotes
        size_t key_value_len = (key_value_end - key_value_start) - 1;
        std::string key_value = line.substr(key_value_start + 1, key_value_len);

        // Key value must not contain single backslashes. However, double
        // backslashes are allowed and get parsed into a single backslash.
        std::string parsed_key_value;
        ignore_line = false;
        for (size_t i = 0; i < key_value.length(); i++) {
            if (key_value[i] == '\\') { // current char is a backslash
                i++; // Advance to next char and check it
                if (i < key_value.length() && key_value[i] == '\\') {
                    parsed_key_value += '\\';
                }
                else { // Backslash isn't followed by backslash -> ignore line
                    ignore_line = true;
                    break;
                }
            }
            else { // Every non-backslash char is copied as is
                parsed_key_value += key_value[i];
            }
        }
        if (ignore_line)
            continue;

        if (out_paths)
            out_paths->emplace_back(std::move(parsed_key_value));
    }

    return { utils::RetCode::SUCCESS };
}
#endif

// Explores all files under a directory recursively and returns their paths
// relative to the starting directory. Given path must be UTF-8 and directory
// separators must be forward slashes. Returned file paths are UTF-8 and can
// contain Unicode characters.
std::vector<Containers::String> ListFilePathsRecursively(const std::string& dir_path)
{
    std::vector<Containers::String> file_paths;

    // Utility::Path::list() expects given path param to be in UTF-8.
    // Corrade prints an error message if Utility::Path::list() fails.
    auto dir_contents = CorrPath::list(dir_path,
        CorrPath::ListFlag::SortAscending |
        CorrPath::ListFlag::SkipDotAndDotDot |
        CorrPath::ListFlag::SkipSpecial);
    if (dir_contents == Containers::NullOpt) // If Utility::Path::list() failed
        return file_paths;

    // Unnecessary string copies here, this is not performance critical though.
    for (Containers::String& dir_entry : *dir_contents) {
        Containers::String full_path = CorrPath::join(dir_path, dir_entry);
        if (CorrPath::isDirectory(full_path)) {
            std::vector<Containers::String> sub_file_paths =
                ListFilePathsRecursively(full_path);
            for (auto& sub_file_path : sub_file_paths) {
                file_paths.emplace_back(
                    CorrPath::join(dir_entry, sub_file_path));
            }
        }
        else {
            file_paths.emplace_back(dir_entry);
        }
    }
    return file_paths;
}

utils::RetCode AssetFinder::FindCsgoPath()
{
#ifdef DZSIM_WEB_PORT
    return { utils::RetCode::STEAM_NOT_INSTALLED };
#else
    // Clear previous results of FindCsgoPath(), RefreshMapFileList() and
    // RefreshVpkArchiveIndex()
    s_csgo_path = "";
    s_map_files.clear();
    fsal::FileSystem fs;
    fs.ClearSearchPaths();
    fs.UnmountAllArchives();

    // Find Steam installation
    std::string steam_path_str; // Steam install path is ASCII-only
    utils::RetCode ret_steam = GetSteamInstallPath(&steam_path_str);

    // ret codes STEAM_NOT_INSTALLED and ERROR_STEAM_REGISTRY just get returned
    if (ret_steam.code != utils::RetCode::SUCCESS)
        return ret_steam;
    
    // Replace backslashes with forward slashes, only needed on Windows
    Containers::String steam_path_cleansed =
        CorrPath::fromNativeSeparators(steam_path_str);

    // Path::join() expects paths' directory separators to be forward slashes
    Containers::String libfolders_file_path =
        CorrPath::join(steam_path_cleansed, STEAM_DIR_LIBRARYFOLDERS_VDF_PATH);

    // Uninstalling Steam doesn't remove its Windows registry entries. Instead,
    // we take this missing file as indication for Steam's uninstalling.
    if (!CorrPath::exists(libfolders_file_path))
        return { utils::RetCode::STEAM_NOT_INSTALLED };

    // We are assuming the file's content is in UTF-8. Extracted library folder
    // paths are in UTF-8 too and may contain Unicode.
    std::vector<std::string> library_folder_paths;
    utils::RetCode ret_lib = GetSteamLibraryFolderPaths(libfolders_file_path,
        &library_folder_paths);

    // ret code ERROR_FILE_OPEN_FAILED just gets returned
    if (ret_lib.code != utils::RetCode::SUCCESS)
        return ret_lib;

    for (std::string& libfolder_path : library_folder_paths) {
        Containers::String csgo_root_cleansed = CorrPath::join(
            CorrPath::fromNativeSeparators(libfolder_path),
            STEAM_LIB_FOLDER_CSGO_PATH);

        // Once we find the 'csgo.exe' file, we know CSGO is installed here
        Containers::String csgo_exe_path =
            CorrPath::join(csgo_root_cleansed, "csgo.exe");

        if (!CorrPath::exists(csgo_exe_path)) // No EXE found, ignore this dir
            continue;

        s_csgo_path = CorrPath::join(csgo_root_cleansed, "csgo/");
        Debug{} << "[AssetFinder] Found CSGO path:" << s_csgo_path.c_str();

        return { utils::RetCode::SUCCESS }; // Stop after finding first CSGO folder
    }
    
    return { utils::RetCode::CSGO_NOT_INSTALLED };
#endif
}

const std::string& AssetFinder::GetCsgoPath()
{
    return s_csgo_path;
}

void AssetFinder::RefreshMapFileList()
{
    s_map_files.clear();

    if (GetCsgoPath().empty()) // If CSGO's install dir wasn't found
        return;

    Debug{} << "[AssetFinder] Refreshing map file list...";

    Containers::String maps_dir = CorrPath::join(GetCsgoPath(), "maps/");
    std::vector<Containers::String> rel_file_list =
        ListFilePathsRecursively(maps_dir);

    // Filter out '.bsp' files
    for (Containers::String& rel_file_path : rel_file_list) {
        Containers::StringView file_name = CorrPath::split(rel_file_path).second();
        size_t len = file_name.size();
        if (len >= 4) { // Check file name's last 4 characters
            if (file_name[len-4] != '.') continue;
            if (file_name[len-3] != 'b' && file_name[len-3] != 'B') continue;
            if (file_name[len-2] != 's' && file_name[len-2] != 'S') continue;
            if (file_name[len-1] != 'p' && file_name[len-1] != 'P') continue;
            s_map_files.emplace_back(std::move(rel_file_path));
        }
    }

    Debug{} << "[AssetFinder] Refreshing map file list DONE";
}

const std::vector<std::string>& AssetFinder::GetMapFileList()
{
    return s_map_files;
}

utils::RetCode AssetFinder::RefreshVpkArchiveIndex(
    const std::vector<std::string>& file_ext_filter)
{
    ZoneScoped;

    fsal::FileSystem fs;
    fs.UnmountAllArchives(); // Delete the previous VPK archive index

    if (GetCsgoPath().empty()) // If CSGO's install dir wasn't found
        return { utils::RetCode::SUCCESS };

    // Indexing CSGO's VPK archives takes some time ( 100ms and more )
    Debug{} << "[AssetFinder] Refreshing VPK archive index...";
    auto archive =
        fsal::OpenVpkArchive(fs, GetCsgoPath(), "pak01_%s.vpk", file_ext_filter);

    if (!fs.MountArchive(archive))
        return { utils::RetCode::ERROR_VPK_PARSING_FAILED };

    Debug{} << "[AssetFinder] Refreshing VPK archive index DONE";

    return { utils::RetCode::SUCCESS };
}

bool AssetFinder::ExistsInGameFiles(const std::string& file_path)
{
#ifdef DZSIM_WEB_PORT
    return false;
#else
    // Look for file under search paths and inside the VPK archive index!
    fsal::Location loc(file_path, fsal::Location::kSearchPathsAndArchives);
    fsal::FileSystem fs;
    return fs.Exists(loc);
#endif
}
