#ifndef COLL_COLLIDABLEWORLD_STATICPROP_H_
#define COLL_COLLIDABLEWORLD_STATICPROP_H_

#include <vector>

#include <Corrade/Containers/Optional.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/Vector3.h>

#include "csgo_parsing/BspMap.h"
#include "utils_3d.h"

namespace coll {

// A collision model consists of a list of sections. A section is a list of
// triangles that describe a convex shape.
struct CollisionModel {
    // NOTE: Collision models might have *slightly* concave sections!
    //       Effects of this are unknown.

    // Section indexing is identical between section_tri_meshes, section_planes,
    // and section_aabbs.

    // Triangle mesh of each (convex) section.
    // 2024-02-01:
    //   Properties of these TriMesh objects parsed from CSGO's sprop PHYs:
    //     - 'edges' array holds unique edges (GUARANTEED)
    //     - 'tris' array likely holds unique tris (not guaranteed)
    //     - 'vertices' array likely holds unique verts (not guaranteed)
    std::vector<utils_3d::TriMesh> section_tri_meshes;

    // For the sake of precomputation, we store each section's planes and AABBs
    // as well.

    // Planes of the triangles of each (convex) section.
    std::vector<std::vector<csgo_parsing::BspMap::Plane>> section_planes;

    // AABB of each (convex) section. Note that these are different from AABBs
    // of sprop sections, since sprop sections have been scaled, rotated and
    // translated.
    struct AABB { Magnum::Vector3 mins, maxs; };
    std::vector<AABB> section_aabbs;
};


// Lookup table used by SPropSectionBevelPlaneGenerator
class SPropSectionBevelPlaneLut {
public:
    // Creates LUT, expensive.
    SPropSectionBevelPlaneLut(
        const Magnum::Matrix3&    sprop_rotationscaling,
        const Magnum::Quaternion& sprop_inv_rotation, // Must be normalized!
        float                     sprop_inv_scale,
        const utils_3d::TriMesh& tri_mesh_of_sprop_section,
        const std::vector<csgo_parsing::BspMap::Plane>& planes_of_sprop_section);

    size_t GetMemorySize() const;

private:
    // Essentially, this LUT represents the information of whether a 'bevel
    // plane candidate' (identified by its index OR generation parameters) is
    // valid or not (i.e. needed for hull traces against this static prop section).

    struct CandidateGenParams { // How this bevel plane candidate is generated
        size_t unique_edge_idx; // What edge this bevel plane is generated on
        int axis; // What axis the bevel plane is aligned to: 0(X), 1(Y) or 2(Z)
        int dir;  // Which side, i.e. negation of bevel plane normal: 1 or -1
    };

    // The LUT represents a list of indices of valid candidates. A candidate
    // index maps to its generation parameters, and the other way around.
    // LUT layout:
    //  candidate idx =>  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17, ...
    //           edge => |<--- EDGE 0 --->||<--- EDGE 1 --->||<--- EDGE 2 --->| ...
    //     axis & dir => |X -X  Y -Y  Z -Z||X -X  Y -Y  Z -Z||X -X  Y -Y  Z -Z| ...

    // Deduce candidate generation parameters from candidate index
    static CandidateGenParams GetGenParams(size_t candidate_idx) {
        // @Optimization Should we add padding to the LUT layout to avoid some
        //               expensive divisions in here?
        size_t unique_edge_idx = candidate_idx / 6;
        int    axis            = (candidate_idx / 2) % 3;
        int    dir             = (candidate_idx % 2 == 0) ? 1 : -1;
        return { .unique_edge_idx = unique_edge_idx, .axis = axis, .dir = dir };
    }
    // Deduce candidate index from candidate generation parameters
    static size_t GetCandidateIdx(const CandidateGenParams& gen_params) {
        size_t candidate_idx = 0;
        candidate_idx += gen_params.unique_edge_idx * 6;
        candidate_idx += gen_params.axis * 2;
        candidate_idx += (gen_params.dir == 1) ? 0 : 1;
        return candidate_idx;
    }

