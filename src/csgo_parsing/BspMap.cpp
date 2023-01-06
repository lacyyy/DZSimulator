#include "BSPMap.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <set>
#include <iterator>
#include <functional>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector4.h>
#include <Magnum/Math/Distance.h>
#include <Magnum/Math/Intersection.h>

#include "utils_3d.h"

using namespace Magnum;
using namespace csgo_parsing;
using namespace utils_3d;

BspMap::BspMap(const std::string& abs_file_path)
    : abs_bsp_file_path(abs_file_path)
    , world_mins{ -INFINITY, -INFINITY, -INFINITY }
    , world_maxs{ INFINITY, INFINITY, INFINITY }
    , map_version(-1)
{
}

// Taken from source-sdk-2013/<...>/src/public/bspfile.h
bool BspMap::DispTri::HasTag_SURFACE()     const { return tags & (1<<0); }
bool BspMap::DispTri::HasTag_WALKABLE()    const { return tags & (1<<1); } 
bool BspMap::DispTri::HasTag_BUILDABLE()   const { return tags & (1<<2); }
bool BspMap::DispTri::HasFlag_SURFPROP1()  const { return tags & (1<<3); }
bool BspMap::DispTri::HasFlag_SURFPROP2()  const { return tags & (1<<4); }
bool BspMap::DispTri::HasTag_REMOVE()      const { return tags & (1<<5); }
// end of source-sdk-2013 code

// Taken from source-sdk-2013/<...>/src/public/bspflags.h
bool BspMap::TexInfo::HasFlag_LIGHT()      const { return flags & ((uint32_t)1 <<  0); }
bool BspMap::TexInfo::HasFlag_SKY2D()      const { return flags & ((uint32_t)1 <<  1); }
bool BspMap::TexInfo::HasFlag_SKY()        const { return flags & ((uint32_t)1 <<  2); }
bool BspMap::TexInfo::HasFlag_WARP()       const { return flags & ((uint32_t)1 <<  3); }
bool BspMap::TexInfo::HasFlag_TRANS()      const { return flags & ((uint32_t)1 <<  4); }
bool BspMap::TexInfo::HasFlag_NOPORTAL()   const { return flags & ((uint32_t)1 <<  5); }
bool BspMap::TexInfo::HasFlag_TRIGGER()    const { return flags & ((uint32_t)1 <<  6); }
bool BspMap::TexInfo::HasFlag_NODRAW()     const { return flags & ((uint32_t)1 <<  7); }
bool BspMap::TexInfo::HasFlag_HINT()       const { return flags & ((uint32_t)1 <<  8); }
bool BspMap::TexInfo::HasFlag_SKIP()       const { return flags & ((uint32_t)1 <<  9); }
bool BspMap::TexInfo::HasFlag_NOLIGHT()    const { return flags & ((uint32_t)1 << 10); }
bool BspMap::TexInfo::HasFlag_BUMPLIGHT()  const { return flags & ((uint32_t)1 << 11); }
bool BspMap::TexInfo::HasFlag_NOSHADOWS()  const { return flags & ((uint32_t)1 << 12); }
bool BspMap::TexInfo::HasFlag_NODECALS()   const { return flags & ((uint32_t)1 << 13); }
bool BspMap::TexInfo::HasFlag_NOCHOP()     const { return flags & ((uint32_t)1 << 14); }
bool BspMap::TexInfo::HasFlag_HITBOX()     const { return flags & ((uint32_t)1 << 15); }
// end of source-sdk-2013 code

bool BspMap::Brush::HasFlags(uint32_t flags) const { return contents & flags; }


// Return true if 2 vertices are so close together they should be considered the same
bool BspMap::AreVerticesEquivalent(const Vector3& a, const Vector3& b)
{
    const Magnum::Float EPSILON = 1.0e-05f;
    const Magnum::Float EPSILON_SQUARED = EPSILON * EPSILON;

    Float distanceSquared = (b - a).dot();

    if (distanceSquared <= EPSILON_SQUARED)
        return true;
    return distanceSquared <= EPSILON_SQUARED * std::max(a.dot(), b.dot());
}

std::vector<Magnum::Vector3> BspMap::GetFaceVertices(uint32_t face_idx) const
{
    std::vector<Magnum::Vector3> vertexListCW;
    BspMap::Face f = this->faces[face_idx];
    // Faces are described in clockwise order when looked at from the front!
    for (size_t i = 0; i < f.num_edges; ++i) {
        int32_t surfedge = this->surfedges[f.first_edge + i];
        if (surfedge > 0) vertexListCW.push_back(this->vertices[this->edges[ surfedge].v[0]]);
        else              vertexListCW.push_back(this->vertices[this->edges[-surfedge].v[1]]);
    }
    return vertexListCW;
}

