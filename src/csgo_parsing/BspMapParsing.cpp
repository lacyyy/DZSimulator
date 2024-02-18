#include "csgo_parsing/BspMapParsing.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <Tracy.hpp>

#include <Corrade/Utility/Debug.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

#include "csgo_parsing/AssetFileReader.h"
#include "csgo_parsing/BspMapLumps.h"
#include "csgo_parsing/utils.h"

using namespace csgo_parsing;
using namespace Magnum;
using namespace Corrade;

bool ParseEntity_worldspawn(std::multimap<std::string, std::string>& key_values, BspMap& in_out)
{
    std::multimap<std::string, std::string>::iterator kv;

    kv = key_values.find("world_mins");
    if (kv != key_values.end()) {
        std::vector<float> num_list = utils::ParseFloatsFromString(kv->second);
        if (num_list.size() < 3) return false;
        in_out.world_mins = { num_list[0], num_list[1], num_list[2] };
    }

    kv = key_values.find("world_maxs");
    if (kv != key_values.end()) {
        std::vector<float> num_list = utils::ParseFloatsFromString(kv->second);
        if (num_list.size() < 3) return false;
        in_out.world_maxs = { num_list[0], num_list[1], num_list[2] };
    }

    kv = key_values.find("mapversion");
    if (kv != key_values.end()) {
        std::vector<int64_t> num_list = utils::ParseIntsFromString(kv->second);
        if (num_list.size() >= 1)
            in_out.map_version = num_list[0];
    }

    kv = key_values.find("skyname");
    if (kv != key_values.end())
        in_out.sky_name = kv->second;

    kv = key_values.find("detailmaterial");
    if (kv != key_values.end())
        in_out.detail_material = kv->second;

    kv = key_values.find("detailvbsp");
    if (kv != key_values.end())
        in_out.detail_vbsp = kv->second;

    return true;
}

bool ParseEntity_info_player_counterterrorist(std::multimap<std::string, std::string>& key_values, BspMap& in_out)
{
    std::multimap<std::string, std::string>::iterator kv;

    kv = key_values.find("enabled");
    if (kv != key_values.end()) {
        int64_t enabled = utils::ParseIntFromString(kv->second, 0);
        if (enabled == 0)
            return true; // Skip this spawn as it's disabled
    }

    BspMap::PlayerSpawn p_spawn;

    kv = key_values.find("origin");
    if (kv == key_values.end()) return false; // must have origin
    std::vector<float> origin_vals = utils::ParseFloatsFromString(kv->second);
    if (origin_vals.size() < 3) return false;
    p_spawn.origin = { origin_vals[0], origin_vals[1], origin_vals[2] };

    kv = key_values.find("angles");
    if (kv == key_values.end()) return false; // must have angles
    std::vector<float> angle_vals = utils::ParseFloatsFromString(kv->second);
    if (angle_vals.size() < 3) return false;
    p_spawn.angles = { angle_vals[0], angle_vals[1], angle_vals[2] };

    kv = key_values.find("priority");
    if (kv != key_values.end()) p_spawn.priority = utils::ParseIntFromString(kv->second, 0);
    else                        p_spawn.priority = 0;

    // Find position of new player spawn in the sorted player spawn list
    auto insert_pos = in_out.player_spawns.end();
    for (auto it = in_out.player_spawns.begin(); it != in_out.player_spawns.end(); ++it) {
        if (p_spawn.priority < it->priority) {
            insert_pos = it;
            break;
        }
    }
    // Insert new player spawn
    in_out.player_spawns.insert(insert_pos, std::move(p_spawn));
    return true;
}

bool ParseEntity_info_player_terrorist(std::multimap<std::string, std::string>& key_values, BspMap& in_out)
{
    return ParseEntity_info_player_counterterrorist(key_values, in_out); // We don't differentiate between t and ct spawns
}

bool ParseEntity_func_brush(std::multimap<std::string, std::string>& key_values, BspMap& in_out)
{
    BspMap::Ent_func_brush fb;
    std::multimap<std::string, std::string>::iterator kv;

    kv = key_values.find("model");
    if (kv != key_values.end()) fb.model = kv->second;
    else                        fb.model = "";

    kv = key_values.find("origin");
    if (kv == key_values.end()) return false; // func_brush must have 'origin' KeyValue
    std::vector<float> origin_vals = utils::ParseFloatsFromString(kv->second);
    if (origin_vals.size() < 3) return false;
    fb.origin = { origin_vals[0], origin_vals[1], origin_vals[2] };

    kv = key_values.find("angles");
    if (kv != key_values.end()) {
        std::vector<float> angle_vals = utils::ParseFloatsFromString(kv->second);
        if (angle_vals.size() < 3) return false;
        fb.angles = { angle_vals[0], angle_vals[1], angle_vals[2] };
    }
    else {
        fb.angles = { 0.0f, 0.0f, 0.0f };
    }

    kv = key_values.find("Solidity");
    if (kv != key_values.end()) fb.solidity = utils::ParseIntFromString(kv->second, 1); // default value: 1 = Never Solid
    else                        fb.solidity = 0; // Solidity toggled with visibility

    kv = key_values.find("StartDisabled");
    if (kv != key_values.end()) fb.start_disabled = utils::ParseIntFromString(kv->second, 0) == 1;
    else                        fb.start_disabled = false;

    in_out.entities_func_brush.push_back(std::move(fb));

    return true;
}

