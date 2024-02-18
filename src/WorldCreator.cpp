#include "WorldCreator.h"

#include <algorithm>
#include <utility>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <Tracy.hpp>

// Include these 2 headers so that GL::Buffer::setData() accepts std::vector.
// @Optimization Can we get rid of STL usage here?
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayViewStl.h>

#include <Corrade/Containers/Optional.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Shaders/GenericGL.h>
#include <Magnum/Trade/MeshData.h>

#include "coll/CollidableWorld.h"
#include "coll/CollidableWorld_Impl.h"
#include "csgo_parsing/AssetFileReader.h"
#include "csgo_parsing/AssetFinder.h"
#include "csgo_parsing/BspMap.h"
#include "csgo_parsing/PhyModelParsing.h"
#include "csgo_parsing/utils.h"
#include "ren/GlidabilityShader3D.h"
#include "ren/RenderableWorld.h"
#include "utils_3d.h"

using namespace Magnum;
using namespace csgo_parsing;
using namespace ren;
using namespace coll;
using namespace utils_3d;

// ------------------------------------------------------------------------
// ----------------- Internal GL::Mesh creation functions -----------------
// ------------------------------------------------------------------------

// From given faces (with clockwise vertex winding), create a GL::Mesh
// with vertex buffer with attributes:
//   - Vertex Position ( Magnum::Shaders::GenericGL3D::Position )
static GL::Mesh GenMeshWithVertAttr_Position(
    const std::vector<std::vector<Vector3>>& faces)
{
    struct Vert {
        Vector3 position;
    };
    std::vector<Vert> data_vertbuf;

    // Turn faces into triangles
    for (const std::vector<Vector3>& face : faces) {
        for (size_t tri = 0; tri < face.size() - 2; tri++) {
            data_vertbuf.push_back({ face[0]       });
            data_vertbuf.push_back({ face[tri + 1] });
            data_vertbuf.push_back({ face[tri + 2] });
        }
    }

    GL::Buffer vertices{ GL::Buffer::TargetHint::Array };
    vertices.setData(data_vertbuf);
    GL::Mesh mesh;
    mesh.setCount(vertices.size() / sizeof(Vert))
        .addVertexBuffer(std::move(vertices), 0,
            Shaders::GenericGL3D::Position{});
    return mesh;
}

// An element of a vertex buffer with a position and a normal attribute
struct VertBufElem_Pos_Nor {
    Vector3 position;
    Vector3 normal;
};

// Helper function
static void _AddFacesToVertBuf_Position_Normal(
    const std::vector<std::vector<Vector3>>& faces,
    std::vector<VertBufElem_Pos_Nor>& vert_buf)
{
    // Turn faces into triangles
    for (const std::vector<Vector3>& face : faces) {
        for (size_t tri = 0; tri < face.size() - 2; tri++) {
            // Individual normal calculation seems to be required, although
            // triangles *should* all face in the same direction?
            Vector3 normal = CalcNormalCwFront(face[0], face[tri+1], face[tri+2]);

            VertBufElem_Pos_Nor vert1 = { face[0],       normal };
            VertBufElem_Pos_Nor vert2 = { face[tri + 1], normal };
            VertBufElem_Pos_Nor vert3 = { face[tri + 2], normal };
            vert_buf.push_back(vert1);
            vert_buf.push_back(vert2);
            vert_buf.push_back(vert3);
        }
    }
}

// Helper function
static void _AddFacesToVertBuf_Position_Normal(
    const TriMesh& tri_mesh,
    std::vector<VertBufElem_Pos_Nor>& vert_buf)
{
    for (const TriMesh::Tri& tri : tri_mesh.tris) {
        const Vector3& v1 = tri_mesh.vertices[tri.verts[0]];
        const Vector3& v2 = tri_mesh.vertices[tri.verts[1]];
        const Vector3& v3 = tri_mesh.vertices[tri.verts[2]];
        Vector3 normal = CalcNormalCwFront(v1, v2, v3);
        VertBufElem_Pos_Nor vert1 = { v1, normal };
        VertBufElem_Pos_Nor vert2 = { v2, normal };
        VertBufElem_Pos_Nor vert3 = { v3, normal };
        vert_buf.push_back(vert1);
        vert_buf.push_back(vert2);
        vert_buf.push_back(vert3);
    }
}

