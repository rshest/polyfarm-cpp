#ifndef __SVG_GEN_H__
#define __SVG_GEN_H__

#include <vector>

#include <vec2.hpp>
#include <shape.hpp>

void create_svg(std::ostream& os, const shape::variation_array& variations, const std::vector<shape_pos>& positions, 
    const shape* core = nullptr, const vec2i* core_pos = nullptr, int cell_side = 15);



#endif