    // The LUT is not stored directly as a list of valid candidate indices but
    // instead as a list of the step sizes between those indices, and using the
    // 'recursive indexing' technique.
    // See the LUT creation code for an explanation of this.
    // 2024-02-01 Note: This elementary 'recursive index' type's size and the
    //                  LUT's layout were chosen to optimize memory usage and
    //                  lookup time when used for static props as found inside
    //                  CSGO DZ maps.
    using RecIdxType = uint8_t; // 'Recursive indexing' int type
    std::vector<RecIdxType> valid_candidate_index_steps_recidx; // <- LUT representation

    friend class SPropSectionBevelPlaneGenerator;
};

// Precomputed data per static prop to speed up collision calculations
// Note: Up to ~10000 static props in a CSGO map have been encountered.
// Note: Up to 160000 total static prop sections in a CSGO map have been
//       encountered.
struct CollisionCache_StaticProp {
    // Transformation data
    Magnum::Quaternion inv_rotation; // Normalized. Reverses sprop rotation
    float              inv_scale;    // (1 / scale)

    // Exact, non-bloated AABB of each section of this static prop.
    // Note that these are different from a CollisionModel's section AABBs!
    // CollisionModel's section AABBs describe the bounds of the unscaled,
    // unrotated and untranslated collision model sections.
    // The CollisionCache_StaticProp's section AABBs describe the bounds of the
    // scaled, rotated and translated static prop sections in the world.
    // @Optimization Alternative section AABB representation 1: Store inv_rotated
    //               unit_vec_* vectors (See sprop trace code) of sprop.
    //               Use those as section AABB plane normals and then store
    //               plane dists (6 floats) for each section. I think switching
    //               to this makes some sprop trace code faster and some slower.
    // @Optimization Alternative section AABB representation 2: To save memory,
    //               store section AABB as 6 vertex indices (uint16) pointing
    //               into the section's triangle mesh. They mark the vertices
    //               that are the section's furthest vertices in +X, -X, +Y, -Y,
    //               +Z and -Z direction. Probably worsens trace performance.
    struct AABB { Magnum::Vector3 mins, maxs; };
    std::vector<AABB> section_aabbs;

    // Bevel plane LUT of each section of this static prop
    // @Optimization Memory: There are possibly a number of duplicate LUTs in here
    std::vector<SPropSectionBevelPlaneLut> section_bevel_luts;
};

// Returns an empty Optional if collision cache creation fails.
Corrade::Containers::Optional<CollisionCache_StaticProp>
    Create_CollisionCache_StaticProp(
        const csgo_parsing::BspMap::StaticProp& sprop,
        const CollisionModel& cmodel);


// Responsible for efficiently generating all bevel planes of a specific section
// of a specific static prop. Bevel planes are calculated on demand.
class SPropSectionBevelPlaneGenerator {
public:
    // Inits this class to generate all bevel planes of a specific section of a
    // specific static prop.
    // CAUTION: The passed collision model must be the one that was used to
    //          create the passed collision cache!
    // CAUTION: The passed collision model and collision cache must persist in
    //          memory without modifications as long as you use this
    //          SPropSectionBevelPlaneGenerator instance!
    SPropSectionBevelPlaneGenerator(
        const CollisionModel&            sprop_coll_model,
        const CollisionCache_StaticProp& sprop_coll_cache,
        size_t idx_of_sprop_section);

    // If successful, sets plane to next bevel plane and returns true.
    // If no more bevel planes can be generated, returns false.
    bool GetNext(csgo_parsing::BspMap::Plane* out);

private:
    // Current position of the bevel plane generator
    size_t cur_candidate_idx;
    size_t cur_lut_pos;

    // Stored info for generation
    const Magnum::Quaternion sprop_inv_rotation; // Normalized
    const utils_3d::TriMesh& tri_mesh_of_sprop_section;
    const std::vector<SPropSectionBevelPlaneLut::RecIdxType>&
                                             valid_candidate_index_steps_recidx;
};

} // namespace coll

#endif // COLL_COLLIDABLEWORLD_STATICPROP_H_