// Helper function
static GL::Mesh _CreateMeshFromVertBuf_Position_Normal(
    std::vector<VertBufElem_Pos_Nor> vert_buf)
{
    // @Optimization Is there an unnecessary copy here?
    GL::Buffer vertices{ GL::Buffer::TargetHint::Array };
    vertices.setData(vert_buf);
    GL::Mesh mesh;
    mesh.setCount(vertices.size() / sizeof(VertBufElem_Pos_Nor))
        .addVertexBuffer(std::move(vertices), 0,
            Shaders::GenericGL3D::Position{},
            Shaders::GenericGL3D::Normal{});
    return mesh;
}

// From given faces (with clockwise vertex winding), create a GL::Mesh
// with vertex buffer with attributes:
//   - Vertex Position ( Magnum::Shaders::GenericGL3D::Position )
//   - Vertex Normal   ( Magnum::Shaders::GenericGL3D::Normal   )
static GL::Mesh GenMeshWithVertAttr_Position_Normal(
    const std::vector<std::vector<Vector3>>& faces)
{
    // @Optimization Reserve correct amount of elements for data_vertbuf
    std::vector<VertBufElem_Pos_Nor> data_vertbuf;

    _AddFacesToVertBuf_Position_Normal(faces, data_vertbuf);
    return _CreateMeshFromVertBuf_Position_Normal(data_vertbuf);
}

// Same as above, but collects all faces from a list of TriMesh objects.
static GL::Mesh GenMeshWithVertAttr_Position_Normal(
    const std::vector<TriMesh>& lists_of_tri_meshes)
{
    // @Optimization Reserve correct amount of elements for data_vertbuf
    std::vector<VertBufElem_Pos_Nor> data_vertbuf;

    for (const TriMesh& tri_mesh : lists_of_tri_meshes)
        _AddFacesToVertBuf_Position_Normal(tri_mesh, data_vertbuf);
    return _CreateMeshFromVertBuf_Position_Normal(data_vertbuf);
}


// ------------------------------------------------------------------------
// -------------------- WorldCreator member functions ---------------------
// ------------------------------------------------------------------------

GL::Mesh WorldCreator::CreateBumpMineMesh()
{
    return MeshTools::compile(Primitives::uvSphereSolid(7, 10));
}

std::pair<
    std::shared_ptr<RenderableWorld>,
    std::shared_ptr<CollidableWorld>>