bool ParseEntity_trigger_push(std::multimap<std::string, std::string>& key_values, BspMap& in_out)
{
    BspMap::Ent_trigger_push tp;
    std::multimap<std::string, std::string>::iterator kv;

    kv = key_values.find("model");
    if (kv != key_values.end()) tp.model = kv->second;
    else                        tp.model = "";

    kv = key_values.find("origin");
    if (kv == key_values.end()) return false; // 'origin' is required
    std::vector<float> origin_vals = utils::ParseFloatsFromString(kv->second);
    if (origin_vals.size() < 3) return false;
    tp.origin = { origin_vals[0], origin_vals[1], origin_vals[2] };

    kv = key_values.find("pushdir");
    if (kv == key_values.end()) return false;  // 'pushdir' is required
    std::vector<float> pushdir_vals = utils::ParseFloatsFromString(kv->second);
    if (pushdir_vals.size() < 3) return false;
    tp.pushdir = { pushdir_vals[0], pushdir_vals[1], pushdir_vals[2] };

    kv = key_values.find("angles");
    if (kv != key_values.end()) {
        std::vector<float> angle_vals = utils::ParseFloatsFromString(kv->second);
        if (angle_vals.size() < 3) return false;
        tp.angles = { angle_vals[0], angle_vals[1], angle_vals[2] };
    }
    else {
        tp.angles = { 0.0f, 0.0f, 0.0f };
    }

    kv = key_values.find("speed");
    if (kv == key_values.end()) return false; // 'speed' is required
    tp.speed = utils::ParseFloatFromString(kv->second, -1.0f);

    kv = key_values.find("spawnflags");
    if (kv != key_values.end()) tp.spawnflags = utils::ParseIntFromString(kv->second, 0);
    else                        tp.spawnflags = 0;

    kv = key_values.find("StartDisabled");
    if (kv != key_values.end()) tp.start_disabled = utils::ParseIntFromString(kv->second, 0) != 0;
    else                        tp.start_disabled = false;

    kv = key_values.find("OnlyFallingPlayers");
    if (kv != key_values.end()) tp.only_falling_players = utils::ParseIntFromString(kv->second, 0) != 0;
    else                        tp.only_falling_players = false;

    kv = key_values.find("FallingSpeedThreshold");
    if (kv != key_values.end()) tp.falling_speed_threshold = utils::ParseFloatFromString(kv->second, 0.0f);
    else                        tp.falling_speed_threshold = 0.0f;

    // There's also an "alternateticksfix" KeyValue. We ignore it as it's only
    // relevant when sv_alternateticks is set to 1.

    in_out.entities_trigger_push.push_back(std::move(tp));
    return true;
}

bool ParseEntity_prop_dynamic(std::multimap<std::string, std::string>& key_values, BspMap& in_out)
{
    // NOTE: We don't collect every prop_dynamic here. Only a few are selected
    //       based on whether they're relevant (worth visualizing).

    BspMap::Ent_prop_dynamic dp;
    std::multimap<std::string, std::string>::iterator kv;

    kv = key_values.find("solid");
    if (kv == key_values.end()) return false; // 'solid' is required
    int64_t solid = utils::ParseIntFromString(kv->second, 0);
    if (solid != 6) { // Skip this prop_dynamic if it doesn't have SOLID_VPHYSICS
        // We skip all non-vphysics dynamic props. They might have these
        // solid values:
        // 0 (SOLID_NONE), not solid
        // 2 (SOLID_BBOX), AABB is solid. Rarely used in maps -> ignore
        // 4 (SOLID_OBB_YAW), oriented bbox is solid. Rarely used in maps -> ignore
        return true;
    }

    kv = key_values.find("spawnflags");
    int64_t spawnflags;
    if (kv == key_values.end()) spawnflags = 0;
    else                        spawnflags = utils::ParseIntFromString(kv->second, 0);
    // Skip this prop_dynamic if it has the 'Start with collision disabled' flag
    if (spawnflags & 256) return true;

    if (key_values.find("parentname") != key_values.end()) {
        // NOTE: Sometimes, a prop_dynamic has its 'parentname' property set.
        //       This is usually done for non-solid or moving objects.
        //       Skip them because we only care about solid and static objects.
        return true;
    }

    kv = key_values.find("model");
    if (kv == key_values.end()) return true; // No model, skip this prop_dynamic
    dp.model = utils::NormalizeGameFilePath(kv->second);

    kv = key_values.find("origin");
    if (kv == key_values.end()) return false; // 'origin' is required
    std::vector<float> origin_vals = utils::ParseFloatsFromString(kv->second);
    if (origin_vals.size() < 3) return false;
    dp.origin = { origin_vals[0], origin_vals[1], origin_vals[2] };

    kv = key_values.find("angles");
    if (kv != key_values.end()) {
        std::vector<float> angle_vals = utils::ParseFloatsFromString(kv->second);
        if (angle_vals.size() < 3) return false;
        dp.angles = { angle_vals[0], angle_vals[1], angle_vals[2] };
    }
    else {
        dp.angles = { 0.0f, 0.0f, 0.0f };
    }

    // NOTE: We don't check the 'modelscale' property because it only affects
    //       a prop_dynamic's visible model, not its collisions!
    // NOTE: We don't check the 'StartDisabled' property because it only affects
    //       a prop_dynamic's visible model, not its collisions!
    // NOTE: A prop_dynamic does not appear in CSGO if its model doesn't have
    //       'dynamic' support. We don't check this because we assume that map
    //       creators have set correct models.

    in_out.relevant_dynamic_props.push_back(std::move(dp));
    return true;
}