// Returns displacement's vertices row by row (or column by column)
std::vector<Magnum::Vector3> BspMap::GetDisplacementVertices(size_t disp_info_idx) const
{
    const BspMap::DispInfo& dispinfo = this->dispinfos[disp_info_idx];
    size_t num_row_verts = ((size_t)1 << dispinfo.power) + 1; // verts in one row
    size_t num_verts = num_row_verts * num_row_verts;
    //const size_t num_tris = 2 * ((size_t)1 << dispinfo.power) * ((size_t)1 << dispinfo.power);

    BspMap::Face face = this->faces[dispinfo.map_face];
    if (face.num_edges != 4) {
        Error{} << "[ERR] BspMap::getDisplacementVertices() dispinfo.map_face.num_edges =" << face.num_edges << "!= 4";
        return {};
    }

    std::vector<Vector3> mapFaceVertListCW;
    for (size_t i = 0; i < face.num_edges; ++i) {
        int32_t surfedge = this->surfedges[face.first_edge + i];
        if (surfedge > 0) mapFaceVertListCW.push_back(this->vertices[this->edges[surfedge].v[0]]);
        else              mapFaceVertListCW.push_back(this->vertices[this->edges[-surfedge].v[1]]);
    }

    // Find index of map_face vertex that is the closest to dispinfo.start_pos
    size_t idx_startPosVert = 0;
    Float dist_startPosVert = Math::Distance::pointPointSquared(dispinfo.start_pos, mapFaceVertListCW[0]);
    for (size_t i = 1; i < mapFaceVertListCW.size(); ++i) {
        Float dist = Math::Distance::pointPointSquared(dispinfo.start_pos, mapFaceVertListCW[i]);
        if (dist < dist_startPosVert) {
            idx_startPosVert = i;
            dist_startPosVert = dist;
        }
    }

    Vector3& mapFaceVertTopLeft = mapFaceVertListCW[(idx_startPosVert + 3) % mapFaceVertListCW.size()];
    Vector3& mapFaceVertTopRight = mapFaceVertListCW[(idx_startPosVert + 0) % mapFaceVertListCW.size()];
    Vector3& mapFaceVertBotRight = mapFaceVertListCW[(idx_startPosVert + 1) % mapFaceVertListCW.size()];
    Vector3& mapFaceVertBotLeft = mapFaceVertListCW[(idx_startPosVert + 2) % mapFaceVertListCW.size()];

    std::vector<Vector3> verts(num_verts);
    for (size_t i = 0; i < num_verts; ++i) {
        // Calc flat displacement vertex position
        Float rowPos = (Float)(i % num_row_verts) / (Float)(num_row_verts - 1);
        Float colPos = (Float)(i / num_row_verts) / (Float)(num_row_verts - 1);
        Vector3 top_interp = (1.0f - rowPos) * mapFaceVertTopLeft + (rowPos)*mapFaceVertTopRight;
        Vector3 bot_interp = (1.0f - rowPos) * mapFaceVertBotLeft + (rowPos)*mapFaceVertBotRight;
        verts[i] = (1.0f - colPos) * top_interp + (colPos)*bot_interp;

        // Invert left/right dispvert read direction (bsp file just counts differently than us)
        size_t idx_rowPos = i % num_row_verts;
        size_t i_inv = i - idx_rowPos + ((num_row_verts - 1) - idx_rowPos);
        // Add offset
        verts[i] += this->dispverts[dispinfo.disp_vert_start + i_inv].dist * this->dispverts[dispinfo.disp_vert_start + i_inv].vec;
    }

    return verts;
}

std::vector<std::vector<Magnum::Vector3>> BspMap::GetDisplacementFaceVertices() const
{
    std::vector<std::vector<Magnum::Vector3>> finalFaces; // will be returned in the end

    for (size_t i = 0; i < this->dispinfos.size(); ++i) {
        const BspMap::DispInfo& dispinfo = this->dispinfos[i];

        if (dispinfo.HasFlag_NO_HULL_COLL()) // Skip if not solid to player and bump mines
            continue;

        size_t num_row_verts = ((size_t)1 << dispinfo.power) + 1; // verts in one row

        std::vector<Vector3> verts = GetDisplacementVertices(i);

        for (size_t tileY = 0; tileY < ((size_t)1 << dispinfo.power); ++tileY) {
            for (size_t tileX = 0; tileX < ((size_t)1 << dispinfo.power); ++tileX) {

                Vector3 vertTopLeft  = verts[(tileY    ) * num_row_verts + (tileX    )];
                Vector3 vertBotLeft  = verts[(tileY + 1) * num_row_verts + (tileX    )];
                Vector3 vertBotRight = verts[(tileY + 1) * num_row_verts + (tileX + 1)];
                Vector3 vertTopRight = verts[(tileY    ) * num_row_verts + (tileX + 1)];

                // Switch up triangle seperating diagonal each tile
                std::vector<Vector3> triangle1, triangle2;
                if ((tileX + tileY) % 2 == 0) {
                    triangle1 = { vertTopLeft, vertBotRight, vertBotLeft  };
                    triangle2 = { vertTopLeft, vertTopRight, vertBotRight };
                } else {
                    triangle1 = { vertTopLeft, vertTopRight, vertBotLeft  };
                    triangle2 = { vertBotLeft, vertTopRight, vertBotRight };
                }
                finalFaces.push_back(std::move(triangle1));
                finalFaces.push_back(std::move(triangle2));
            }
        }
    }
    
    return finalFaces;
}

