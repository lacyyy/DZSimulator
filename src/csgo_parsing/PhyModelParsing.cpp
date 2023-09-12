#include "csgo_parsing/PhyModelParsing.h"

#include <utility>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "csgo_parsing/AssetFileReader.h"

using namespace csgo_parsing;
using namespace Magnum;

// In CSGO's coordinate system, 1 unit is 1 inch (0.0254 meters)
// Parsed collision models are scaled using the following factor:
#define VPHY_MODEL_SCALE (1.0f / 0.0254f)

/* How we read ".phy" files. Integer and float fields are read in little-endian.
 * Notation:  <FIELD_BYTE_SIZE> - <FIELD_DESCRIPTION>
 * 
 * -- PHY header
 *     4 - size of this header (must be 16)
 *     4 - unused
 *     4 - solid count (we require exactly 1 solid, cuz we only read phy models of static props so far)
 *     4 - unused
 * 
 * -- surface header
 *     4 - size of following binary geometry data until text section
 *     4 - id - "VPHY" - decimal 1497911382 (marks "new" PHY file format?)
 *     68 - unused
 *     4 - ascii code, we expect "IVPS"
 * 
 *     -- unknown amount of section headers (a convex mesh each)
 *         4 - offset to vertices, from current position
 *         4 - unused
 *         4 - flags - Section is shrink wrap shape with (flags & 0x0001)
 *         4 - count of triangle faces following after this
 * 
 *         -- triangle face
 *             1 - face id - starts counting at 0 for each new section, overflows if more than 256 faces!
 *             3 - unused
 *             2 - vertex1 idx
 *             2 - unused
 *             2 - vertex2 idx
 *             2 - unused
 *             2 - vertex3 idx
 *             2 - unused
 * 
 *     -- (possibly) second section header
 *         ...
 * 
 *     --  (max_idx_in_all_triangle_faces + 1) vertices
 *         4 - float, vertex X position
 *         4 - float, vertex Y position
 *         4 - float, vertex Z position
 *         4 - unused
 * 
 * 
 *     -- ??? some more unknown data until text section
 * 
 * -- text section, ascii (we extract just the surfaceprop value), e.g.:
 * 
 * solid {
 * "index" "0"
 * "name" "standing_rock_2_physbox"
 * "mass" "45768.667969"
 * "surfaceprop" "rock"
 * "damping" "0.000000"
 * "rotdamping" "0.000000"
 * "inertia" "1.000000"
 * "volume" "1163739.875000"
 * }
 * editparams {
 * "rootname" ""
 * "totalmass" "45768.667969"
 * "concave" "1"
 * }
 * 
 */