WorldCreator::InitFromBspMap(
    std::shared_ptr<const BspMap> bsp_map,
    std::string* dest_errors)
{
    ZoneScoped;

    std::shared_ptr<RenderableWorld> r_world = std::make_shared<RenderableWorld>();
    std::string error_msgs = "";

    // Only look up assets in the game's directory and its VPK archives if it
    // isn't an embedded map. Embedded maps are supposed to be independent and
    // self-contained, not requiring any external files.
    bool use_game_dir_assets = !bsp_map->is_embedded_map;

    {
        ZoneScopedN("GenDispFaceMesh");
        Debug{} << "Parsing displacement face mesh";
        auto displacementFaces = bsp_map->GetDisplacementFaceVertices();
        r_world->mesh_displacements =
            GenMeshWithVertAttr_Position_Normal(displacementFaces);
        //MeshGenerator::GenStaticColoredMeshFromFaces(displacementFaces);
    } // Destruct face array once it's no longer needed (reduce peak RAM usage)

    // Idea: Instead of destructing face array, just .clear() it and reuse it
    // for displacement boundary faces?

    { // @Optimization Maybe only load when "Show displacement edges" is ticked
        ZoneScopedN("GenDispBoundaryMesh");
        Debug{} << "Parsing displacement boundary mesh";
        auto displacementBoundaryFaces =
            bsp_map->GetDisplacementBoundaryFaceVertices();
        r_world->mesh_displacement_boundaries =
            GenMeshWithVertAttr_Position(displacementBoundaryFaces);
    } // Destruct face array once it's no longer needed (reduce peak RAM usage)

    // Init required displacement collision structures
    std::vector<CDispCollTree> hull_disp_coll_trees;
    size_t relevant_disp_cnt = 0;
    for (size_t i = 0; i < bsp_map->dispinfos.size(); i++) {
        if (bsp_map->dispinfos[i].HasFlag_NO_HULL_COLL())
            continue;
        relevant_disp_cnt++;
    }
    hull_disp_coll_trees.reserve(relevant_disp_cnt);
    for (size_t i = 0; i < bsp_map->dispinfos.size(); i++) {
        if (bsp_map->dispinfos[i].HasFlag_NO_HULL_COLL())
            continue;
        // @Optimization Only get disp vertices once and use it for mesh and coll init
        hull_disp_coll_trees.emplace_back(i, *bsp_map);
    }
    
    // ---- Collect all ".mdl" and ".phy" files from the packed files
    std::vector<uint16_t> packed_mdl_file_indices; // indices into BspMap::packed_files
    std::vector<uint16_t> packed_phy_file_indices; // indices into BspMap::packed_files
    for (size_t i = 0; i < bsp_map->packed_files.size(); i++) {
        const std::string& fname = bsp_map->packed_files[i].file_name;
        if (fname.length() >= 5) {
            if      (fname.ends_with(".mdl")) packed_mdl_file_indices.push_back(i);
            else if (fname.ends_with(".phy")) packed_phy_file_indices.push_back(i);
        }
    }
    // ---- Sort packed file indices by file name to enable fast lookup later
    auto comp__packed_file_name = [&](uint16_t idx_a, uint16_t idx_b) {
        return bsp_map->packed_files[idx_a].file_name < bsp_map->packed_files[idx_b].file_name;
    };
    std::sort(
        packed_mdl_file_indices.begin(),
        packed_mdl_file_indices.end(),
        comp__packed_file_name);
    std::sort(
        packed_phy_file_indices.begin(),
        packed_phy_file_indices.end(),
        comp__packed_file_name);

    for (auto packed_file_idx : packed_mdl_file_indices)
        Debug{} << "packed MDL:" << bsp_map->packed_files[packed_file_idx].file_name.c_str();
    for (auto packed_file_idx : packed_phy_file_indices)
        Debug{} << "packed PHY:" << bsp_map->packed_files[packed_file_idx].file_name.c_str();

    // predicate function used for binary lookup of packed file idx with file name
    auto comp__find_packed_file_name_idx =
        [&](uint16_t packed_file_idx, const std::string& file_name) {
            return bsp_map->packed_files[packed_file_idx].file_name < file_name;
        };

    // ---- Load collision models of solid prop_static and prop_dynamic entities

    // Get MDL paths referenced by at least one solid prop (static or dynamic)
    std::set<std::string> solid_xprop_mdl_paths;
    for (const BspMap::StaticProp& sprop : bsp_map->static_props)
        if (sprop.IsSolidWithVPhysics())
            solid_xprop_mdl_paths.insert(bsp_map->static_prop_model_dict[sprop.model_idx]);
    for (const BspMap::Ent_prop_dynamic& dprop : bsp_map->relevant_dynamic_props)
        solid_xprop_mdl_paths.insert(dprop.model);

    // key:   ".mdl" file path referenced by at least one solid prop (static or dynamic)
    // value: Corresponding collision model mesh
    std::map<std::string, GL::Mesh> xprop_coll_meshes;

    // Collision models used in at least one solid prop (static or dynamic).
    // Keys are MDL paths, values are collision models.
    std::map<std::string, CollisionModel> xprop_coll_models;

    // When loading regular (non-embedded) maps, a requirement to consider a
    // prop as solid is the existence of the MDL file it references.
    // This is done to faithfully represent how CSGO would load a map.
    // When loading embedded maps, we don't require an MDL file for solid props
    // because these maps are custom-made to only be loaded by DZSimulator and
    // MDL files themselves are not read and they would unnecessarily increase
    // embedded file size.
    bool require_existing_mdl_file = !bsp_map->is_embedded_map;

    // Now attempt to load required collision models
    for (const std::string& mdl_path : solid_xprop_mdl_paths) {
        ZoneScopedN("xprop phy load");

        if (mdl_path.length() < 5) // Ensure valid file path
            continue;
        std::string phy_path = mdl_path;
        phy_path[phy_path.length() - 3] = 'p';
        phy_path[phy_path.length() - 2] = 'h';
        phy_path[phy_path.length() - 1] = 'y';

        // Search for MDL file in packed files
        auto it_packed_mdl_idx = std::lower_bound(
            packed_mdl_file_indices.begin(),
            packed_mdl_file_indices.end(),
            mdl_path,
            comp__find_packed_file_name_idx);
        bool is_mdl_in_packed_files =
            it_packed_mdl_idx != packed_mdl_file_indices.end() &&
            mdl_path.compare(bsp_map->packed_files[*it_packed_mdl_idx].file_name) == 0;

        // Search for PHY file in packed files
        auto it_packed_phy_idx = std::lower_bound(
            packed_phy_file_indices.begin(),
            packed_phy_file_indices.end(),
            phy_path,
            comp__find_packed_file_name_idx);
        bool is_phy_in_packed_files =
            it_packed_phy_idx != packed_phy_file_indices.end() &&
            phy_path.compare(bsp_map->packed_files[*it_packed_phy_idx].file_name) == 0;

        bool is_mdl_in_game_files = use_game_dir_assets ?
            AssetFinder::ExistsInGameFiles(mdl_path) : false;

        // Sometimes we require every prop to have an existing ".mdl" file
        if (require_existing_mdl_file
            && !is_mdl_in_game_files && !is_mdl_in_packed_files)
        {
            error_msgs += "Failed to find MDL file '" + mdl_path + "', "
                "referenced by at least one solid prop. "
                "All props with this model will be missing from the world.\n";
            continue;
        }

        // Open the PHY file at the correct location
        AssetFileReader phy_file_reader;
        std::string phy_file_read_err = ""; // empty means no error occurred

        if (is_phy_in_packed_files) {
            // Depending on where we parsed the original '.bsp' file from,
            // we need to read its packed files accordingly.
            switch (bsp_map->file_origin.type) {
            case BspMap::FileOrigin::FILE_SYSTEM: {
                auto& abs_bsp_file_path = bsp_map->file_origin.abs_file_path;
                if (!phy_file_reader.OpenFileFromAbsolutePath(abs_bsp_file_path))
                    phy_file_read_err = "Failed to open BSP file for parsing a "
                    "packed PHY file: " + abs_bsp_file_path;
                break;
            }
            case BspMap::FileOrigin::MEMORY: {
                auto& bsp_file_mem = bsp_map->file_origin.file_content_mem;
                if (!phy_file_reader.OpenFileFromMemory(bsp_file_mem))
                    phy_file_read_err = "Failed to open BSP file from memory to"
                    " parse packed PHY file";
                break;
            }
            default:
                phy_file_read_err = "Failed to read packed PHY file: Unknown "
                    "BSP file origin: " + std::to_string(bsp_map->file_origin.type);
                break;
            }

            // If opening the original bsp file succeeded without errors
            if (phy_file_read_err.empty()) {
                bool x = phy_file_reader.OpenSubFileFromCurrentlyOpenedFile(
                    bsp_map->packed_files[*it_packed_phy_idx].file_offset,
                    bsp_map->packed_files[*it_packed_phy_idx].file_len
                ); // This can't fail because phy_file_reader is opened in a file
            }
        }
        else {
            // Look for PHY file in game directory and VPK archives
            bool is_phy_in_game_files = use_game_dir_assets ?
                AssetFinder::ExistsInGameFiles(phy_path) : false;

            // Prop is non-solid if their model's PHY doesn't exist anywhere
            if (!is_phy_in_game_files)
                continue; // Not an error, we just skip this non-solid model

            if (!phy_file_reader.OpenFileFromGameFiles(phy_path))
                phy_file_read_err = "Failed to open PHY file from game files";
        }

        if (phy_file_read_err.empty()) { // If no error occurred on file open
            // A collision model consists of one or more "sections".
            // A "section" is a triangle mesh that describes a convex shape.
            std::vector<TriMesh> section_tri_meshes;
            std::string surface_property;
            // CSGO loads the phy model even if checksum of MDL and PHY are not identical.
            // NOTE: If you change the way PHY models are parsed, please see
            //       whether comments surrounding CollisionModel::section_tri_meshes
            //       need to be updated! E.g. regarding edge duplicate-freeness guarantees.
            // NOTE: Static props' phy model always have a single solid.
            //       Dynamic props' phy model very rarely have multiple solids.
            auto ret = ParseSingleSolidPhyModel(
                &section_tri_meshes, &surface_property, phy_file_reader);

            // Special case: We treat this error as a non-error because maps
            // rarely have dynamic props with a phy model with multiple solids.
            // These are mostly hostage/character models or an animated garage
            // doors. It's not worth supporting these, so skip without error.
            if (ret.code == csgo_parsing::utils::RetCode::ERROR_PHY_MULTIPLE_SOLIDS) {
                Debug{} << "Skipped multi-solid collision model:" << phy_path.c_str();
                continue; // Not an error
            }

            if (ret.successful()) {
                ZoneScopedN("gen phy mesh + collmodel");
                GL::Mesh phy_mesh = GenMeshWithVertAttr_Position_Normal(section_tri_meshes);
                xprop_coll_meshes[mdl_path] = std::move(phy_mesh);

                // For each section, get its AABB and create plane of each triangle
                const size_t NUM_SECTIONS = section_tri_meshes.size();
                std::vector<std::vector<BspMap::Plane>> section_planes(NUM_SECTIONS);
                std::vector<CollisionModel::AABB>       section_aabbs (NUM_SECTIONS);
                for (size_t section_idx = 0; section_idx < NUM_SECTIONS; section_idx++) {
                    const TriMesh& section_tri_mesh = section_tri_meshes[section_idx];
                    const std::vector<Vector3>& section_vertices = section_tri_mesh.vertices;
                    auto& planes_of_section = section_planes[section_idx];
                    planes_of_section.reserve(section_tri_mesh.tris.size());

                    Vector3 section_aabb_mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
                    Vector3 section_aabb_maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
                    for (const Vector3& vert : section_vertices) {
                        for (int axis = 0; axis < 3; axis++) { // Add vertex to section's AABB
                            section_aabb_mins[axis] = Math::min(section_aabb_mins[axis], vert[axis]);
                            section_aabb_maxs[axis] = Math::max(section_aabb_maxs[axis], vert[axis]);
                        }
                    }
                    section_aabbs[section_idx].mins = section_aabb_mins;
                    section_aabbs[section_idx].maxs = section_aabb_maxs;

                    for (const TriMesh::Tri& triangle : section_tri_mesh.tris) {
                        const Vector3& v1 = section_vertices[triangle.verts[0]];
                        const Vector3& v2 = section_vertices[triangle.verts[1]];
                        const Vector3& v3 = section_vertices[triangle.verts[2]];
                        Vector3 plane_normal = CalcNormalCwFront(v1, v2, v3);
                        float   plane_dist = Math::dot(plane_normal, v1);
                        planes_of_section.push_back({
                            .normal = plane_normal,
                            .dist   = plane_dist
                        });
                    }
                }
                // Construct CollisionModel object
                xprop_coll_models[mdl_path] = CollisionModel {
                    .section_tri_meshes = std::move(section_tri_meshes),
                    .section_planes     = std::move(section_planes),
                    .section_aabbs      = std::move(section_aabbs)
                };
            }
            else { // If parsing failed for other reasons, get error msg
                phy_file_read_err = ret.desc_msg;
            }
        }

        if (!phy_file_read_err.empty()) { // If anything failed
            error_msgs += "All prop_static/prop_dynamic using the model '"
                + mdl_path + "' will be missing from the world because loading "
                "their collision model failed:\n    " + phy_file_read_err + "\n";
        }
    }

    struct InstanceData {
        // @Optimization Instead, represent transformation using
        //               scaling + quaternion + translation or
        //               3x3 rotationscaling matrix + translation?
        Matrix4 model_transformation; // model scale, rotation, translation
        //Color3 color; // other attributes are possible
    };
    // key is MDL name, value is list of its xprop's transformation matrices
    std::map<std::string, std::vector<InstanceData>> xprop_instance_data;

    for (const BspMap::StaticProp& sprop : bsp_map->static_props) {
        const auto& mdl_path = bsp_map->static_prop_model_dict[sprop.model_idx];
        if (sprop.IsSolidWithVPhysics()) {
            // We only care about static props with successfully loaded collision models
            if (xprop_coll_meshes.contains(mdl_path)) {
                // Compute static prop's transformation matrix
                xprop_instance_data[mdl_path].push_back(InstanceData{
                    CalcModelTransformationMatrix(
                        sprop.origin, sprop.angles, sprop.uniform_scale)
                });
            }
        }
    }
    for (const BspMap::Ent_prop_dynamic& dprop : bsp_map->relevant_dynamic_props) {
        const auto& mdl_path = dprop.model;
        // We only care about dynamic props with successfully loaded collision models
        if (xprop_coll_meshes.contains(mdl_path)) {
            // Compute dynamic prop's transformation matrix
            xprop_instance_data[mdl_path].push_back(InstanceData{
                CalcModelTransformationMatrix(dprop.origin, dprop.angles, 1.0f)
            });
        }
    }

    for (auto& kv : xprop_instance_data) {
        const std::string& mdl_path = kv.first;
        std::vector<InstanceData>& instances = kv.second;
        GL::Mesh& mesh = xprop_coll_meshes[mdl_path];

        mesh.setInstanceCount(instances.size())
            .addVertexBufferInstanced(
                GL::Buffer{
                    GL::Buffer::TargetHint::Array,
                    std::move(instances)
                },
                1,
                0,
                GlidabilityShader3D::TransformationMatrix{}
                //, GlidabilityShader3D::Color3{} // other attributes are possible
        );

        r_world->instanced_xprop_meshes.emplace_back(std::move(mesh));
    }
    // Precompute collision caches of each solid prop (static or dynamic).
    // MUST HAPPEN AFTER COLL MODEL CREATION!
    Debug{} << "Creating collision caches of static props";
    // Keys are indices into BspMap::static_props, values are the caches.
    std::map<uint32_t, CollisionCache_XProp> coll_caches_sprop;
    for (size_t sprop_idx = 0; sprop_idx < bsp_map->static_props.size(); sprop_idx++) {
        const BspMap::StaticProp& sprop = bsp_map->static_props[sprop_idx];
        if (!sprop.IsSolidWithVPhysics())
            continue;

        // Path to ".mdl" file used by static prop
        const std::string& mdl_path = bsp_map->static_prop_model_dict[sprop.model_idx];

        auto coll_model_it = xprop_coll_models.find(mdl_path);
        if (coll_model_it == xprop_coll_models.end())
            continue; // No collision model
        const CollisionModel& cmodel = coll_model_it->second;

        auto sprop_coll_cache = coll::Create_CollisionCache_StaticProp(sprop, cmodel);
        if (sprop_coll_cache == Corrade::Containers::NullOpt)
            continue; // Cache creation failed
        coll_caches_sprop[sprop_idx] = std::move(*sprop_coll_cache);
    }
    Debug{} << "Creating collision caches of dynamic props";
    // Keys are indices into BspMap::relevant_dynamic_props, values are the caches.
    std::map<uint32_t, CollisionCache_XProp> coll_caches_dprop;
    for (size_t dprop_idx = 0; dprop_idx < bsp_map->relevant_dynamic_props.size(); dprop_idx++) {
        const BspMap::Ent_prop_dynamic& dprop = bsp_map->relevant_dynamic_props[dprop_idx];

        auto coll_model_it = xprop_coll_models.find(dprop.model);
        if (coll_model_it == xprop_coll_models.end())
            continue; // No collision model
        const CollisionModel& cmodel = coll_model_it->second;

        auto dprop_coll_cache = coll::Create_CollisionCache_DynamicProp(dprop, cmodel);
        if (dprop_coll_cache == Corrade::Containers::NullOpt)
            continue; // Cache creation failed
        coll_caches_dprop[dprop_idx] = std::move(*dprop_coll_cache);
    }



    // ----- BRUSHES
    Debug{} << "Parsing model brush indices";
    std::vector<std::set<size_t>> bmodel_brush_indices;
    for (size_t i = 0; i < bsp_map->models.size(); i++)
        bmodel_brush_indices.push_back(std::move(bsp_map->GetModelBrushIndices(i)));
    // bmodel at idx 0 is worldspawn, containing most map geometry
    // all other bmodels are tied to brush entities
    std::set<size_t>& worldspawn_brush_indices = bmodel_brush_indices[0];

    Debug{} << "Calculating func_brush rotation transformations";
    // Calculate rotation transformation for every SOLID func_brush entity, whose angles are not { 0, 0, 0 }
    // @Optimization Use 3x3 rotation matrices here, not 4x4, or quaternions
    std::map<const BspMap::Ent_func_brush*, Matrix4> func_brush_rot_transformations;
    for (size_t i = 0; i < bsp_map->entities_func_brush.size(); i++) {
        auto& func_brush = bsp_map->entities_func_brush[i];
        if (!func_brush.IsSolid()) continue;
        if (func_brush.angles[0] == 0.0f && func_brush.angles[1] == 0.0f && func_brush.angles[2] == 0.0f)
            continue;

        // Order of axis rotations is important! First roll, then pitch, then yaw rotation!
        func_brush_rot_transformations[&func_brush] =
            Matrix4::rotationZ(Deg{ func_brush.angles[1] }) * // (yaw)   rotation around z axis
            Matrix4::rotationY(Deg{ func_brush.angles[0] }) * // (pitch) rotation around y axis
            Matrix4::rotationX(Deg{ func_brush.angles[2] });  // (roll)  rotation around x axis
    }

    // Keep this list in the same order as the enum declaration, so that a brush
    // category can be identified by index
    std::vector<BrushSeparation::Category> bCategories = {
        BrushSeparation::Category::SOLID,
        BrushSeparation::Category::PLAYERCLIP,
        BrushSeparation::Category::GRENADECLIP,
        BrushSeparation::Category::LADDER,
        BrushSeparation::Category::WATER,
        BrushSeparation::Category::SKY
    };

    for (size_t i = 0; i < bCategories.size(); i++) {
        BrushSeparation::Category brushCat = bCategories[i];
        ZoneScopedN("parse brush cat");
        Debug{} << "Parsing brush category" << brushCat;

        auto testFuncs = BrushSeparation::getBrushCategoryTestFuncs(brushCat);
        std::vector<std::vector<Vector3>> faces =
            bsp_map->GetBrushFaceVertices(worldspawn_brush_indices, testFuncs.first,
                testFuncs.second);

        // Look for additional brushes from the current category in func_brush entities
        for (auto& func_brush : bsp_map->entities_func_brush) {
            if (!func_brush.IsSolid())
                continue;
            // Special case: grenadeclip brushes don't work in func_brush entities (for unknown reasons)
            if (brushCat == BrushSeparation::Category::GRENADECLIP)
                continue;

            if (func_brush.model.size() == 0 || func_brush.model[0] != '*') continue;
            std::string idxStr = func_brush.model.substr(1);
            int64_t modelIdx = utils::ParseIntFromString(idxStr, -1);
            if (modelIdx <= 0 || modelIdx >= (int64_t)bsp_map->models.size()) {
                error_msgs += "Failed to load func_brush at origin=("
                    + std::to_string((int64_t)func_brush.origin.x()) + ","
                    + std::to_string((int64_t)func_brush.origin.y()) + ","
                    + std::to_string((int64_t)func_brush.origin.z()) + "), "
                    "it has an invalid model idx.\n";
                continue;
            }

            auto& brush_indices = bmodel_brush_indices[modelIdx];
            auto faces_from_func_brush = bsp_map->GetBrushFaceVertices(brush_indices,
                testFuncs.first, testFuncs.second);
            if (faces_from_func_brush.size() == 0) continue;

            // Rotate and translate every vertex with func_brush's origin and angle
            bool is_func_brush_rotated =
                func_brush.angles[0] != 0.0f ||
                func_brush.angles[1] != 0.0f ||
                func_brush.angles[2] != 0.0f;
            Matrix4* rotTransformation = is_func_brush_rotated
                ? &func_brush_rot_transformations[&func_brush]
                : nullptr;
            for (auto& face : faces_from_func_brush) {
                for (auto& v : face) {
                    // Rotate vertex if func_brush has a non-zero angle
                    if (is_func_brush_rotated)
                        // Rotate point around origin
                        v = (*rotTransformation).transformVector(v);
                    // Translate point
                    v += func_brush.origin;
                }
            }
            // Append new faces
            faces.insert(faces.end(),
                std::make_move_iterator(faces_from_func_brush.begin()),
                std::make_move_iterator(faces_from_func_brush.end()));
        }

        // Remove all water faces that are not facing upwards. We draw water
        // with transparency, so we dont want water faces other than those
        // representing the water surface
        if (brushCat == BrushSeparation::Category::WATER) {
            std::vector<std::vector<Vector3>> water_surface_faces;
            for (auto& face : faces) {
                // faces have clockwise vertex winding
                if (IsCwTriangleFacingUp(face[0], face[1], face[2]))
                    water_surface_faces.push_back(std::move(face));
            }
            faces = std::move(water_surface_faces);
        }

        r_world->brush_category_meshes[brushCat] =
            GenMeshWithVertAttr_Position_Normal(faces);
    }

    // ----- trigger_push BRUSHES (only use those that push players)
    std::vector<std::vector<Vector3>> trigger_push_faces;
    for (const auto& trigger_push : bsp_map->entities_trigger_push) {
        if (!trigger_push.CanPushPlayers())
            continue;
        if (trigger_push.model.size() == 0 || trigger_push.model[0] != '*')
            continue;
        std::string idx_str = trigger_push.model.substr(1);
        int64_t model_idx = utils::ParseIntFromString(idx_str, -1);
        if (model_idx <= 0 || model_idx >= (int64_t)bsp_map->models.size()) {
            error_msgs += "Failed to load trigger_push at origin=("
                + std::to_string((int64_t)trigger_push.origin.x()) + ","
                + std::to_string((int64_t)trigger_push.origin.y()) + ","
                + std::to_string((int64_t)trigger_push.origin.z()) + "), "
                "it has an invalid model idx.\n";
            continue;
        }
        auto& brush_indices = bmodel_brush_indices[model_idx];
        auto faces_from_trigger_push = bsp_map->GetBrushFaceVertices(brush_indices);
        if (faces_from_trigger_push.size() == 0) continue;

        // Rotate and translate model of trigger_push.
        // Elevate non-ladder push triggers above water surface to fix
        // Z-fighting with the water. Downside: These push triggers are drawn
        // slightly in the wrong position. Don't elevate ladder push triggers,
        // just in case someone needs to look at them very precisely.
        Vector3 z_fighting_resolver = trigger_push.only_falling_players ?
            Vector3{ 0.0f, 0.0f, 0.0f } : Vector3{ 0.0f, 0.0f, 1.0f };

        Matrix4 trigger_push_transf = CalcModelTransformationMatrix(
            trigger_push.origin + z_fighting_resolver,
            trigger_push.angles
        );
        for (auto& face : faces_from_trigger_push)
            for (auto& v : face)
                v = trigger_push_transf.transformPoint(v);

        trigger_push_faces.insert(trigger_push_faces.end(),
            std::make_move_iterator(faces_from_trigger_push.begin()),
            std::make_move_iterator(faces_from_trigger_push.end()));
    }
    r_world->trigger_push_meshes =
        GenMeshWithVertAttr_Position_Normal(trigger_push_faces);


    // Create CollidableWorld object and move all collision structures into it.
    std::shared_ptr<CollidableWorld> c_world = std::make_shared<CollidableWorld>(bsp_map);
    c_world->pImpl->hull_disp_coll_trees = std::move(hull_disp_coll_trees);
    c_world->pImpl->xprop_coll_models    = std::move(xprop_coll_models);
    c_world->pImpl->coll_caches_sprop    = std::move(coll_caches_sprop);
    c_world->pImpl->coll_caches_dprop    = std::move(coll_caches_dprop);
    // ...

    // BVH must be created *after* all other collision structures were created
    // and moved into the CollidableWorld object!
    assert(c_world->pImpl->hull_disp_coll_trees != Corrade::Containers::NullOpt);
    assert(c_world->pImpl->xprop_coll_models    != Corrade::Containers::NullOpt);
    assert(c_world->pImpl->coll_caches_sprop    != Corrade::Containers::NullOpt);
    assert(c_world->pImpl->coll_caches_dprop    != Corrade::Containers::NullOpt);
    // ...
    c_world->pImpl->bvh = BVH(*c_world);


    if (dest_errors)
        *dest_errors = std::move(error_msgs);
    return { r_world, c_world };
}