// OPTIMIZE: Merge boundary faces that can be merged (e.g. in displacement walls)
std::vector<std::vector<Magnum::Vector3>> BspMap::GetDisplacementBoundaryFaceVertices() const
{
    const float EPSILON = 0.00001f; // Min vector length allowed for normalization
    // By how much the boundary faces are placed above the displacement faces
    const float BOUNDARY_HOVER_DIST = 2.0f;
    // Ratio of boundary width to displacement tile width
    const float BOUNDARY_THICKNESS = 0.1f; // between 0.0 and 1.0

    std::vector<std::vector<Magnum::Vector3>> total_faces; // will be returned in the end

    for (size_t disp_idx = 0; disp_idx < this->dispinfos.size(); disp_idx++) {
        const BspMap::DispInfo& dispinfo = this->dispinfos[disp_idx];

        if (dispinfo.HasFlag_NO_HULL_COLL()) // Skip if not solid to player and bump mines
            continue;

        size_t num_row_verts = ((size_t)1 << dispinfo.power) + 1; // verts in one row

        std::vector<Vector3> verts = GetDisplacementVertices(disp_idx);

        // Outermost vertex line of each of the 4 displacement sides
        std::vector<std::vector<Vector3>> first_outer_edge_lines{ 4 };
        // Second outermost vertex line of each of the 4 displacement sides
        std::vector<std::vector<Vector3>> secnd_outer_edge_lines{ 4 };

        // [Side idx 0] Add vertices of top row (from left to right)
        for (size_t i = 0; i < num_row_verts; i++) {
            size_t idx_outermost_vert = i;
            first_outer_edge_lines[0].push_back(verts[idx_outermost_vert]);
            secnd_outer_edge_lines[0].push_back(verts[idx_outermost_vert + num_row_verts]);
        }
        // [Side idx 1] Add vertices of right column (from top to bottom)
        for (size_t i = 0; i < num_row_verts; i++) {
            size_t idx_outermost_vert = num_row_verts - 1 + i * num_row_verts;
            first_outer_edge_lines[1].push_back(verts[idx_outermost_vert]);
            secnd_outer_edge_lines[1].push_back(verts[idx_outermost_vert - 1]);
        }
        // [Side idx 2] Add vertices of bottom row (from right to left)
        for (size_t i = 0; i < num_row_verts; i++) {
            size_t idx_outermost_vert = num_row_verts * num_row_verts - 1 - i;
            first_outer_edge_lines[2].push_back(verts[idx_outermost_vert]);
            secnd_outer_edge_lines[2].push_back(verts[idx_outermost_vert - num_row_verts]);
        }
        // [Side idx 3] Add vertices of left column (from bottom to top)
        for (size_t i = 0; i < num_row_verts; i++) {
            size_t idx_outermost_vert = (num_row_verts - 1) * num_row_verts - i * num_row_verts;
            first_outer_edge_lines[3].push_back(verts[idx_outermost_vert]);
            secnd_outer_edge_lines[3].push_back(verts[idx_outermost_vert + 1]);
        }

        // Go along displacement side vertices and build the boundary mesh.
        // Displacement side tile count is a multiple of 2. The triangles have
        // clockwise vertex winding order and are formed as follows
        // (each 'X' represents a vertex):
        //
        //     vertex index -> [0]   [1]   [2]   [3]   [4]   [5]   [6]   [7]   [8]
        // first outermost  ->  X --- X --- X --- X --- X --- X --- X --- X --- X  
        //   vertex line        | \ooo|ooo/ | \ooo|ooo/ | \ooo|ooo/ | \ooo|ooo/ |
        //                      |  \oo|oo/  |  \oo|oo/  |  \oo|oo/  |  \oo|oo/  |
        // second outermost     |   \o|o/   |   \o|o/   |   \o|o/   |   \o|o/   |  
        //   vertex line    ->  X --- X --- X --- X --- X --- X --- X --- X --- X  
        
        for (size_t side = 0; side < 4; side++) {
            std::vector<Vector3>& first_outermost_vertices = first_outer_edge_lines[side];
            std::vector<Vector3>& secnd_outermost_vertices = secnd_outer_edge_lines[side];

            // Calculate normals of triangles that have the displacement's
            // outermost edges (triangles filled with 'o' in the comment above)
            std::vector<Vector3> outermost_edge_normals;
            outermost_edge_normals.reserve(num_row_verts);
            for (size_t tri_idx = 0; tri_idx < num_row_verts - 1; tri_idx++) {
                outermost_edge_normals.push_back(CalcNormalCwFront(
                    first_outermost_vertices[tri_idx],
                    first_outermost_vertices[tri_idx + 1],
                    secnd_outermost_vertices[1 + (size_t)(tri_idx / 2) * 2]));
            }

            // Determine boundary mesh vertices that roughly follow the
            // displacement side's edge
            std::vector<std::vector<Vector3>> boundary_mesh_vertices{ 2 };

            // Calculate outer vertex line of the boundary mesh
            for (size_t i = 0; i < num_row_verts; i++) {
                Vector3 v = first_outermost_vertices[i];

                Vector3 offset_dir = { 0.0f, 0.0f, 0.0f };
                if (i != 0)
                    offset_dir += outermost_edge_normals[i - 1];
                if (i != num_row_verts - 1)
                    offset_dir += outermost_edge_normals[i];
                if(offset_dir.length() > EPSILON)
                    v += BOUNDARY_HOVER_DIST * offset_dir.normalized();

                boundary_mesh_vertices[0].push_back(v);
            }
            // Calculate inner vertex line of the boundary mesh
            for (size_t i = 0; i < num_row_verts; i++) {
                Vector3 v = first_outermost_vertices[i];
                
                // Offset is the average of the adjacent triangles' normals
                Vector3 offset_dir = { 0.0f, 0.0f, 0.0f };
                if (i != 0)
                    offset_dir += outermost_edge_normals[i - 1];
                if (i != num_row_verts - 1)
                    offset_dir += outermost_edge_normals[i];
                if (offset_dir.length() > EPSILON)
                    v += BOUNDARY_HOVER_DIST * offset_dir.normalized();

                Vector3 true_inwards_vec = (secnd_outermost_vertices[i] -
                                            first_outermost_vertices[i]);
                Vector3 inwards_vec = { 0.0f, 0.0f, 0.0f };
                if (i % 2 == 1) {
                    inwards_vec = true_inwards_vec;
                }
                else {
                    if (i != 0) {
                        Vector3 tmp = Math::cross((first_outermost_vertices[i] -
                                                   first_outermost_vertices[i - 1]),
                                                  outermost_edge_normals[i - 1]);
                        if (tmp.length() > EPSILON)
                            inwards_vec += tmp.normalized();
                    }
                        
                    if (i != num_row_verts - 1) {
                        Vector3 tmp = Math::cross((first_outermost_vertices[i + 1] -
                                                   first_outermost_vertices[i]),
                                                  outermost_edge_normals[i]);
                        if (tmp.length() > EPSILON)
                            inwards_vec += tmp.normalized();
                    }

                    if (inwards_vec.length() > EPSILON) {
                        inwards_vec = inwards_vec.normalized();
                        inwards_vec *= true_inwards_vec.length();
                    }
                    else {
                        inwards_vec = { 0.0f, 0.0f, 0.0f };
                    }
                }
                v += BOUNDARY_THICKNESS * inwards_vec;

                boundary_mesh_vertices[1].push_back(v);
            }

            // Make boundary mesh vertices into triangles
            for (size_t tile = 0; tile < num_row_verts - 1; tile++) {
                total_faces.push_back({
                    boundary_mesh_vertices[0][tile],
                    boundary_mesh_vertices[1][tile + 1],
                    boundary_mesh_vertices[1][tile] });
                total_faces.push_back({
                    boundary_mesh_vertices[0][tile],
                    boundary_mesh_vertices[0][tile + 1],
                    boundary_mesh_vertices[1][tile + 1] });
            }
        }
    }

    return total_faces;
}

