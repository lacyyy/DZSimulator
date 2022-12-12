#ifndef CSGO_PARSING_PHYMODELPARSING_H_
#define CSGO_PARSING_PHYMODELPARSING_H_

#include <limits>
#include <string>
#include <vector>

#include <Magnum/Magnum.h>

#include "csgo_parsing/AssetFileReader.h"
#include "csgo_parsing/utils.h"

namespace csgo_parsing {

    // @param dest_tris If parsing is successful, list of triangles are appended
    //                  to the std::vector pointed to by dest_tris. Triangles
    //                  have clockwise vertex winding.
    // @param dest_surfaceprop If parsing is successful, the PHY model's surface
    //                         property string is written into the std::string
    //                         pointed to by dest_surfaceprop.
    // @param opened_reader The data will be parsed from the file and position the
    //                      given reader object is currently opened in. If it
    //                      is not opened in any file, this function fails.
    // @param max_byte_read_count Maximum number of bytes this function is allowed
    //                            to read starting from the current position of the
    //                            given reader.
    // @param include_shrink_wrap_shape Optional extra convex section added to the list.
    // @return Code of returned RetCode is either SUCCESS or ERROR_PHY_PARSING_FAILED.
    //         ERROR_PHY_PARSING_FAILED has an error description.
    utils::RetCode ParsePhyModel(
        std::vector<std::vector<Magnum::Vector3>>* dest_tris,
        std::string* dest_surfaceprop,
        AssetFileReader& opened_reader,
        size_t max_byte_read_count = std::numeric_limits<size_t>::max(),
        bool include_shrink_wrap_shape = false);

}

#endif // CSGO_PARSING_PHYMODELPARSING_H_
