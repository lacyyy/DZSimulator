#ifndef CSGO_PARSING_BSPMAP_H_
#define CSGO_PARSING_BSPMAP_H_

#include <cfloat>
#include <set>
#include <string>
#include <vector>

#include <Corrade/Containers/ArrayView.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

namespace csgo_parsing {

class BspMap {
public:
    // -------- start of source-sdk-2013 code --------
    // Most of the information in here is taken from these 2 sources:
    //   - source-sdk-2013/<...>/src/public/bspfile.h
    //   - https://developer.valvesoftware.com/wiki/BSP_%28Source_1%29

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

    static const size_t MAX_BRUSH_LENGTH = 32768; // max brush length in CSGO
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
        uint32_t file_offset = 0;
        uint32_t file_len = 0;
        uint32_t version = 0;
        uint32_t four_cc = 0;
    };

    struct Header { // BSP file header
        uint32_t     identifier = 0; // interpreted as little-endian uint32
        bool         little_endian = true; // if bsp file is saved in little endian form
        uint32_t     version = 0;
        LumpDirEntry lump_dir[HEADER_LUMP_CNT];
        uint32_t     map_revision = 0;
    };

    // @Cleanup This plane structure should be moved to a 'common' header file
    struct Plane {
        Magnum::Vector3 normal;
        float dist; // distance from origin
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

        // FIXME: Apparently, flags are only set if MSb is set.
        //  TEST: Are there dispinfos without the MSb set?
        // https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/builddisp.cpp#L762-L768
        // https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/utils/vbsp/disp_vbsp.cpp#L325-L328

        bool HasFlag_NO_PHYSICS_COLL() const;
        bool HasFlag_NO_HULL_COLL()    const; // if not solid to player and bump mines
        bool HasFlag_NO_RAY_COLL()     const;
        bool HasFlag_UNKNOWN_1()       const;
        bool HasFlag_UNKNOWN_2()       const; // every displacement seems to have this flag
        static const uint32_t FLAG_NO_PHYSICS_COLL = ((uint32_t)1 <<  1);
        static const uint32_t FLAG_NO_HULL_COLL    = ((uint32_t)1 <<  2);
        static const uint32_t FLAG_NO_RAY_COLL     = ((uint32_t)1 <<  3);
        static const uint32_t FLAG_UNKNOWN_1       = ((uint32_t)1 << 30);
        static const uint32_t FLAG_UNKNOWN_2       = ((uint32_t)1 << 31);
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
        int16_t bevel; // If 1, this brushside is only used for detection of collisions with AABBs
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
    // --------- end of source-sdk-2013 code ---------

    // --------------------------------------------------------------------------

    // Map was read from a regular file on the file system.
    // Takes absolute path to that '.bsp' file, may contain UTF-8 Unicode chars.
    // Knowing the '.bsp' file's path is necessary to load its packed files later.
    BspMap(const std::string& abs_bsp_file_path);

    // Map was read from an embedded '.bsp' file that was compiled into the executable.
    // Takes memory block containing that file's content.
    // Knowing the '.bsp' file's content location is necessary to load its
    // packed files later.
    BspMap(Corrade::Containers::ArrayView<const uint8_t> bsp_file_content);

    struct FileOrigin {
        enum Type {
            UNKNOWN,     // Map is uninitialized or constructed from another source
            FILE_SYSTEM, // Map was loaded from '.bsp' file on the file system
            MEMORY       // Map was loaded from memory block holding '.bsp' file content
        };
        Type type = UNKNOWN;

        // --- origin info for type == FILE_SYSTEM
        std::string abs_file_path = ""; // absolute path, may contain UTF-8 Unicode
        // --- origin info for type == MEMORY
        Corrade::Containers::ArrayView<const uint8_t> file_content_mem = {};
    }; 
    const FileOrigin file_origin; // Where this BSP map was loaded from

    // Embedded map files (whose file content got compiled into the executable)
    // receive special treatment:
    //   - Assets referenced by the map are not looked up in the game directory
    //     or the game's VPK archives (because embedded maps must be independent
    //     and self-contained)
    //   - Parsing the collision model of a solid prop (static or dynamic) no
    //     longer requires the '.mdl' file referenced by the prop to exist
    //     (because we don't use '.mdl' file content and removing them reduces
    //     embedded file size)
    const bool is_embedded_map;


    Header header;

    std::vector<Plane> planes; // Plane lump (idx 1)

