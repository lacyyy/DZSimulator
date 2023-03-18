#ifndef RENDERING_WORLDCREATION_H_
#define RENDERING_WORLDCREATION_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>

#include "csgo_parsing/BrushSeparation.h"
#include "csgo_parsing/BspMap.h"
#include "csgo_parsing/utils.h"

namespace rendering {

    // Map-specific data container for convenient deallocation
    struct CsgoMapGeometry {
        // Mesh for each brush category
        std::map<csgo_parsing::BrushSeparation::Category, Magnum::GL::Mesh> brush_category_meshes;

        // Mesh of all trigger_push entities that can push players
        Magnum::GL::Mesh trigger_push_meshes { Magnum::NoCreate };

        // Displacement meshes
        Magnum::GL::Mesh mesh_displacements { Magnum::NoCreate };
        Magnum::GL::Mesh mesh_displacement_boundaries { Magnum::NoCreate };

        // Collision model meshes of solid static props
        std::vector<Magnum::GL::Mesh> instanced_static_prop_meshes;

        // Mesh for predicted player trajectory
        Magnum::GL::Mesh unit_trajectory_mesh;
    };

    // Namespace that is mainly responsible for GL::Mesh creation
    namespace WorldCreation {

        // Mainly creates all GL::Mesh objects from a parsed CSGO bsp map file.
        // Error messages are put into the string pointed to by dest_errors.
        std::unique_ptr<CsgoMapGeometry> CreateCsgoMapGeometry(
            std::shared_ptr<const csgo_parsing::BspMap> bsp_map,
            std::string* dest_errors = nullptr);

        // Returned code is either SUCCESS or ERROR_PHY_PARSING_FAILED.
        // ERROR_PHY_PARSING_FAILED has a description string.
        csgo_parsing::utils::RetCode CreatePhyModelMeshFromGameFile(
            Magnum::GL::Mesh* dest,
            const std::string& src_phy_path);

        // Returned code is either SUCCESS or ERROR_PHY_PARSING_FAILED.
        // ERROR_PHY_PARSING_FAILED has a description string.
        csgo_parsing::utils::RetCode CreatePhyModelMeshFromPackedPhyFile(
            Magnum::GL::Mesh* dest,
            const std::string& abs_bsp_file_path, // absolute bsp map file path
            size_t packed_phy_file_pos, // start position inside bsp file
            size_t packed_phy_file_len); // phy file byte count

        // Mesh of Bump Mines thrown/placed into the world
        Magnum::GL::Mesh CreateBumpMineMesh();



        // -----------------------------------
        // -------- GL::Mesh creation --------
        // -----------------------------------

        // From given faces, create a GL::Mesh with vertex buffer with attributes:
        //   - Vertex Position ( Magnum::Shaders::GenericGL3D::Position )
        Magnum::GL::Mesh GenMeshWithVertAttr_Position(
            const std::vector<std::vector<Magnum::Vector3>>& faces);

        // From given faces, create a GL::Mesh with vertex buffer with attributes:
        //   - Vertex Position ( Magnum::Shaders::GenericGL3D::Position )
        //   - Vertex Normal   ( Magnum::Shaders::GenericGL3D::Normal )
        Magnum::GL::Mesh GenMeshWithVertAttr_Position_Normal(
            const std::vector<std::vector<Magnum::Vector3>>& faces);

    }
}

#endif // RENDERING_WORLDCREATION_H_
