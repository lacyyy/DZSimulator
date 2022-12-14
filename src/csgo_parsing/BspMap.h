#ifndef CSGO_PARSING_BSPMAP_H_
#define CSGO_PARSING_BSPMAP_H_

#include <set>
#include <string>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

namespace csgo_parsing {

// Most of this info is taken from source-sdk-2013/<...>/src/public/bspfile.h
class BspMap {
public:
    static const size_t HEADER_LUMP_CNT = 64;

    static const size_t MAX_ENTITIES = 20480;
    static const size_t MAX_ENTITY_KEY_LEN = 32;
    static const size_t MAX_ENTITY_VALUE_LEN = 1024;

    static const size_t MAX_PLANES = 65536;

    static const size_t MAX_VERTICES = 65536;
    static const size_t MAX_EDGES = 256000;
    static const size_t MAX_SURFEDGES = 512000;

    static const size_t MAX_FACES = 65536;
    static const size_t MAX_ORIGINALFACES = 65536;

    // Displacement power: 2 -> 4 subdivisions, 3 -> 8 subdivions, 4 -> 16 subdivisions
    static const size_t MAX_DISPINFOS = 32768; // Displacements
    static const size_t MIN_DISP_POWER = 2;
    static const size_t MAX_DISP_POWER = 4;
    static const size_t MAX_DISPVERTS = (MAX_DISPINFOS * ((1 << MAX_DISP_POWER) + 1) * ((1 << MAX_DISP_POWER) + 1));
    static const size_t MAX_DISPTRIS = (MAX_DISPINFOS * 2 * (1 << MAX_DISP_POWER) * (1 << MAX_DISP_POWER));

    static const size_t MAX_TEXINFOS = 12288;
    static const size_t MAX_TEXDATAS = 2048;
    static const size_t MAX_TEXDATA_STRING_TABLE_ENTRIES = MAX_TEXDATAS; // unsure
    static const size_t MAX_TEXDATA_STRING_DATA = 256000;
    static const size_t MAX_TEXTURE_NAME_LENGTH = 128;

    // Currently, max csgo brush length is 32768.
    // MAX_BRUSH_LENGTH must be bigger, it's the length of the cube to cut brushes from
    static const size_t MAX_BRUSH_LENGTH = 65536; // 2 times the current limit in csgo
    static const size_t MAX_BRUSHES = 8192;
    static const size_t MAX_BRUSHSIDES = 65536;
    static const size_t MAX_BRUSH_SIDES = 128; // max number of faces on a single brush

    static const size_t MAX_NODES = 65536;
    static const size_t MAX_LEAFS = 65536;
    static const size_t MAX_LEAFFACES = 65536;
    static const size_t MAX_LEAFBRUSHES = 65536;

    static const size_t MAX_MODELS = 1024;

    static const size_t MAX_STATIC_PROPS = 65536; // keep room; CSGO's current limit is 16384

    struct LumpDirEntry {
        uint32_t file_offset;
        uint32_t file_len;
        uint32_t version;
        uint32_t four_cc;
    };

    struct Header { // BSP file header
        uint32_t     identifier; // interpreted as little-endian uint32
        bool         little_endian; // if bsp file is saved in little endian form
        uint32_t     version;
        LumpDirEntry lump_dir[HEADER_LUMP_CNT];
        uint32_t     map_revision;
    };

    struct Plane {
        Magnum::Vector3 normal;
        float dist; // distance from origin
        // FIXME Add type here?! Might be useful for brush vertex parsing
        // https://developer.valvesoftware.com/wiki/Source_BSP_File_Format#Plane
    };

    struct Edge {
        uint16_t v[2]; // indices into vertices array
    };

    struct Face {
        uint32_t first_edge; // index into surfedges array
        uint16_t num_edges; // total number of surfedges
        int16_t disp_info; // index into dispinfos array
    };

    struct OrigFace {
        uint32_t first_edge; // index into surfedges array
        uint16_t num_edges; // total number of surfedges
        int16_t disp_info; // index into dispinfos array
    };

    struct DispVert {
        Magnum::Vector3 vec; // normalized offset direction
        float dist; // offset distance
    };

    struct DispTri {
        uint16_t tags;
        bool HasTag_SURFACE() const;
        bool HasTag_WALKABLE() const; // Not used by CSGO, sv_walkable_normal determines if walkable
        bool HasTag_BUILDABLE() const;
        bool HasFlag_SURFPROP1() const;
        bool HasFlag_SURFPROP2() const;
        bool HasTag_REMOVE() const;
    };