// FIXME Look at
// https://github.com/ValveSoftware/source-sdk-2013/blob/0d8dceea4310fde5706b3ce1c70609d72a38efdf/sp/src/utils/vbsp/brushbsp.cpp#L1048
// to finally fix incorrect brush faces?!
// is this useful? https://github.com/magcius/noclip.website
// Use Plane type here?! Might be useful for brush vertex parsing:
//      https://developer.valvesoftware.com/wiki/Source_BSP_File_Format#Plane
// Returns faces with clockwise vertex winding
std::vector<std::vector<Magnum::Vector3>> BspMap::GetBrushFaceVertices(const std::set<size_t>& brush_indices,
    bool (*pred_Brush)(const Brush&),
    bool (*pred_BrushSide)(const BrushSide&, const BspMap&)) const
{
    std::vector<std::vector<Magnum::Vector3>> finalFaces; // This vector will be returned

    // TODO Optimization: delete redundant vertices that are on the line from the last to to the next vertex
    // TODO Observe vertex drifts, adjust overcut
    // TODO Remove redundant faces inside other faces?

    // When checking what vertices fall behind a plane, vertices on the plane are
    // treated pretty much randomly (float inaccuracy). Therefore cut a fraction
    // more behind the plane and then connect edges back up exactly with the plane.
    const Float BRUSH_PLANE_OVER_CUT = 0.001f; // observed intersection floating point drift: 0.000488281

    // How much a plane at least has to cut to not be considered redundant and skipped
    const Float BRUSH_PLANE_REDUNDANT_CUT_SIZE = 0.01f; // must be greater than zero

    // Our coordinate system follows the right hand rule: thumb = +X, index finger = +Y, middle finger = +Z
    const float MAX_P = BspMap::MAX_BRUSH_LENGTH / 2; // max positive vertex position
    const float EXTREME_THRESH = MAX_P * 0.98f;
    std::vector<std::vector<Vector3>> hugeCube = { // cube faces with clockwise vertex winding
    { { MAX_P, MAX_P, MAX_P}, { MAX_P,-MAX_P, MAX_P}, {-MAX_P,-MAX_P, MAX_P}, {-MAX_P, MAX_P, MAX_P} },  // face facing +Z
    { {-MAX_P, MAX_P,-MAX_P}, {-MAX_P,-MAX_P,-MAX_P}, { MAX_P,-MAX_P,-MAX_P}, { MAX_P, MAX_P,-MAX_P} },  // face facing -Z
    { { MAX_P,-MAX_P, MAX_P}, { MAX_P, MAX_P, MAX_P}, { MAX_P, MAX_P,-MAX_P}, { MAX_P,-MAX_P,-MAX_P} },  // face facing +X
    { {-MAX_P,-MAX_P,-MAX_P}, {-MAX_P, MAX_P,-MAX_P}, {-MAX_P, MAX_P, MAX_P}, {-MAX_P,-MAX_P, MAX_P} },  // face facing -X
    { { MAX_P, MAX_P, MAX_P}, {-MAX_P, MAX_P, MAX_P}, {-MAX_P, MAX_P,-MAX_P}, { MAX_P, MAX_P,-MAX_P} },  // face facing +Y
    { { MAX_P,-MAX_P,-MAX_P}, {-MAX_P,-MAX_P,-MAX_P}, {-MAX_P,-MAX_P, MAX_P}, { MAX_P,-MAX_P, MAX_P} } };// face facing -Y

    // To check vertices for equivalence, their extreme components must be removed first
    std::function<Vector3(const Vector3&)> getVertexNonExtremeComponents = [EXTREME_THRESH](const Vector3& v) {
        Vector3 result(v); // Copy v
        for (int i = 0; i < 3; ++i) {
            if (result[i] > EXTREME_THRESH)
                result[i] = 1.0f;
            else if (result[i] < -EXTREME_THRESH)
                result[i] = -1.0f;
        }
        return result;
    };

    // Function to check if vertex is cut by plane or not
    // Plane vectors point OUT of the brush, not into it
    std::function<bool(Vector3&, BspMap::Plane&, Float)> isVertexBehindPlane = [](Vector3& v, BspMap::Plane& p, Float overcut) {
        return Math::dot(v, p.normal) - p.dist < -overcut;
    };

    for (size_t brush_idx : brush_indices) {
        const BspMap::Brush& brush = this->brushes[brush_idx];

        if (pred_Brush) // If Brush predicate function was provided
            if (!pred_Brush(brush)) // Skip brush if it doesn't fit the predicate
                continue;

        // Initially a cube bigger than the whole level
        std::vector<std::vector<Vector3>> brushFaces(hugeCube); // Copy hugeCube vector
        brushFaces.reserve(brush.num_sides);

        std::vector<size_t> brushside_indices;
        for (size_t i = 0; i < brush.num_sides; ++i)
            brushside_indices.push_back(brush.first_side + i);

        // Check if we are interested in any of the brushsides, if not -> skip this brush
        if (pred_BrushSide) {// If BrushSide predicate function was provided
            bool isAnyFaceWanted = false;
            for (size_t bSideIdx : brushside_indices) {
                if (pred_BrushSide(this->brushsides[bSideIdx], *this)) {
                    isAnyFaceWanted = true;
                    break;
                }
            }
            if (!isAnyFaceWanted)
                continue; // Skip this brush
        }

        // All csgo brushes have at least 6 axis planes(normals with just one non-zero component) which make
        // a nice axis-aligned small box. To save potential float error trouble with very large slant surfaces
        // later on, we first cut to a (relatively) small 6-sided cuboid. After that, cut slant surfaces.
        // -> Sort planes, axis planes(normals with just one non-zero component) first
        std::sort(brushside_indices.begin(), brushside_indices.end(), [this](size_t idx1, size_t idx2) {
            Vector3 n1 = this->planes[this->brushsides[idx1].plane_num].normal;
            Vector3 n2 = this->planes[this->brushsides[idx2].plane_num].normal;
            int emptyComps1 = 0, emptyComps2 = 0;
            for (int i = 0; i < 3; ++i) if (n1[i] == 0.0f) ++emptyComps1;
            for (int i = 0; i < 3; ++i) if (n2[i] == 0.0f) ++emptyComps2;
            return emptyComps1 > emptyComps2;
        });

        std::vector<size_t> unwantedBrushFaceIndices; // indices into brushFaces that need to be removed at the end

        for (size_t bSideIdx : brushside_indices) { // Iterate through brush planes

            const BspMap::BrushSide& bSide = this->brushsides[bSideIdx];
            BspMap::Plane plane = this->planes[bSide.plane_num];
            std::vector<Vector3> bSideVertices; // vertices of face of this plane after clipping

            // First, check if ALL vertices of ALL faces are EXACTLY(no overcut) behind the plane, if yes -> plane redundant,skip
            bool isPlaneRedundant = true;
            for (std::vector<Vector3>& bFace : brushFaces) {
                for (Vector3& v : bFace) {
                    if (!isVertexBehindPlane(v, plane, -BRUSH_PLANE_REDUNDANT_CUT_SIZE)) { // if plane cuts vertex deep enough
                        isPlaneRedundant = false;
                        break;
                    }
                }
                if (!isPlaneRedundant)
                    break;
            }
            if (isPlaneRedundant) // Plane doesn't cut any face -> skip
                continue;

            for (std::vector<Vector3>& bFace : brushFaces) {
                std::vector<Vector3> alteredVertices; // vertex list of bFace after clipping

                // Check for all vertices if they're behind the plane (with overcut)
                std::vector<bool> areVertsBehindPlane;
                areVertsBehindPlane.reserve(bFace.size());
                for (Vector3& v : bFace)
                    areVertsBehindPlane.push_back(isVertexBehindPlane(v, plane, BRUSH_PLANE_OVER_CUT));

                // Go through every edge and cut them if necessary
                for (size_t idx_currVert = 0; idx_currVert < bFace.size(); ++idx_currVert) {
                    size_t idx_nextVert = idx_currVert + 1;
                    if (idx_nextVert == bFace.size())
                        idx_nextVert = 0;

                    Vector3& currVert = bFace[idx_currVert];
                    Vector3& nextVert = bFace[idx_nextVert];
                    bool isCurrVertBehindPlane = areVertsBehindPlane[idx_currVert];
                    bool isNextVertBehindPlane = areVertsBehindPlane[idx_nextVert];

                    if (isCurrVertBehindPlane) {
                        // Only add current vertex if alteredVertices doesn't contain it yet
                        bool duplicate = false;
                        Vector3 currVert_nonEx = getVertexNonExtremeComponents(currVert);
                        for (Vector3& v : alteredVertices) {
                            if (BspMap::AreVerticesEquivalent(currVert_nonEx, getVertexNonExtremeComponents(v))) {
                                duplicate = true;
                                break;
                            }
                        }
                        if (!duplicate)
                            alteredVertices.push_back(currVert);
                    }

                    if (isCurrVertBehindPlane != isNextVertBehindPlane) {
                        // Check if intersection calculation is required: is edge actually cut by plane (no overcut)
                        bool isEdgeActuallyCutByPlane;
                        if (isNextVertBehindPlane) isEdgeActuallyCutByPlane = !isVertexBehindPlane(currVert, plane, 0.0f);
                        else                       isEdgeActuallyCutByPlane = !isVertexBehindPlane(nextVert, plane, 0.0f);

                        Vector3 newVertex;
                        if (isEdgeActuallyCutByPlane) { // Calculate intersection point of plane and line from currVert to nextVert
                            // Our BSP plane has the form {normal, dist}: normal.X*x + normal.Y*y + normal.Z*z = dist
                            // planeEquation has the form (A,B,C,D): Ax + By + Cz + D = 0
                            Vector4 planeEquation = { plane.normal.x(), plane.normal.y(), plane.normal.z(), -plane.dist };
                            Vector3& lineStart = currVert;
                            Vector3 lineDirection = nextVert - currVert;

                            Float linePosition = Math::Intersection::planeLine(planeEquation, lineStart, lineDirection);

                            if (Math::isNan(linePosition) || Math::isInf(linePosition)) { // NaN -> Line lies on the plane, Inf -> No intersection
                                // In this case, the line is very close to the plane and almost parallel to it. It's so tight
                                // and inaccurate we decide to not cut the edge and use the "cut" vertex as the "intersection".
                                if (isCurrVertBehindPlane) newVertex = nextVert;
                                else                       newVertex = currVert;
                            }
                            else { // Intersection found!
                                // If line is almost parallel to plane, linePosition can go outside of interval [0,1] due to float inaccuracy
                                linePosition = std::min(std::max(linePosition, 0.0f), 1.0f); // Limit linePosition to [0,1]
                                newVertex = lineStart + linePosition * lineDirection; // intersection point
                            }
                        }
                        else { // Plane doesn't actually cut edge -> use the "cut" vertex as the "intersection"
                            // Since a plane cut gives at least 2 "intersections" per face this tries to
                            // add 2 duplicate vertices. The duplicate one gets sorted out right below.
                            if (isCurrVertBehindPlane) newVertex = nextVert;
                            else                       newVertex = currVert;
                        }

                        // Add new vertex to new surface on the plane
                        bSideVertices.push_back(newVertex); // duplicate vertices get sorted out later

                        // Only add new vertex if alteredVertices doesn't contain it yet
                        bool duplicate = false;
                        Vector3 newVertex_nonEx = getVertexNonExtremeComponents(newVertex);
                        for (Vector3& v : alteredVertices) {
                            if (BspMap::AreVerticesEquivalent(newVertex_nonEx, getVertexNonExtremeComponents(v))) {
                                duplicate = true;
                                break;
                            }
                        }
                        if (!duplicate)
                            alteredVertices.push_back(newVertex);
                    }
                }
                if (alteredVertices.size() >= 3) // at least 3 have to remain for a surface!
                    bFace.swap(alteredVertices); // take new vertex list
                else
                    bFace.clear(); // delete vertices of cut face
            }

            if (!bSideVertices.empty()) {
                // Sort out duplicate vertices
                std::vector<Vector3> filteredVertices;
                for (Vector3& v : bSideVertices) {
                    bool duplicate = false;
                    Vector3 v_nonEx = getVertexNonExtremeComponents(v);
                    for (Vector3& v_filtered : filteredVertices) {
                        // If vertices are too close together
                        if (BspMap::AreVerticesEquivalent(v_nonEx, getVertexNonExtremeComponents(v_filtered))) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate)
                        filteredVertices.push_back(v);
                }

                if (filteredVertices.size() >= 3) { // size is 1 if plane only touched one brush corner point
                    // Calculate center of face
                    Vector3 center = { 0.0f, 0.0f, 0.0f };
                    for (Vector3& v : filteredVertices)
                        center += v;
                    center /= filteredVertices.size();

                    // Sort vertices in clockwise order
                    std::function<bool(Vector3&, Vector3&)> comp = [&center, &plane](Vector3& a, Vector3& b) {
                        bool result = Math::dot(plane.normal, Math::cross(a - center, b - center)) < 0; // "< 0" for CW, "> 0" for CCW
                        return result; // true if b comes after a (clockwise)
                    };
                    // Take first vertex and divide the remaining vertices into two halves, one before and one after the first vertex
                    std::vector<Vector3> preHalf, postHalf;
                    Vector3 referenceVertex = filteredVertices[0]; // Used to divide face vertices into 2 halves
                    for (auto it = std::next(filteredVertices.begin()); it != filteredVertices.end(); ++it) {
                        if (comp(*it, referenceVertex)) preHalf.push_back(*it);
                        else                            postHalf.push_back(*it);
                    }
                    // In each half, the comparison function works out and sorts vertices in correct winding order
                    std::sort(preHalf.begin(), preHalf.end(), comp);
                    std::sort(postHalf.begin(), postHalf.end(), comp);
                    // Put first half, reference vertex and second half back together
                    std::vector<Vector3> sortedVertices;
                    sortedVertices.insert(sortedVertices.end(), preHalf.begin(), preHalf.end()); // Append preHalf
                    sortedVertices.insert(sortedVertices.end(), referenceVertex); // Append reference vertex
                    sortedVertices.insert(sortedVertices.end(), postHalf.begin(), postHalf.end()); // Append postHalf
                    // Check if brushside face fits the predicate, otherwise mark as unwanted
                    if (pred_BrushSide) // If BrushSide predicate function was provided
                        if (!pred_BrushSide(bSide, *this)) // Mark brushside face as unwanted if it doesn't fit the predicate
                            unwantedBrushFaceIndices.push_back(brushFaces.size()); // Save index to delete brushside face later
                    // Add new face
                    brushFaces.push_back(std::move(sortedVertices));
                }
            }
        }

        // Clear brush faces that did not fit the predicate
        for (size_t i : unwantedBrushFaceIndices)
            brushFaces[i].clear();
        // Append non-empty faces of this brush to finalFaces
        for (std::vector<Vector3>& face : brushFaces)
            if(!face.empty())
                finalFaces.push_back(std::move(face));
    }
    return finalFaces;
}