bool ParseEntity(std::multimap<std::string, std::string>& key_values, BspMap& in_out)
{
    auto it = key_values.find("classname");
    if (it == key_values.end()) // Every entity must have a classname
        return false;
    std::string& cn = it->second;

    if (0
        || (cn == "worldspawn"                   && !ParseEntity_worldspawn                  (key_values, in_out))
        || (cn == "info_player_terrorist"        && !ParseEntity_info_player_terrorist       (key_values, in_out))
        || (cn == "info_player_counterterrorist" && !ParseEntity_info_player_counterterrorist(key_values, in_out))
        || (cn == "func_brush"                   && !ParseEntity_func_brush                  (key_values, in_out))
        || (cn == "trigger_push"                 && !ParseEntity_trigger_push                (key_values, in_out))
        || (cn == "prop_dynamic"                 && !ParseEntity_prop_dynamic                (key_values, in_out))
        || (cn == "prop_dynamic_override"        && !ParseEntity_prop_dynamic                (key_values, in_out)))
        return false;

    return true;
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Entities(AssetFileReader& fr, BspMap& in_out)
{
    // Lump length is the exact length of the entity string including the NULL-terminator at the end
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_ENTITIES].file_len;
    size_t lump_pos = fr.GetPos();

    // Each line contains a KeyValue with format:
    // "KEY" "VALUE"
    const size_t MAX_LINE_LEN = BspMap::MAX_ENTITY_KEY_LEN + BspMap::MAX_ENTITY_VALUE_LEN + 5; // 5 extra: 4x '"' and 1x ' '

    bool parsing_key_values = false; // false -> looking for '{'; true -> parsing KeyValues
    std::multimap<std::string, std::string> key_values;

    std::string line;
    line.reserve(MAX_LINE_LEN);

    bool success = false;
    while (1) {
        size_t remaining_chars = lump_pos + lump_len - fr.GetPos(); // Including line-feeds and NULL terminator
        // 1 or 3 remaining_chars is valid, not 0 or 2: ['}','\n','\0'] and ['\0']
        if (remaining_chars == 0 || remaining_chars == 2)
            break; // error
        if (remaining_chars == 1) { // Only NULL-terminator should be left
            uint8_t c;
            if (!fr.ReadByteArray(&c, 1))
                break; // error
            if (c != '\0')
                break; // error
            success = true;
            break; // success
        }
                                                                      
        // 2 chars come after the last line: '\n', '\0'
        size_t max_line_length = std::min(MAX_LINE_LEN, remaining_chars - 2);

        line.clear();
        if (!fr.ReadLine(line, '\n', max_line_length + 1))
            break; // error

        if (!parsing_key_values) { // Looking for a new entity beginning, i.e. a '{'
            if (line.length() != 1 || line[0] != '{')
                break; // error
            parsing_key_values = true;
            continue;
        }

        if (line.length() == 1 && line[0] == '}') { // End of entity definition
            if (!ParseEntity(key_values, in_out)) // Parse entity KeyValues
                break;
            key_values.clear(); // Delete KeyValues
            parsing_key_values = false; // Start looking for '{' again
            continue; // Look for next entity entry
        }

        // KeyValue parsing
        if (line.length() < 5 || line[0] != '"' || line[line.length()-1] != '"')
            break; // error
        char* key_start = line.data() + 1;
        // Find end of key string
        char* key_end = key_start; // past-the-end iterator
        char* max_key_end = line.data() + line.length() - 4;
        while (key_end <= max_key_end) {
            if (*key_end == '"')
                break;
            ++key_end;
        }
        if (key_start == key_end || key_end > max_key_end) // empty/invalid key
            break; // error
        // Extract key string
        std::string key(key_start, key_end - key_start);
        // Extract value string
        if (*(key_end + 1) != ' ' || *(key_end + 2) != '"') // invalid if not separated by one space
            break; // error
        char* value_start = key_end + 3;
        char* value_end = line.data() + line.length() - 1; // past-the-end iterator
        std::string value(value_start, value_end - value_start);
        key_values.insert({key, value}); // Insert new KeyValue
    }
    if (!success)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Entity parse error"};
    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Planes(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 20; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_PLANES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t plane_cnt = lump_len / STRUCT_SIZE;

    if (plane_cnt > BspMap::MAX_PLANES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many planes: " + std::to_string(plane_cnt) };

    in_out.planes.reserve(plane_cnt);

    uint8_t unused[STRUCT_SIZE];
    while (plane_cnt--) {
        BspMap::Plane p;
        if (0
            || !fr.ReadFLOAT32_LE(p.normal.x())
            || !fr.ReadFLOAT32_LE(p.normal.y())
            || !fr.ReadFLOAT32_LE(p.normal.z())
            || !fr.ReadFLOAT32_LE(p.dist)
            || !fr.ReadByteArray(unused, 4))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.planes.push_back(std::move(p));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Vertexes(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 12; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_VERTEXES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t vertex_cnt = lump_len / STRUCT_SIZE;

    if (vertex_cnt > BspMap::MAX_VERTICES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many vertices: " + std::to_string(vertex_cnt) };

    in_out.vertices.reserve(vertex_cnt);

    while (vertex_cnt--) {
        Vector3 vec;
        if (0
            || !fr.ReadFLOAT32_LE(vec.x())
            || !fr.ReadFLOAT32_LE(vec.y())
            || !fr.ReadFLOAT32_LE(vec.z()))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.vertices.push_back(std::move(vec));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Edges(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 4; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_EDGES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t edge_cnt = lump_len / STRUCT_SIZE;

    if (edge_cnt > BspMap::MAX_EDGES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many edges: " + std::to_string(edge_cnt) };

    in_out.edges.reserve(edge_cnt);

    while (edge_cnt--) {
        BspMap::Edge e;
        if (!fr.ReadUINT16_LE(e.v[0]) || !fr.ReadUINT16_LE(e.v[1]))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.edges.push_back(std::move(e));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_SurfEdges(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 4; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_SURFEDGES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t surfedge_cnt = lump_len / STRUCT_SIZE;

    if (surfedge_cnt > BspMap::MAX_SURFEDGES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many surfedges: " + std::to_string(surfedge_cnt) };

    in_out.surfedges.reserve(surfedge_cnt);

    while (surfedge_cnt--) {
        int32_t se;
        if (!fr.ReadINT32_LE(se))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.surfedges.push_back(se);
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Faces(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 56; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_FACES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t face_cnt = lump_len / STRUCT_SIZE;

    if (face_cnt > BspMap::MAX_FACES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many faces: " + std::to_string(face_cnt) };

    in_out.faces.reserve(face_cnt);

    uint8_t unused[STRUCT_SIZE];
    while (face_cnt--) {
        BspMap::Face face;
        if (0
            || !fr.ReadByteArray(unused, 4)
            || !fr.ReadUINT32_LE(face.first_edge)
            || !fr.ReadUINT16_LE(face.num_edges)
            || !fr.ReadByteArray(unused, 2)
            || !fr.ReadINT16_LE(face.disp_info)
            || !fr.ReadByteArray(unused, 42))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.faces.push_back(std::move(face));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_OriginalFaces(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 56; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_ORIGINALFACES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t origface_cnt = lump_len / STRUCT_SIZE;

    if (origface_cnt > BspMap::MAX_ORIGINALFACES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many origfaces: " + std::to_string(origface_cnt) };

    in_out.origfaces.reserve(origface_cnt);

    uint8_t unused[STRUCT_SIZE];
    while (origface_cnt--) {
        BspMap::OrigFace oface;
        if (0
            || !fr.ReadByteArray(unused, 4)
            || !fr.ReadUINT32_LE(oface.first_edge)
            || !fr.ReadUINT16_LE(oface.num_edges)
            || !fr.ReadByteArray(unused, 2)
            || !fr.ReadINT16_LE(oface.disp_info)
            || !fr.ReadByteArray(unused, 42))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.origfaces.push_back(std::move(oface));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_DispVerts(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 20; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_DISP_VERTS].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t dispvert_cnt = lump_len / STRUCT_SIZE;

    if (dispvert_cnt > BspMap::MAX_DISPVERTS)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many dispverts: " + std::to_string(dispvert_cnt) };

    in_out.dispverts.reserve(dispvert_cnt);

    uint8_t unused[STRUCT_SIZE];
    while (dispvert_cnt--) {
        BspMap::DispVert dv;
        if (0
            || !fr.ReadFLOAT32_LE(dv.vec.x())
            || !fr.ReadFLOAT32_LE(dv.vec.y())
            || !fr.ReadFLOAT32_LE(dv.vec.z())
            || !fr.ReadFLOAT32_LE(dv.dist)
            || !fr.ReadByteArray(unused, 4))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.dispverts.push_back(std::move(dv));
    }
    
    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_DispTris(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 2; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_DISP_TRIS].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t disptris_cnt = lump_len / STRUCT_SIZE;

    if (disptris_cnt > BspMap::MAX_DISPTRIS)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many disptris: " + std::to_string(disptris_cnt) };

    in_out.disptris.reserve(disptris_cnt);

    while (disptris_cnt--) {
        BspMap::DispTri dt;
        if (!fr.ReadUINT16_LE(dt.tags))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.disptris.push_back(std::move(dt));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_DispInfos(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 176; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_DISPINFO].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t dispinfo_cnt = lump_len / STRUCT_SIZE;

    if (dispinfo_cnt > BspMap::MAX_DISPINFOS)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many dispinfos: " + std::to_string(dispinfo_cnt) };

    in_out.dispinfos.reserve(dispinfo_cnt);

    uint8_t unused[STRUCT_SIZE];
    while (dispinfo_cnt--) {
        BspMap::DispInfo dinfo;
        if (0
            || !fr.ReadFLOAT32_LE(dinfo.start_pos.x())
            || !fr.ReadFLOAT32_LE(dinfo.start_pos.y())
            || !fr.ReadFLOAT32_LE(dinfo.start_pos.z())
            || !fr.ReadUINT32_LE(dinfo.disp_vert_start)
            || !fr.ReadUINT32_LE(dinfo.disp_tri_start)
            || !fr.ReadUINT32_LE(dinfo.power)
            || !fr.ReadUINT32_LE(dinfo.flags) // Valve Dev Community page says this is "minTess", but seems to be flags instead
            || !fr.ReadByteArray(unused, 8)
            || !fr.ReadUINT16_LE(dinfo.map_face)
            || !fr.ReadByteArray(unused, 138))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };


        if (dinfo.power < BspMap::MIN_DISP_POWER || dinfo.power > BspMap::MAX_DISP_POWER)
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
                "Invalid dispinfo.power: " + std::to_string(dinfo.power) };

        in_out.dispinfos.push_back(std::move(dinfo));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_TexInfos(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 72; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_TEXINFO].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t texinfo_cnt = lump_len / STRUCT_SIZE;

    if (texinfo_cnt > BspMap::MAX_TEXINFOS)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many texinfos: " + std::to_string(texinfo_cnt) };

    in_out.texinfos.reserve(texinfo_cnt);

    uint8_t unused[STRUCT_SIZE];
    while (texinfo_cnt--) {
        BspMap::TexInfo ti;
        if (0
            || !fr.ReadByteArray(unused, 64)
            || !fr.ReadUINT32_LE(ti.flags)
            || !fr.ReadUINT32_LE(ti.texdata))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.texinfos.push_back(std::move(ti));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_TexDatas(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 32; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_TEXDATA].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t texdata_cnt = lump_len / STRUCT_SIZE;

    if (texdata_cnt > BspMap::MAX_TEXDATAS)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many texdatas: " + std::to_string(texdata_cnt) };

    in_out.texdatas.reserve(texdata_cnt);

    uint8_t unused[STRUCT_SIZE];
    while (texdata_cnt--) {
        BspMap::TexData td;
        if (0
            || !fr.ReadByteArray(unused, 12)
            || !fr.ReadUINT32_LE(td.name_string_table_id)
            || !fr.ReadByteArray(unused, 16))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.texdatas.push_back(std::move(td));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_TexDataStringTable(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 4; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_TEXDATA_STRING_TABLE].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t stringelem_cnt = lump_len / STRUCT_SIZE;

    if (stringelem_cnt > BspMap::MAX_TEXDATA_STRING_TABLE_ENTRIES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many stringelems: " + std::to_string(stringelem_cnt) };

    in_out.texdatastringtable.reserve(stringelem_cnt);

    while (stringelem_cnt--) {
        uint32_t offset;
        if (!fr.ReadUINT32_LE(offset))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.texdatastringtable.push_back(offset);
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_TexDataStringData(AssetFileReader& fr, BspMap& in_out)
{
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_TEXDATA_STRING_DATA].file_len;
    if (lump_len == 0) // in_out.texdatastringdata.back() is undefined when vector is empty
        return { utils::RetCode::SUCCESS };
    
    if (lump_len > BspMap::MAX_TEXDATA_STRING_DATA)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Lump is bigger than allowed: " + std::to_string(lump_len) };

    // Init as well to make sure data() returns valid pointer for whole array
    in_out.texdatastringdata = std::vector<char>(lump_len);

    if (!fr.ReadCharArray(in_out.texdatastringdata.data(), lump_len))
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

    if (in_out.texdatastringdata.back() != '\0')
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Lump is not null-terminated!" };

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Brushes(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 12; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_BRUSHES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t brush_cnt = lump_len / STRUCT_SIZE;
    
    if (brush_cnt > BspMap::MAX_BRUSHES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many brushes: " + std::to_string(brush_cnt) };

    in_out.brushes.reserve(brush_cnt);

    while (brush_cnt--) {
        BspMap::Brush brush;
        if (0
            || !fr.ReadUINT32_LE(brush.first_side)
            || !fr.ReadUINT32_LE(brush.num_sides)
            || !fr.ReadUINT32_LE(brush.contents))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.brushes.push_back(std::move(brush));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_BrushSides(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 8; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_BRUSHSIDES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t brushside_cnt = lump_len / STRUCT_SIZE;

    if (brushside_cnt > BspMap::MAX_BRUSHSIDES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many brushsides: " + std::to_string(brushside_cnt) };

    in_out.brushsides.reserve(brushside_cnt);

    while (brushside_cnt--) {
        BspMap::BrushSide bs;
        if (0
            || !fr.ReadUINT16_LE(bs.plane_num)
            || !fr.ReadINT16_LE(bs.texinfo)
            || !fr.ReadINT16_LE(bs.disp_info)
            || !fr.ReadINT16_LE(bs.bevel))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        // The bevel field can take the values 0, 1, 256 and 257 in CSGO maps.
        // The least significant bit is the actual bevel value. The other bit's
        // purpose is unknown, we don't care about it.
        bs.bevel = bs.bevel & 0x0001;

        in_out.brushsides.push_back(std::move(bs));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Nodes(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 32; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_NODES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t node_cnt = lump_len / STRUCT_SIZE;

    if (node_cnt > BspMap::MAX_NODES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many nodes: " + std::to_string(node_cnt) };

    in_out.nodes.reserve(node_cnt);

    uint8_t unused[STRUCT_SIZE];
    while (node_cnt--) {
        BspMap::Node node;
        if (0
            || !fr.ReadByteArray(unused, 4)
            || !fr.ReadINT32_LE(node.children[0])
            || !fr.ReadINT32_LE(node.children[1])
            || !fr.ReadByteArray(unused, 12)
            || !fr.ReadUINT16_LE(node.first_face)
            || !fr.ReadUINT16_LE(node.num_faces)
            || !fr.ReadByteArray(unused, 4))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.nodes.push_back(std::move(node));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Leafs(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 32; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_LEAFS].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t leaf_cnt = lump_len / STRUCT_SIZE;

    if (leaf_cnt > BspMap::MAX_LEAFS)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many leafs: " + std::to_string(leaf_cnt) };

    in_out.leafs.reserve(leaf_cnt);
    
    uint8_t unused[STRUCT_SIZE];
    while (leaf_cnt--) {
        BspMap::Leaf leaf;
        if (0
            || !fr.ReadUINT32_LE(leaf.contents)
            || !fr.ReadByteArray(unused, 16)
            || !fr.ReadUINT16_LE(leaf.first_leaf_face)
            || !fr.ReadUINT16_LE(leaf.num_leaf_faces)
            || !fr.ReadUINT16_LE(leaf.first_leaf_brush)
            || !fr.ReadUINT16_LE(leaf.num_leaf_brushes)
            || !fr.ReadByteArray(unused, 4))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.leafs.push_back(std::move(leaf));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_LeafFaces(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 2; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_LEAFFACES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t leafface_cnt = lump_len / STRUCT_SIZE;

    if (leafface_cnt > BspMap::MAX_LEAFFACES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many leaffaces: " + std::to_string(leafface_cnt) };

    in_out.leaffaces.reserve(leafface_cnt);

    while (leafface_cnt--) {
        uint16_t face_idx;
        if (!fr.ReadUINT16_LE(face_idx))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.leaffaces.push_back(face_idx);
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_LeafBrushes(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 2; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_LEAFBRUSHES].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t leafbrush_cnt = lump_len / STRUCT_SIZE;

    if (leafbrush_cnt > BspMap::MAX_LEAFBRUSHES)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many leafbrushes: " + std::to_string(leafbrush_cnt) };

    in_out.leafbrushes.reserve(leafbrush_cnt);

    while (leafbrush_cnt--) {
        uint16_t brushIndex;
        if (!fr.ReadUINT16_LE(brushIndex))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.leafbrushes.push_back(brushIndex);
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Models(AssetFileReader& fr, BspMap& in_out)
{
    const size_t STRUCT_SIZE = 48; // size in bytes per array element
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_MODELS].file_len;
    if (lump_len % STRUCT_SIZE != 0)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Invalid lump length: " + std::to_string(lump_len) };

    size_t model_cnt = lump_len / STRUCT_SIZE;

    if (model_cnt > BspMap::MAX_MODELS)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many models: " + std::to_string(model_cnt) };

    in_out.models.reserve(model_cnt);

    uint8_t unused[STRUCT_SIZE];
    while (model_cnt--) {
        BspMap::Model model;
        if (0
            || !fr.ReadByteArray(unused, 24)
            || !fr.ReadFLOAT32_LE(model.origin.x())
            || !fr.ReadFLOAT32_LE(model.origin.y())
            || !fr.ReadFLOAT32_LE(model.origin.z())
            || !fr.ReadINT32_LE(model.head_node)
            || !fr.ReadUINT32_LE(model.first_face)
            || !fr.ReadUINT32_LE(model.num_faces))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.models.push_back(std::move(model));
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_GameLump(AssetFileReader& fr, BspMap& in_out)
{
    in_out.static_prop_model_dict.clear();
    in_out.static_prop_leaf_arr.clear();
    in_out.static_props.clear();

    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_GAME_LUMP].file_len;
    if (lump_len < 4)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "lump_len < 4" };

    uint32_t game_lump_count = 0;
    if (!fr.ReadUINT32_LE(game_lump_count))
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };


    uint32_t gl_id = 0;
    uint16_t gl_flags = 0;
    uint16_t gl_version = 0;
    uint32_t gl_fileofs = 0; // offset relative to the beginning of the bsp file
    uint32_t gl_filelen = 0;
    bool static_prop_lump_found = false;
    while(game_lump_count--) {
        if (0
            || !fr.ReadUINT32_LE(gl_id)
            || !fr.ReadUINT16_LE(gl_flags)
            || !fr.ReadUINT16_LE(gl_version)
            || !fr.ReadUINT32_LE(gl_fileofs)
            || !fr.ReadUINT32_LE(gl_filelen))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };


        // We only care about the static prop game lump
        if (gl_id == 1936749168) { // decimal representation of ASCII "sprp"
            static_prop_lump_found = true;
            break;
        }
    }
    if (!static_prop_lump_found)
        return { utils::RetCode::SUCCESS }; // This map doesn't have static props, fine.

    if (gl_version != 10 && gl_version != 11)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Unsupported static prop game lump version: " + std::to_string(gl_version)};

    uint32_t sprp_dict_entry_count = 0;
    if (!fr.SetPos(gl_fileofs) || !fr.ReadUINT32_LE(sprp_dict_entry_count))
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

    if (sprp_dict_entry_count >= BspMap::MAX_STATIC_PROPS)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many static prop dict entries: " + std::to_string(sprp_dict_entry_count) };

    in_out.static_prop_model_dict.reserve(sprp_dict_entry_count);

    const size_t DICT_ENTRY_LEN = 128;
    char dict_entry[DICT_ENTRY_LEN + 1];
    dict_entry[DICT_ENTRY_LEN] = '\0'; // Ensure buffer is null-terminated

    while(sprp_dict_entry_count--) {
        if (!fr.ReadCharArray(dict_entry, DICT_ENTRY_LEN))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        // Convert to lower case because CSGO's file lookup is case-insensitive
        std::string mdl_path_lower_case =
            utils::NormalizeGameFilePath(dict_entry);
        in_out.static_prop_model_dict.push_back(std::move(mdl_path_lower_case));
    }

    uint32_t sprp_leaf_arr_entry_count = 0;
    if (!fr.ReadUINT32_LE(sprp_leaf_arr_entry_count))
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

    if (sprp_leaf_arr_entry_count >= BspMap::MAX_STATIC_PROPS * 100) // rough limit
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many static prop leaf array entries: " + std::to_string(sprp_leaf_arr_entry_count) };

    in_out.static_prop_leaf_arr.reserve(sprp_leaf_arr_entry_count);

    uint16_t leaf_arr_entry;
    while(sprp_leaf_arr_entry_count--) {
        if (!fr.ReadUINT16_LE(leaf_arr_entry))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.static_prop_leaf_arr.push_back(leaf_arr_entry);
    }

    uint32_t sprp_count = 0;
    if (!fr.ReadUINT32_LE(sprp_count))
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

    if (sprp_count > BspMap::MAX_STATIC_PROPS)
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Too many static props: " + std::to_string(sprp_count) };

    in_out.static_props.reserve(sprp_count);

    // With static prop game lump version 10, the static prop structure is 76 bytes
    // With static prop game lump version 11, the static prop structure is 80 bytes
    uint8_t unused[80];
    while (sprp_count--) {
        BspMap::StaticProp sprop;
        bool read_fail = false;

        if (0
            || !fr.ReadFLOAT32_LE(sprop.origin.x())
            || !fr.ReadFLOAT32_LE(sprop.origin.y())
            || !fr.ReadFLOAT32_LE(sprop.origin.z())
            || !fr.ReadFLOAT32_LE(sprop.angles.x())
            || !fr.ReadFLOAT32_LE(sprop.angles.y())
            || !fr.ReadFLOAT32_LE(sprop.angles.z())
            || !fr.ReadUINT16_LE(sprop.model_idx)
            || !fr.ReadUINT16_LE(sprop.first_leaf)
            || !fr.ReadUINT16_LE(sprop.leaf_count)
            || !fr.ReadUINT8(sprop.solid)
            || !fr.ReadByteArray(unused, 45)) {
            read_fail = true;
        }

        // Reading the remaining fields depends on the game lump version
        if (!read_fail) {
            if (gl_version == 10) {
                // uniform scale field doesn't exist in version 10
                sprop.uniform_scale = 1.0f;
            }
            if (gl_version == 11) {
                if (!fr.ReadFLOAT32_LE(sprop.uniform_scale))
                    read_fail = true;
            }
        }

        if (read_fail)
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        in_out.static_props.push_back(sprop);
    }

    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLump_Pakfile(AssetFileReader& fr, BspMap& in_out)
{
    in_out.packed_files.clear();
    uint32_t lump_len = in_out.header.lump_dir[LUMP_IDX_PAKFILE].file_len;
    uint32_t lump_end_pos = fr.GetPos() + lump_len;
    if (lump_len == 0)
        return { utils::RetCode::SUCCESS }; // No packed files is fine

    // Read an unknown amount of ZIP_LocalFileHeader
    while (1) {
        size_t header_pos = fr.GetPos();
        uint32_t next_signature;
        if (fr.GetPos() + 4 > lump_end_pos)
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
                "Invalid lump, lump_end_pos too small" };

        if (!fr.ReadUINT32_LE(next_signature))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };

        // Once we encounter signatures of ZIP_FileHeader (PK12) or
        // ZIP_EndOfCentralDirRecord (PK56) structures we finished collecting
        // all packed files.
        if (0
            || next_signature == ('P' | 'K' << 8 | (1) << 16 | (2) << 24)
            || next_signature == ('P' | 'K' << 8 | (5) << 16 | (6) << 24))
            return { utils::RetCode::SUCCESS };

        // Until then we must only get ZIP_LocalFileHeader structures (PK34)
        if (next_signature != ('P' | 'K' << 8 | (3) << 16 | (4) << 24))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
                "Invalid lump, expected ZIP_LocalFileHeader structures" };

        const size_t MIN_LOCALFILEHEADER_SIZE = 30; // excluding variable length fields
        if (header_pos + MIN_LOCALFILEHEADER_SIZE > lump_end_pos)
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
                "Invalid lump, lump_end_pos too small" };

        uint8_t unused[4];
        uint16_t compression_method;
        uint32_t crc32;
        uint32_t size_uncompressed;
        uint16_t file_name_len;
        uint16_t extra_field_len;
        if (0
            || !fr.ReadByteArray(unused, 4)
            || !fr.ReadUINT16_LE(compression_method)
            || !fr.ReadByteArray(unused, 4)
            || !fr.ReadUINT32_LE(crc32)
            || !fr.ReadByteArray(unused, 4)
            || !fr.ReadUINT32_LE(size_uncompressed)
            || !fr.ReadUINT16_LE(file_name_len)
            || !fr.ReadUINT16_LE(extra_field_len))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };


        if (compression_method != 0) // Fail if file is not uncompressed
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
                "Unsupported compression method: " + std::to_string(compression_method) };

        if (file_name_len > 2048) // Ensure arbitrary, but sane limit
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
                "File name is too long: " + std::to_string(file_name_len) };

        size_t variable_fields_total_len =
            file_name_len + extra_field_len + size_uncompressed;
        if (fr.GetPos() + variable_fields_total_len > lump_end_pos)
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
                "Invalid lump, lump_end_pos too small" };

        std::string fname(file_name_len, '\0');
        if (!fr.ReadCharArray(fname.data(), file_name_len))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Read error" };


        BspMap::PakfileEntry entry;
        // Convert to lower case because CSGO's file lookup is case-insensitive
        entry.file_name = utils::NormalizeGameFilePath(fname);
        entry.file_offset = fr.GetPos() + extra_field_len;
        entry.file_len = size_uncompressed;
        entry.crc32 = crc32;

        in_out.packed_files.push_back(std::move(entry));

        if (!fr.SetPos(fr.GetPos() + extra_field_len + size_uncompressed))
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
                "Seek to end of packed file failed" };
    }
    return { utils::RetCode::SUCCESS };
}

// Returned code is SUCCESS (possibly with warning msg) or
// ERROR_BSP_PARSING_FAILED (with desc msg)
utils::RetCode ParseHeader(AssetFileReader& fr, BspMap::Header& out)
{
    auto p_err = utils::RetCode::ERROR_BSP_PARSING_FAILED; // parsing error code
    std::string read_error_msg = "BSP file read error";

    std::string warning_msgs = "";

    // Read map file's identifier and version
    if (!fr.ReadUINT32_LE(out.identifier) || !fr.ReadUINT32_LE(out.version))
        return { p_err, read_error_msg };

    if (out.identifier == ('V' | ('B' << 8) | ('S' << 16) | ('P' << 24))) { // VBSP
        out.little_endian = true;
    }
    else if (out.identifier == ('P' | ('S' << 8) | ('B' << 16) | ('V' << 24))) { // PSBV
        out.little_endian = false;
        return { p_err, "Big-endian file format is not supported"};
    }
    else { // Unknown BSP format
        return { p_err, "Invalid BSP file, header.identifier = "
            + std::to_string(out.identifier)};
    }

    if (out.version < 21)
        return { p_err, "Unsupported BSP file version: "
            + std::to_string(out.version) + "\nOnly version 21 is supported."};

    if (out.version > 21)
        warning_msgs += "This BSP file has a newer file format version that "
            "might be unsupported: " + std::to_string(out.version) + "\nThis "
            "version of this program is only guaranteed to read BSP version 21.\n"
            "The loaded map might be broken or missing elements!";

    for (size_t i = 0; i < BspMap::HEADER_LUMP_CNT; ++i) {
        BspMap::LumpDirEntry& lump = out.lump_dir[i];
        if (0
            || !fr.ReadUINT32_LE(lump.file_offset)
            || !fr.ReadUINT32_LE(lump.file_len)
            || !fr.ReadUINT32_LE(lump.version)
            || !fr.ReadUINT32_LE(lump.four_cc))
            return { p_err, read_error_msg };
    }

    if (!fr.ReadUINT32_LE(out.map_revision))
        return { p_err, read_error_msg };

    return { utils::RetCode::SUCCESS, warning_msgs };
}

// Returned code is SUCCESS (no desc) or ERROR_BSP_PARSING_FAILED (with desc)
utils::RetCode ParseLumpData(AssetFileReader& fr, BspMap& in_out)
{
    auto p_err = utils::RetCode::ERROR_BSP_PARSING_FAILED; // parsing error code

    // Indices of all lumps we are interested in
    std::vector<size_t> required_lumps = {
        LUMP_IDX_ENTITIES,
        LUMP_IDX_PLANES,
        LUMP_IDX_VERTEXES,
        LUMP_IDX_EDGES,
        LUMP_IDX_SURFEDGES,
        LUMP_IDX_FACES,
        LUMP_IDX_ORIGINALFACES,
        LUMP_IDX_DISP_VERTS,
        LUMP_IDX_DISP_TRIS,
        LUMP_IDX_DISPINFO,
        LUMP_IDX_TEXINFO,
        LUMP_IDX_TEXDATA,
        LUMP_IDX_TEXDATA_STRING_TABLE,
        LUMP_IDX_TEXDATA_STRING_DATA,
        LUMP_IDX_BRUSHES,
        LUMP_IDX_BRUSHSIDES,
        LUMP_IDX_NODES,
        LUMP_IDX_LEAFS,
        LUMP_IDX_LEAFFACES,
        LUMP_IDX_LEAFBRUSHES,
        LUMP_IDX_MODELS,
        LUMP_IDX_GAME_LUMP,
        LUMP_IDX_PAKFILE
    };

    const auto& lump_dir = in_out.header.lump_dir; // for convenience

    // Sort required_lumps by the order they appear in the file
    std::sort(required_lumps.begin(), required_lumps.end(),
        [&lump_dir](size_t idx_a, size_t idx_b) {
            return lump_dir[idx_a].file_offset < lump_dir[idx_b].file_offset;
        });

    // Read the rest of the file linearly, as required_lumps is sorted by file offset
    for (size_t next_lump_idx : required_lumps) {
        const BspMap::LumpDirEntry& next_lump = lump_dir[next_lump_idx];

        // Abort if lump is compressed
        if (next_lump.four_cc != 0)
            return { p_err, "Compressed lumps are not supported. (lump "
                + std::to_string(next_lump_idx) + ")"};

        // Skip forward to lump position in the file if lump is non-empty
        if (next_lump.file_len != 0) {
            // Detect if some parse function read too many bytes
            if (fr.GetPos() > next_lump.file_offset)
                return { p_err, "Parse bug: Reader (pos="
                    + std::to_string(fr.GetPos()) + ") already advanced past lump "
                    + std::to_string(next_lump_idx) + " (file_offset="
                    + std::to_string(next_lump.file_offset) + ")" };

            // Seek if necessary
            if (next_lump.file_offset != fr.GetPos()) {
                if (!fr.SetPos(next_lump.file_offset))
                    return { p_err, "BSP file read error: Seek to lump "
                        + std::to_string(next_lump_idx) + " (file_offset="
                        + std::to_string(next_lump.file_offset) + ") failed" };
            }
        }

        // Call the right parse function
        utils::RetCode ret;
        switch (next_lump_idx)
        {
        case LUMP_IDX_ENTITIES:             ret = ParseLump_Entities(fr, in_out); break;
        case LUMP_IDX_PLANES:               ret = ParseLump_Planes(fr, in_out); break;
        case LUMP_IDX_VERTEXES:             ret = ParseLump_Vertexes(fr, in_out); break;
        case LUMP_IDX_EDGES:                ret = ParseLump_Edges(fr, in_out); break;
        case LUMP_IDX_SURFEDGES:            ret = ParseLump_SurfEdges(fr, in_out); break;
        case LUMP_IDX_FACES:                ret = ParseLump_Faces(fr, in_out); break;
        case LUMP_IDX_ORIGINALFACES:        ret = ParseLump_OriginalFaces(fr, in_out); break;
        case LUMP_IDX_DISP_VERTS:           ret = ParseLump_DispVerts(fr, in_out); break;
        case LUMP_IDX_DISP_TRIS:            ret = ParseLump_DispTris(fr, in_out); break;
        case LUMP_IDX_DISPINFO:             ret = ParseLump_DispInfos(fr, in_out); break;
        case LUMP_IDX_TEXINFO:              ret = ParseLump_TexInfos(fr, in_out); break;
        case LUMP_IDX_TEXDATA:              ret = ParseLump_TexDatas(fr, in_out); break;
        case LUMP_IDX_TEXDATA_STRING_TABLE: ret = ParseLump_TexDataStringTable(fr, in_out); break;
        case LUMP_IDX_TEXDATA_STRING_DATA:  ret = ParseLump_TexDataStringData(fr, in_out); break;
        case LUMP_IDX_BRUSHES:              ret = ParseLump_Brushes(fr, in_out); break;
        case LUMP_IDX_BRUSHSIDES:           ret = ParseLump_BrushSides(fr, in_out); break;
        case LUMP_IDX_NODES:                ret = ParseLump_Nodes(fr, in_out); break;
        case LUMP_IDX_LEAFS:                ret = ParseLump_Leafs(fr, in_out); break;
        case LUMP_IDX_LEAFFACES:            ret = ParseLump_LeafFaces(fr, in_out); break;
        case LUMP_IDX_LEAFBRUSHES:          ret = ParseLump_LeafBrushes(fr, in_out); break;
        case LUMP_IDX_MODELS:               ret = ParseLump_Models(fr, in_out); break;
        case LUMP_IDX_GAME_LUMP:            ret = ParseLump_GameLump(fr, in_out); break;
        case LUMP_IDX_PAKFILE:              ret = ParseLump_Pakfile(fr, in_out); break;
        default:
            ret = { p_err, "Parse bug: Lump " + std::to_string(next_lump_idx)
                + " is missing a switch case!" };
            break;
        }

        if (!ret.successful()) {
            return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
                "Error occurred while parsing lump " + std::to_string(next_lump_idx)
                + ":\n\n" + ret.desc_msg
            };
        }
    }

    return { utils::RetCode::SUCCESS };
}

// Reads content of the '.bsp' file that the reader is opened in and puts them
// into the BspMap object. Returned code is SUCCESS (possibly with warning msg)
// or ERROR_BSP_PARSING_FAILED (with an error description)
static utils::RetCode _ParseBspMapFile(BspMap& dest_bsp_map,
    AssetFileReader& opened_reader)
{
    if (!opened_reader.IsOpenedInFile())
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED, "Reader not opened in file"};

    // Parse file data
    utils::RetCode header_parse_status = ParseHeader(opened_reader, dest_bsp_map.header);
    if (!header_parse_status.successful())
        return header_parse_status; // Return header parse error

    std::string parse_warning_msg = header_parse_status.desc_msg;

    // Parse lumps
    utils::RetCode lump_data_parse_status = ParseLumpData(opened_reader, dest_bsp_map);
    if (!lump_data_parse_status.successful())
        return lump_data_parse_status; // Return lump parse error

    return { utils::RetCode::SUCCESS, parse_warning_msg };
}

utils::RetCode csgo_parsing::ParseBspMapFile(
    std::shared_ptr<BspMap>* dest_parsed_bsp_map,
    const std::string& abs_bsp_file_path)
{
    ZoneScoped;

    AssetFileReader reader;
    if (!reader.OpenFileFromAbsolutePath(abs_bsp_file_path)) {
        if (dest_parsed_bsp_map)
            *dest_parsed_bsp_map = { nullptr };
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Failed to open '.bsp' file" };
    }

    std::shared_ptr<BspMap> bsp_map = std::make_shared<BspMap>(abs_bsp_file_path);
    auto status = _ParseBspMapFile(*bsp_map, reader);
    if (dest_parsed_bsp_map) {
        if (status.successful()) *dest_parsed_bsp_map = bsp_map;
        else                     *dest_parsed_bsp_map = { nullptr };
    }
    return status;
}

utils::RetCode csgo_parsing::ParseBspMapFile(
    std::shared_ptr<BspMap>* dest_parsed_bsp_map,
    Containers::ArrayView<const uint8_t> bsp_file_content)
{
    AssetFileReader reader;
    if (!reader.OpenFileFromMemory(bsp_file_content)) {
        if (dest_parsed_bsp_map)
            *dest_parsed_bsp_map = { nullptr };
        return { utils::RetCode::ERROR_BSP_PARSING_FAILED,
            "Failed to open '.bsp' file (in memory)" };
    }

    std::shared_ptr<BspMap> bsp_map = std::make_shared<BspMap>(bsp_file_content);
    auto status = _ParseBspMapFile(*bsp_map, reader);
    if (dest_parsed_bsp_map) {
        if (status.successful()) *dest_parsed_bsp_map = bsp_map;
        else                     *dest_parsed_bsp_map = { nullptr };
    }
    return status;
}