    struct DispInfo {
        Magnum::Vector3 start_pos;
        uint32_t disp_vert_start; // index into dispverts array
        uint32_t disp_tri_start; // index into disptris array
        uint32_t power;
        uint32_t flags; // not documented on Valve Dev Community's "BSP File Format" page
        uint16_t map_face; // which map face this displacement comes from, index into faces array
        bool HasFlag_NO_PHYSICS_COLL() const;
        bool HasFlag_NO_HULL_COLL()    const; // if not solid to player and bump mines
        bool HasFlag_NO_RAY_COLL()     const;
        bool HasFlag_UNKNOWN_1()       const;
        bool HasFlag_UNKNOWN_2()       const; // every displacement seems to have this flag
    };

    struct TexInfo {
        uint32_t flags;
        uint32_t texdata; // index into texdatas array
        bool HasFlag_LIGHT()     const;
        bool HasFlag_SKY2D()     const;
        bool HasFlag_SKY()       const;
        bool HasFlag_WARP()      const;
        bool HasFlag_TRANS()     const;
        bool HasFlag_NOPORTAL()  const;
        bool HasFlag_TRIGGER()   const;
        bool HasFlag_NODRAW()    const;
        bool HasFlag_HINT()      const;
        bool HasFlag_SKIP()      const;
        bool HasFlag_NOLIGHT()   const;
        bool HasFlag_BUMPLIGHT() const;
        bool HasFlag_NOSHADOWS() const;
        bool HasFlag_NODECALS()  const;
        bool HasFlag_NOCHOP()    const;
        bool HasFlag_HITBOX()    const;
    };

    struct TexData {
        uint32_t name_string_table_id; // index into texdatastringtable array
    };

    struct Brush {
        uint32_t first_side; // index into brushsides array
        uint32_t num_sides;
        uint32_t contents;

        bool HasFlags(uint32_t flags) const;

        static const uint32_t       SOLID = 1 <<  0;
        static const uint32_t      WINDOW = 1 <<  1;
        static const uint32_t       GRATE = 1 <<  3;
        static const uint32_t       WATER = 1 <<  5;
        static const uint32_t    MOVEABLE = 1 << 14;
        static const uint32_t  PLAYERCLIP = 1 << 16;
        static const uint32_t GRENADECLIP = 1 << 19;
        static const uint32_t   DRONECLIP = 1 << 20;
        static const uint32_t      DEBRIS = 1 << 26;
        static const uint32_t      DETAIL = 1 << 27;
        static const uint32_t      LADDER = 1 << 29;
        static const uint32_t      HITBOX = 1 << 30;
    };

    struct BrushSide {
        uint16_t plane_num; // index into planes array
        int16_t texinfo; // index into texinfos array
        int16_t disp_info; // index into dispinfos array
        int16_t bevel; // relevant for collision? randomly takes values of 0,1,256,257
    };

    struct Node { // Node of the BSP tree
        int32_t children[2]; // positive: node index, negative: leaf index = (-1-child)
        uint16_t first_face; // index into faces array
        uint16_t num_faces; // counting both sides of the plane
    };

    struct Leaf {
        uint32_t contents; // OR of all brushes
        uint16_t first_leaf_face; // index into leaffaces array
        uint16_t num_leaf_faces;
        uint16_t first_leaf_brush; // index into leafbrushes array
        uint16_t num_leaf_brushes;
    };

    struct Model { // bmodel, a collection of brushes and faces, not a prop model!
        Magnum::Vector3 origin; // for sounds or lights (unnecessary for us?)
        int32_t head_node; // index into nodes(or leafs?) array (root of separate bsp tree)
        uint32_t first_face; // index into faces array
        uint32_t num_faces;
    };

    struct StaticProp {
        Magnum::Vector3 origin;
        Magnum::Vector3 angles; // pitch, yaw, roll
        uint16_t model_idx;  // index into static_prop_model_dict
        uint16_t first_leaf; // index into static_prop_leaf_arr
        uint16_t leaf_count;
        uint8_t solid; // 0 (SOLID_NONE), 2 (SOLID_BBOX) or 6 (SOLID_VPHYSICS)
        float uniform_scale;

        bool IsNotSolid() const; // prop is not solid
        bool IsSolidWithAABB() const; // prop's AABB is solid
        bool IsSolidWithVPhysics() const; // prop's vcollide model is solid
    };

    struct PakfileEntry {
        std::string file_name; // file name/path CONVERTED TO LOWER CASE
        uint32_t file_offset; // start pos of file contents (relative to beginning of bsp file)
        uint32_t file_len; // byte count of file contents at file_offset
        uint32_t crc32; // file checksum
    };

    // --------------------------------------------------------------------------

    // Absolute file path to BSP file, may contain UTF-8 Unicode chars
    BspMap(const std::string& abs_bsp_file_path);

    std::string abs_bsp_file_path; // absolute path, may contain UTF-8 Unicode

    Header header;

