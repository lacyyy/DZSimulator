#ifndef CSGO_PARSING_ASSETFINDER_H_
#define CSGO_PARSING_ASSETFINDER_H_

#include <string>
#include <vector>

#include "csgo_parsing/utils.h"

namespace csgo_parsing::AssetFinder {

    // Try to find installation directory of CSGO and make the result available
    // through AssetFinder::GetCsgoPath(). This method clears results of
    // previous AssetFinder::RefreshMapFileList() and
    // AssetFinder::RefreshVpkArchiveIndex() calls. If CSGO's path was found, it
    // will be used by AssetFinder::ExistsInGameFiles(),
    // AssetFinder::RefreshMapFileList(), AssetFinder::RefreshVpkArchiveIndex()
    // and AssetFileReader::OpenFileFromGameFiles().
    // The code of the returned RetCode object is one of:
    // SUCCESS, STEAM_NOT_INSTALLED, ERROR_STEAM_REGISTRY,
    // ERROR_FILE_OPEN_FAILED, CSGO_NOT_INSTALLED
    utils::RetCode FindCsgoPath();

    // Result of the most recent AssetFinder::FindCsgoPath() call. If CSGO was
    // not found, returns an empty string. Otherwise, returns a path to the
    // "csgo/" game directory, e.g.:
    //   "C:/Steam/steamapps/common/Counter-Strike Global Offensive/csgo/"
    // Returned string is in UTF-8 with forward slash directory separators.
    // NOTE: If you store the reference to this string, be aware its value could
    // change after every call to AssetFinder::FindCsgoPath() !
    const std::string& GetCsgoPath();

    // Clears previous map file list. Then looks up ".bsp" files in the "maps/"
    // dir of the currently detected game directory
    // (i.e. AssetFinder::GetCsgoPath() + "/maps/") and makes their paths, which
    // are relative to that "maps" dir, available through the
    // AssetFinder::GetMapFileList() method.
    void RefreshMapFileList();

    // Returns paths to all currently detected ".bsp" map files, relative to
    // AssetFinder::GetCsgoPath() + "/maps/" .
    // Path strings are in UTF-8 with forward slash directory separators.
    // NOTE: If you store the reference to this list, be aware its contents
    // could change after every call to AssetFinder::FindCsgoPath() and
    // AssetFinder::RefreshMapFileList() !
    const std::vector<std::string>& GetMapFileList();

    // Clears previous VPK indexing results (making previously indexed files
    // unavailable). Then it indexes files that are contained in VPK archives
    // from the currently detected game directory
    // ( AssetFinder::GetCsgoPath() + "/pak01_*.vpk" ). Indexed files can then
    // be found by AssetFinder::ExistsInGameFiles() and opened by
    // AssetFileReader::OpenFileFromGameFiles().
    // 
    // @param file_ext_filter A list of file extensions (e.g. "mdl", "phy"). All
    //                        files from the VPK archive with one of those
    //                        extensions will be indexed (this param is for
    //                        optimization). Passing an empty list disables
    //                        filtering and makes all contained files available.
    // @return The code of the returned RetCode object is one of:
    //         SUCCESS, ERROR_VPK_PARSING_FAILED
    utils::RetCode RefreshVpkArchiveIndex(
        const std::vector<std::string>& file_ext_filter = {});

    // Checks if file at path (e.g. "models/props/cs_militia/ladderwood.mdl")
    // exists under the currently detected game directory
    // ( AssetFinder::GetCsgoPath() ) or if it was indexed by a call to
    // AssetFinder::RefreshVpkArchiveIndex().
    // NOTE: Files for which this is true can be read using
    //       AssetFileReader::OpenFileFromGameFiles().
    // NOTE: This function does not consider the files packed inside ".bsp" map
    //       files!
    bool ExistsInGameFiles(const std::string& file_path);

}

#endif // CSGO_PARSING_ASSETFINDER_H_
