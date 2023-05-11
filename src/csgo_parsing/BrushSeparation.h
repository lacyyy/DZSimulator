#ifndef CSGO_PARSING_BRUSHSEPARATION_H_
#define CSGO_PARSING_BRUSHSEPARATION_H_

#include <utility>

#include "csgo_parsing/BspMap.h"

namespace csgo_parsing::BrushSeparation {

    enum Category {
        SOLID,
        PLAYERCLIP,
        GRENADECLIP,
        LADDER,
        WATER,
        SKY
    };

    // Types of functions to test if brush/brushside belongs to some category
    typedef bool (*isBrush_X_func_t)    (const csgo_parsing::BspMap::Brush&);
    typedef bool (*isBrushSide_X_func_t)(const csgo_parsing::BspMap::BrushSide&, const csgo_parsing::BspMap&);

    std::pair<isBrush_X_func_t, isBrushSide_X_func_t> getBrushCategoryTestFuncs(Category cat);

}

#endif // CSGO_PARSING_BRUSHSEPARATION_H_