    std::vector<Magnum::Vector3> vertices; // Vertex lump (idx 3)
    std::vector<Edge> edges; // Edge lump (idx 12)
    std::vector<int32_t> surfedges; // Surfedge lump (idx 13)

    std::vector<Face> faces; // Face lump (idx 7)
    std::vector<OrigFace> origfaces; // Original face lump (idx 27) [CURRENTLY UNUSED]

    std::vector<DispVert> dispverts; // DispVert lump (idx 33)
    std::vector<DispTri> disptris; // DispTri lump (idx 48) [CURRENTLY UNUSED]
    std::vector<DispInfo> dispinfos; // DispInfo lump (idx 26)

    std::vector<TexInfo> texinfos; // TexInfo lump (idx 6)
    std::vector<TexData> texdatas; // TexData lump (idx 2)
    std::vector<uint32_t> texdatastringtable; // TexdataStringTable lump (idx 44)
    std::vector<char> texdatastringdata; // TexdataStringData lump (idx 43)

    std::vector<Brush> brushes; // Brush lump (idx 18)
    std::vector<BrushSide> brushsides; // BrushSide lump (idx 19)

    std::vector<Node> nodes; // Node lump (idx 5)
    std::vector<Leaf> leafs; // Leaf lump (idx 10)
    std::vector<uint16_t> leaffaces; // LeafFace lump (idx 16)  [CURRENTLY UNUSED]
    std::vector<uint16_t> leafbrushes; // LeafBrush lump (idx 17)

    std::vector<Model> models; // Model lump (idx 14)

    // from Game lump (idx 35)
    std::vector<std::string> static_prop_model_dict; // model names CONVERTED TO LOWER CASE
    std::vector<uint16_t> static_prop_leaf_arr; // [CURRENTLY UNUSED]
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

    struct Ent_prop_dynamic {
        Magnum::Vector3 origin;
        Magnum::Vector3 angles; // pitch, yaw, roll
        std::string model; // model name CONVERTED TO LOWER CASE
    };

    // worldspawn entity
    Magnum::Vector3 world_mins = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF }; // ???
    Magnum::Vector3 world_maxs = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF }; // ???
    int32_t map_version = -1; // same as map_revision in bsp file header
    std::string sky_name;
    std::string detail_material;
    std::string detail_vbsp;

    // Parsed from info_player_terrorist and info_player_counterterrorist entities
    std::vector<PlayerSpawn> player_spawns;

    std::vector<Ent_func_brush>   entities_func_brush;
    std::vector<Ent_trigger_push> entities_trigger_push;

    std::vector<Ent_prop_dynamic> relevant_dynamic_props; // Subset that's visualized

    // --------------------------------------------------------------------------

    // If two map vertices are so close together they should be considered equal
    static bool AreVerticesEquivalent(const Magnum::Vector3& a, const Magnum::Vector3& b);

    std::vector<Magnum::Vector3> GetFaceVertices(uint32_t face_idx) const; // index into faces array

    // Returns a displacement's vertices in the same order as they are found in
    // the BSP map file's DISP_VERTS lump
    std::vector<Magnum::Vector3> GetDisplacementVertices(size_t disp_info_idx) const;

    std::vector<std::vector<Magnum::Vector3>> GetDisplacementFaceVertices() const;
    std::vector<std::vector<Magnum::Vector3>> GetDisplacementBoundaryFaceVertices() const;

    // A returned face (std::vector<Magnum::Vector3>) is never empty
    std::vector<std::vector<Magnum::Vector3>> GetBrushFaceVertices(
        const std::set<size_t>& brush_indices, // list of all brush indices that we want to look at
        bool (*pred_Brush)(const Brush&) = nullptr, // brush selection function
        bool (*pred_BrushSide)(const BrushSide&, const BspMap&) = nullptr) // brushside selection function
        const;

    // If brush is invalid, false is returned and aabb_mins and aabb_maxs do not
    // get set.
    bool GetBrushAABB(size_t brush_idx,
        Magnum::Vector3* aabb_mins, Magnum::Vector3* aabb_maxs) const;

    std::set<size_t> GetModelBrushIndices_worldspawn() const; // worldspawn is model idx 0
    std::set<size_t> GetModelBrushIndices(uint32_t model_index) const;

};

} // namespace csgo_parsing

#endif // CSGO_PARSING_BSPMAP_H_
