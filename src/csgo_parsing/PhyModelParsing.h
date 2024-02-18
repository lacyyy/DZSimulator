#ifndef CSGO_PARSING_PHYMODELPARSING_H_
#define CSGO_PARSING_PHYMODELPARSING_H_

#include <limits>
#include <string>
#include <vector>

#include <Magnum/Magnum.h>

#include "csgo_parsing/AssetFileReader.h"
#include "csgo_parsing/utils.h"
#include "utils_3d.h"

namespace csgo_parsing {

    // Parse the collision model from a PHY file that only has a single solid.
    // Attempting to parse a PHY file with multiple solids fails with the code
    // ERROR_PHY_MULTIPLE_SOLIDS.
    // @param dest_sections If parsing is successful, a number of sections are
    //                      put in the std::vector pointed to by dest_sections.
    //                      Each section is a convex shape described by a
    //                      triangle mesh.
    //                      CAUTION: CSGO's PHY models might have *slightly*
    //                               concave sections! Effects of this are unknown.
    //                      Properties of returned TriMesh objects:
    //                      - 'edges' array holds unique edges (GUARANTEED)
    //                      - 'tris' array likely holds unique tris (not guaranteed)
    //                      - 'vertices' array likely holds unique verts (not guaranteed)
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
    // @return Code of returned RetCode is one of:
    //         - SUCCESS (no description)
    //         - ERROR_PHY_MULTIPLE_SOLIDS (no description, PHY file had more than 1 solid)
    //         - ERROR_PHY_PARSING_FAILED (something else failed, has description)
    utils::RetCode ParseSingleSolidPhyModel(
        std::vector<utils_3d::TriMesh>* dest_sections,
        std::string* dest_surfaceprop,
        AssetFileReader& opened_reader,
        size_t max_byte_read_count = std::numeric_limits<size_t>::max(),
        bool include_shrink_wrap_shape = false);

}

#endif // CSGO_PARSING_PHYMODELPARSING_H_