// The first model in the models array is "worldspawn", containing the geometry of the whole map
// excluding entities (but including func_detail brushes)
std::set<size_t> BspMap::GetModelBrushIndices_worldspawn() const
{
    return GetModelBrushIndices(0);
}

std::set<size_t> BspMap::GetModelBrushIndices(uint32_t model_idx) const
{
    const Model& m = this->models[model_idx];
    //std::vector<std::vector<Magnum::Vector3>> totalFaces;

    std::vector<int32_t> pendingNodesAndLeafs = { m.head_node };

    std::set<size_t> brush_indices; // indices of all brushes that we find

    while (!pendingNodesAndLeafs.empty()) {
        int32_t nextEntry = pendingNodesAndLeafs.back();
        pendingNodesAndLeafs.pop_back();

        if (nextEntry >= 0) { // Node is referenced
            const Node& node = this->nodes[nextEntry];
            /*for (size_t i = node.first_face; i < (size_t)node.first_face + node.num_faces; ++i) {
                auto faceVertices = GetFaceVertices(i);
                totalFaces.push_back(std::move(faceVertices));
            }*/
            pendingNodesAndLeafs.push_back(node.children[0]);
            pendingNodesAndLeafs.push_back(node.children[1]);
        }
        else { // Leaf is referenced
            const Leaf& leaf = this->leafs[-(nextEntry + 1)];
            // Go through leaffaces of this leaf
            /*for (size_t lface = leaf.first_leaf_face; lface < (size_t)leaf.first_leaf_face + leaf.num_leaf_faces; ++lface) {
                auto faceVertices = GetFaceVertices(this->leaffaces[lface]);
                totalFaces.push_back(std::move(faceVertices));
            }*/
            // Go through leafbrushes of this leaf
            for (size_t lbrush = leaf.first_leaf_brush; lbrush < (size_t)leaf.first_leaf_brush + leaf.num_leaf_brushes; ++lbrush) {
                size_t brush_idx = this->leafbrushes[lbrush];
                brush_indices.insert(brush_idx); // std::set -> only inserts if it doesn't already contain the element
            }
        }
    }
    
    return brush_indices;
}