utils::RetCode
csgo_parsing::ParsePhyModel(
    std::vector<std::vector<Magnum::Vector3>>* dest_tris,
    std::string* dest_surfaceprop,
    AssetFileReader& opened_reader,
    size_t max_byte_read_count,
    bool include_shrink_wrap_shape)
{
    auto p_err = utils::RetCode::ERROR_PHY_PARSING_FAILED; // parsing error code
    std::string invalid_file_msg = "Invalid PHY file, ";
    std::string read_error_msg = "PHY file read error";

    if (!opened_reader.IsOpenedInFile())
        return { p_err, "PHY reader not opened" };

    // Abort once reader would reach max_reader_pos
    size_t max_reader_pos = std::numeric_limits<size_t>::max();
    if (max_byte_read_count != std::numeric_limits<size_t>::max())
        max_reader_pos = opened_reader.GetPos() + max_byte_read_count;

    const size_t PHY_HEADER_SIZE = 96; // Number of bytes until sections data
    if (opened_reader.GetPos() + PHY_HEADER_SIZE > max_reader_pos)
        return { p_err, invalid_file_msg + "smaller than PHY header alone" };


    // Read PHY header
    uint8_t unused[68];
    uint32_t header_size = 0;
    uint32_t solid_count = 0;
    if (0
        || !opened_reader.ReadUINT32_LE(header_size)
        || !opened_reader.ReadByteArray(unused, 4)
        || !opened_reader.ReadUINT32_LE(solid_count)
        || !opened_reader.ReadByteArray(unused, 4))
        return { p_err, read_error_msg };

    if (header_size != 16)
        return { p_err, invalid_file_msg + "header size = " + std::to_string(header_size) };

    if (solid_count != 1)
        return { p_err, invalid_file_msg + "solid count = " + std::to_string(solid_count) };

    uint32_t binary_data_size; // size of following data until text section
    if (!opened_reader.ReadUINT32_LE(binary_data_size))
        return { p_err, read_error_msg };
    size_t text_section_pos = opened_reader.GetPos() + binary_data_size;

    if (text_section_pos > max_reader_pos)
        return { p_err, invalid_file_msg + "text_section_pos > max_reader_pos" };

    uint32_t vphysics_id;
    uint32_t ivps_field;
    if (0
        || !opened_reader.ReadUINT32_LE(vphysics_id)
        || !opened_reader.ReadByteArray(unused, 68)
        || !opened_reader.ReadUINT32_LE(ivps_field))
        return { p_err, read_error_msg };

    if (vphysics_id != ('V' | 'P' << 8 | 'H' << 16 | 'Y' << 24))
        return { p_err, invalid_file_msg + "vphysics_id = " + std::to_string(vphysics_id) };
    
    if (ivps_field != ('I' | 'V' << 8 | 'P' << 16 | 'S' << 24))
        return { p_err, invalid_file_msg + "ivps_field = " + std::to_string(ivps_field) };

    int32_t highest_vertex_idx = -1; // Highest triangle vertex idx we read
    size_t vertices_start_pos = 0;
    // Marker that disallows reading sections beyond this position
    size_t sections_end_pos = text_section_pos;

    // Each section consists of a list of uint16_t vertex indices, always 3
    // indices per triangle. E.g.: v1_idx, v2_idx, v3_idx, v1_idx, v2_idx, ...
    std::vector<std::vector<uint16_t>> sections;

    const size_t SECTION_HEADER_SIZE = 16;
    const size_t TRIANGLE_SIZE = 16;
    const size_t VERTEX_SIZE = 16;

    // Read an unknown amount of sections
    // A section is a collection of triangles that describe a convex shape
    while (opened_reader.GetPos() + SECTION_HEADER_SIZE <= sections_end_pos) {
        if (sections.size() > 16000) // Enforce some arbitrary, but sane limit
            return { p_err, "Too complex model, section limit reached" };

        size_t section_start_pos = opened_reader.GetPos();
        uint32_t offset_to_vertices_start; // offset from section start pos
        uint32_t flags;
        uint32_t triangle_count;
        if (0
            || !opened_reader.ReadUINT32_LE(offset_to_vertices_start)
            || !opened_reader.ReadByteArray(unused, 4)
            || !opened_reader.ReadUINT32_LE(flags)
            || !opened_reader.ReadUINT32_LE(triangle_count))
            return { p_err, read_error_msg };

        // Some PHY files have a section with this flag set. That section
        // describes the smallest possible convex shape that encompasses the
        // entire collision model, like shrinkwrapping it.
        // This section with this flag is probably not present for collision
        // models that are already convex and can be described entirely by a
        // single section.
        bool is_shrink_wrap_section = flags & 0x0001;
        bool ignore_section = is_shrink_wrap_section && !include_shrink_wrap_shape;

        vertices_start_pos = section_start_pos + offset_to_vertices_start;

        if (vertices_start_pos > text_section_pos)
            return { p_err, invalid_file_msg + "vertices_start_pos > text_section_pos" };

        sections_end_pos = vertices_start_pos; // Stop reading sections at vertices

        if (opened_reader.GetPos() + triangle_count * TRIANGLE_SIZE > sections_end_pos)
            return { p_err, invalid_file_msg + "invalid section header" };

        if (triangle_count > 128000) // Enforce some arbitrary, but sane limit
            return { p_err, "Too complex model, triangle limit reached" };

        std::vector<uint16_t> cur_section; // list of triangle vertex indices
        if(!ignore_section)
            cur_section.reserve(triangle_count * 3); // 3 indices per triangle
        
        for (size_t tri_idx = 0; tri_idx < triangle_count; tri_idx++) {
            uint16_t v1_idx, v2_idx, v3_idx;
            if (0
                || !opened_reader.ReadByteArray(unused, 4)
                || !opened_reader.ReadUINT16_LE(v1_idx)
                || !opened_reader.ReadByteArray(unused, 2)
                || !opened_reader.ReadUINT16_LE(v2_idx)
                || !opened_reader.ReadByteArray(unused, 2)
                || !opened_reader.ReadUINT16_LE(v3_idx)
                || !opened_reader.ReadByteArray(unused, 2))
                return { p_err, read_error_msg };

            if (ignore_section)
                continue;
            cur_section.push_back(v1_idx);
            cur_section.push_back(v2_idx);
            cur_section.push_back(v3_idx);
            if (v1_idx > highest_vertex_idx) highest_vertex_idx = v1_idx;
            if (v2_idx > highest_vertex_idx) highest_vertex_idx = v2_idx;
            if (v3_idx > highest_vertex_idx) highest_vertex_idx = v3_idx;
        }

        if (!ignore_section)
            sections.emplace_back(std::move(cur_section));
    }
    
    std::vector<Magnum::Vector3> vertices;

    if (highest_vertex_idx >= 0) { // If any vertex indices were read
        int32_t num_vertices = highest_vertex_idx + 1;
        if (vertices_start_pos + num_vertices * VERTEX_SIZE > text_section_pos)
            return { p_err, invalid_file_msg + "vertices_end_pos > text_section_pos" };
        if (num_vertices > 256000) // Enforce some arbitrary, but sane limit
            return { p_err, "Too complex model, vertex limit reached" };

        if (opened_reader.GetPos() != vertices_start_pos)
            if (!opened_reader.SetPos(vertices_start_pos))
                return { p_err, read_error_msg + ", seek to vertices failed" };

        vertices.reserve(num_vertices);

        for (size_t vert_idx = 0; vert_idx < num_vertices; vert_idx++) {
            float vert_x, vert_y, vert_z;
            if (0
                || !opened_reader.ReadFLOAT32_LE(vert_x)
                || !opened_reader.ReadFLOAT32_LE(vert_y)
                || !opened_reader.ReadFLOAT32_LE(vert_z)
                || !opened_reader.ReadByteArray(unused, 4))
                return { p_err, read_error_msg };

            // Swap Y and Z axis and invert vertical axis for valid vertex
            // positions in CSGO's coordinate system. Additionally, scale the
            // model to appear in CSGO at the right size.
            vertices.emplace_back(
                VPHY_MODEL_SCALE * vert_x,
                VPHY_MODEL_SCALE * vert_z,
                VPHY_MODEL_SCALE * -vert_y);
        }
    }

    if (!opened_reader.SetPos(text_section_pos))
        return { p_err, read_error_msg + ", seek to text section failed" };

    std::string surface_properties = "";
    bool in_solid_block = false;
    std::string line;
    while (1) {
        // Determine how many more bytes we are allowed to read
        size_t max_bytes_left = max_reader_pos - opened_reader.GetPos();

        // Abort if read error occurs or line is too long
        if (!opened_reader.ReadLine(line, '\n', max_bytes_left))
            break;

        if (line.compare("solid {") == 0) { in_solid_block =  true; continue; }
        if (line.compare("}")       == 0) { in_solid_block = false; continue; }
        if (in_solid_block) {
            if (line.length() >= 16 && line[line.length() - 1] == '\"') {
                if (line.starts_with("\"surfaceprop\" \"")) {
                    surface_properties = line.substr(15, line.size() - 16);
                    break;
                }
            }
        }
    }

    if (dest_surfaceprop)
        *dest_surfaceprop = std::move(surface_properties);

    if (dest_tris) {
        dest_tris->clear();

        size_t total_tri_count = 0;
        for (const auto& section : sections)
            total_tri_count += section.size() / 3;
        dest_tris->reserve(total_tri_count);

        for (const auto& section : sections) {
            for (size_t i = 0; i < section.size(); i += 3) {
                // Invert triangle order to get triangles with clockwise vertex winding
                dest_tris->push_back({
                    vertices[section[i]],
                    vertices[section[i+2]],
                    vertices[section[i+1]]});
            }
        }
    }

    return { utils::RetCode::SUCCESS };
}
