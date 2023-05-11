#include "BrushSeparation.h"

#include <cstring>
#include <string>

#include <Magnum/Magnum.h>

#include "csgo_parsing/BspMap.h"


using namespace csgo_parsing::BrushSeparation;


// -------- SOLID BRUSHES ---------------------------------------------------------------------------------

bool IS_BRUSH_SOLID(const csgo_parsing::BspMap::Brush& b) {
    if (b.HasFlags(csgo_parsing::BspMap::Brush::WATER) || b.HasFlags(csgo_parsing::BspMap::Brush::LADDER))
        return false;
    //Debug{} << b.contents;
    return b.HasFlags(csgo_parsing::BspMap::Brush::SOLID) || b.HasFlags(csgo_parsing::BspMap::Brush::GRATE) || b.HasFlags(csgo_parsing::BspMap::Brush::WINDOW);
}
std::string solid_tex_trigger = "TOOLS/TOOLSTRIGGER";
bool IS_BRUSHSIDE_SOLID(const csgo_parsing::BspMap::BrushSide& bs, const csgo_parsing::BspMap& map) {
    // TOOLS/TOOLSTRIGGER brush flags = 1
    // TOOLS/TOOLSAREAPORTAL brush flags = 1
    // TOOLS/TOOLSNODRAW brush flags = 1

    csgo_parsing::BspMap::TexInfo ti = map.texinfos[bs.texinfo];
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

bool IS_BRUSH_PLAYERCLIP(const csgo_parsing::BspMap::Brush& b) {
    return b.HasFlags(csgo_parsing::BspMap::Brush::PLAYERCLIP);
}

// -------- GRENADECLIP BRUSHES ---------------------------------------------------------------------------

bool IS_BRUSH_GRENADECLIP(const csgo_parsing::BspMap::Brush& b) {
    return b.HasFlags(csgo_parsing::BspMap::Brush::GRENADECLIP);
}

// -------- LADDER BRUSHES --------------------------------------------------------------------------------

bool IS_BRUSH_LADDER(const csgo_parsing::BspMap::Brush& b) {
    return b.HasFlags(csgo_parsing::BspMap::Brush::LADDER);
}

// -------- WATER BRUSHES ---------------------------------------------------------------------------------

bool IS_BRUSH_WATER(const csgo_parsing::BspMap::Brush& b) {
    return b.HasFlags(csgo_parsing::BspMap::Brush::WATER);
}

// -------- SKY BRUSHES -----------------------------------------------------------------------------------

bool IS_BRUSHSIDE_SKY(const csgo_parsing::BspMap::BrushSide& bs, const csgo_parsing::BspMap& map) {
    return map.texinfos[bs.texinfo].HasFlag_SKY();
}

// -------- 



// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

std::pair<isBrush_X_func_t, isBrushSide_X_func_t>
csgo_parsing::BrushSeparation::getBrushCategoryTestFuncs(Category cat)
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