bool BspMap::Ent_func_brush::IsSolid() const
{
    if (solidity == 1) return false; // Never solid
    if (solidity == 2) return true; // Always solid
    // When solidity is set to 0 (Toggle), it depends on start_disabled
    if (start_disabled) return false;
    return true;
}

bool BspMap::DispInfo::HasFlag_NO_PHYSICS_COLL() const { return flags & ((uint32_t)1 <<  1); }
bool BspMap::DispInfo::HasFlag_NO_HULL_COLL()    const { return flags & ((uint32_t)1 <<  2); }
bool BspMap::DispInfo::HasFlag_NO_RAY_COLL()     const { return flags & ((uint32_t)1 <<  3); }
bool BspMap::DispInfo::HasFlag_UNKNOWN_1()       const { return flags & ((uint32_t)1 << 30); }
bool BspMap::DispInfo::HasFlag_UNKNOWN_2()       const { return flags & ((uint32_t)1 << 31); }

bool BspMap::StaticProp::IsNotSolid()          const { return solid == 0; }
bool BspMap::StaticProp::IsSolidWithAABB()     const { return solid == 2; }
bool BspMap::StaticProp::IsSolidWithVPhysics() const { return solid == 6; }

bool BspMap::Ent_trigger_push::CanPushPlayers()                 const { return spawnflags & ((uint32_t)1 <<  0); }
bool BspMap::Ent_trigger_push::CorrectlyAccountsForObjectMass() const { return spawnflags & ((uint32_t)1 << 12); }
