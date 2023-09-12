#include "csgo_parsing/BrushSeparation.h"

#include <cstring>
#include <string>

#include <Magnum/Magnum.h>

#include "csgo_parsing/BspMap.h"

using namespace csgo_parsing;
using namespace csgo_parsing::BrushSeparation;


// -------- SOLID BRUSHES ---------------------------------------------------------------------------------

// FIXME the "solid category", includes solid brushes and sky brushes, but it does not have playerclips,ladders,...
// FIXME TODO brush separation should be split into "visual" and "functional" separation
bool IS_BRUSH_SOLID(const BspMap::Brush& b) {
    // FIXME? Returns true for sky brushes
    if (b.HasFlags(BspMap::Brush::WATER) || b.HasFlags(BspMap::Brush::LADDER))
        return false;
    //Debug{} << b.contents;
    return b.HasFlags(BspMap::Brush::SOLID) || b.HasFlags(BspMap::Brush::GRATE) || b.HasFlags(BspMap::Brush::WINDOW);
}
std::string solid_tex_trigger = "TOOLS/TOOLSTRIGGER";
bool IS_BRUSHSIDE_SOLID(const BspMap::BrushSide& bs, const BspMap& map) {
    // TOOLS/TOOLSTRIGGER brush flags = 1
    // TOOLS/TOOLSAREAPORTAL brush flags = 1
    // TOOLS/TOOLSNODRAW brush flags = 1

    BspMap::TexInfo ti = map.texinfos[bs.texinfo];
    if (ti.HasFlag_SKY()) return false;
    const char* texName = map.texdatastringdata.data() + map.texdatastringtable[map.texdatas[ti.texdata].name_string_table_id];

    if (solid_tex_trigger.compare(texName) == 0) {
        //Debug{} << texName;
        return false;
    }
    return true;
    //Debug{} << ti.HasFlag_NODRAW() << texName;
    //return bs.dispinfo != -1;
}

// -------- PLAYERCLIP BRUSHES ----------------------------------------------------------------------------

bool IS_BRUSH_PLAYERCLIP(const BspMap::Brush& b) {
    return b.HasFlags(BspMap::Brush::PLAYERCLIP);
}

// -------- GRENADECLIP BRUSHES ---------------------------------------------------------------------------

bool IS_BRUSH_GRENADECLIP(const BspMap::Brush& b) {
    return b.HasFlags(BspMap::Brush::GRENADECLIP);
}

// -------- LADDER BRUSHES --------------------------------------------------------------------------------

bool IS_BRUSH_LADDER(const BspMap::Brush& b) {
    return b.HasFlags(BspMap::Brush::LADDER);
}

// -------- WATER BRUSHES ---------------------------------------------------------------------------------

bool IS_BRUSH_WATER(const BspMap::Brush& b) {
    return b.HasFlags(BspMap::Brush::WATER);
}

// -------- SKY BRUSHES -----------------------------------------------------------------------------------

bool IS_BRUSHSIDE_SKY(const BspMap::BrushSide& bs, const BspMap& map) {
    return map.texinfos[bs.texinfo].HasFlag_SKY();
}

// -------- 



// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

std::pair<isBrush_X_func_t, isBrushSide_X_func_t>
BrushSeparation::getBrushCategoryTestFuncs(Category cat)
{
    switch (cat) {
    case Category::SOLID:       return { &IS_BRUSH_SOLID,       &IS_BRUSHSIDE_SOLID };
    case Category::PLAYERCLIP:  return { &IS_BRUSH_PLAYERCLIP,  nullptr };
    case Category::GRENADECLIP: return { &IS_BRUSH_GRENADECLIP, nullptr };
    case Category::LADDER:      return { &IS_BRUSH_LADDER,      nullptr };
    case Category::WATER:       return { &IS_BRUSH_WATER,       nullptr };
    case Category::SKY:         return { nullptr,               &IS_BRUSHSIDE_SKY };
    }
    Magnum::Error{} << "[ERR] BrushSeparation::getBrushCategoryTestFuncs() unknown category:"
        << cat << ", forgotten switch case?";
    return { nullptr, nullptr };
}