    std::vector<Plane> planes; // Plane lump (idx 1)

    std::vector<Magnum::Vector3> vertices; // Vertex lump (idx 3)
    std::vector<Edge> edges; // Edge lump (idx 12)
    std::vector<int32_t> surfedges; // Surfedge lump (idx 13)

    std::vector<Face> faces; // Face lump (idx 7)
    std::vector<OrigFace> origfaces; // Original face lump (idx 27)

    std::vector<DispVert> dispverts; // DispVert lump (idx 33)
    std::vector<DispTri> disptris; // DispTri lump (idx 48)
    std::vector<DispInfo> dispinfos; // DispInfo lump (idx 26)

    std::vector<TexInfo> texinfos; // TexInfo lump (idx 6)
    std::vector<TexData> texdatas; // TexData lump (idx 2)
    std::vector<uint32_t> texdatastringtable; // TexdataStringTable lump (idx 44)
    std::vector<char> texdatastringdata; // TexdataStringData lump (idx 43)

    std::vector<Brush> brushes; // Brush lump (idx 18)
    std::vector<BrushSide> brushsides; // BrushSide lump (idx 19)

    std::vector<Node> nodes; // Node lump (idx 5)
    std::vector<Leaf> leafs; // Leaf lump (idx 10)
    std::vector<uint16_t> leaffaces; // LeafFace lump (idx 16)
    std::vector<uint16_t> leafbrushes; // LeafBrush lump (idx 17)

    std::vector<Model> models; // Model lump (idx 14)

    // from Game lump (idx 35)
    std::vector<std::string> static_prop_model_dict; // model names CONVERTED TO LOWER CASE
    std::vector<uint16_t> static_prop_leaf_arr; // unused for now
    std::vector<StaticProp> static_props;

    std::vector<PakfileEntry> packed_files; // from Pakfile lump (idx 40)

    // --------------------------------------------------------------------------
    // Entities

    struct PlayerSpawn {
        Magnum::Vector3 origin;
        Magnum::Vector3 angles;
        int16_t priority;
    };

    struct Ent_func_brush {
        std::string model;
        Magnum::Vector3 origin;
        Magnum::Vector3 angles;
        int16_t solidity; // 0: Solidity toggled with visibility, 1: Never Solid, 2: Always Solid
        bool start_disabled;
        bool IsSolid() const;
    };

    struct Ent_trigger_push {
        std::string model;
        Magnum::Vector3 origin;
        Magnum::Vector3 angles;
        Magnum::Vector3 pushdir;
        uint32_t spawnflags;
        bool start_disabled;
        float speed; // units per second
        bool only_falling_players; // Only push players if they are falling (and not pressing jump)
        float falling_speed_threshold; // Player must be falling this fast for push to happen

        bool CanPushPlayers() const;
        // Not tested if this property affects the push mechanic. It's present
        // in dz_ember's geyser push triggers.
        bool CorrectlyAccountsForObjectMass() const;
    };

    // worldspawn entity
    Magnum::Vector3 world_mins, world_maxs; // describe player area, but water towers still go above maxs[2]
    int32_t map_version; // same as map_revision in bsp file header
    std::string sky_name;
    std::string detail_material;
    std::string detail_vbsp;

    // Parsed from info_player_terrorist and info_player_counterterrorist entities
    std::vector<PlayerSpawn> player_spawns;

    std::vector<Ent_func_brush>   entities_func_brush;
    std::vector<Ent_trigger_push> entities_trigger_push;


    // --------------------------------------------------------------------------

    // If two map vertices are so close together they should be considered equal
    static bool AreVerticesEquivalent(const Magnum::Vector3& a, const Magnum::Vector3& b);

    std::vector<Magnum::Vector3> GetFaceVertices(uint32_t face_idx) const; // index into faces array

    std::vector<Magnum::Vector3> GetDisplacementVertices(size_t disp_info_idx) const;
    std::vector<std::vector<Magnum::Vector3>> GetDisplacementFaceVertices() const;
    std::vector<std::vector<Magnum::Vector3>> GetDisplacementBoundaryFaceVertices() const;

    std::vector<std::vector<Magnum::Vector3>> GetBrushFaceVertices(
        const std::set<size_t>& brush_indices, // list of all brush indices that we want to look at
        bool (*pred_Brush)(const Brush&) = nullptr, // brush selection function
        bool (*pred_BrushSide)(const BrushSide&, const BspMap&) = nullptr) // brushside selection function
        const;


    std::set<size_t> GetModelBrushIndices_worldspawn() const; // worldspawn is model idx 0
    std::set<size_t> GetModelBrushIndices(uint32_t model_index) const;

};

} // namespace csgo_parsing

#endif // CSGO_PARSING_BSPMAP_H_
