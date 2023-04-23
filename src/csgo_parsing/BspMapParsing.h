#ifndef CSGO_PARSING_BSPMAPPARSING_H_
#define CSGO_PARSING_BSPMAPPARSING_H_

#include <memory>
#include <string>

#include <Corrade/Containers/ArrayView.h>

#include "csgo_parsing/BspMap.h"
#include "csgo_parsing/utils.h"

namespace csgo_parsing {

    // Parse a ".bsp" CSGO map file from an absolute file path that is allowed
    // to contain UTF-8 Unicode chars.
    // 
    // If parsing fails, the returned code is ERROR_BSP_PARSING_FAILED with an
    // error description and an empty shared_ptr is put where
    // dest_parsed_bsp_map points to.
    // 
    // If parsing succeeds, the returned code is SUCCESS (possibly with a
    // warning msg) and a shared_ptr managing the newly parsed BspMap object is
    // put where dest_parsed_bsp_map points to.
    utils::RetCode ParseBspMapFile(std::shared_ptr<BspMap>* dest_parsed_bsp_map,
        const std::string& abs_bsp_file_path);

    // Parse a ".bsp" CSGO map file from a memory block containing the content
    // of a ".bsp" file.
    // 
    // If parsing fails, the returned code is ERROR_BSP_PARSING_FAILED with an
    // error description and an empty shared_ptr is put where
    // dest_parsed_bsp_map points to.
    // 
    // If parsing succeeds, the returned code is SUCCESS (possibly with a
    // warning msg) and a shared_ptr managing the newly parsed BspMap object is
    // put where dest_parsed_bsp_map points to.
    utils::RetCode ParseBspMapFile(std::shared_ptr<BspMap>* dest_parsed_bsp_map,
        Corrade::Containers::ArrayView<const uint8_t> bsp_file_content);
}

#endif // CSGO_PARSING_BSPMAPPARSING_H_
