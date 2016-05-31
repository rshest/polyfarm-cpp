#include <rect_contour.hpp>
#include <svg_gen.h>


static const std::vector<const char*> COLORS = {
    "8dd3c7", "ffffb3", "bebada", "fb8072", "80b1d3", "fdb462",
    "b3de69", "fccde5", "d9d9d9", "bc80bd", "ccebc5", "ffed6f"
};

void gen_shape_path(const shape& sh, std::ostream& os, int ext = 1, int cell_side = 15, bool outline = true) {
    if (outline) {
        std::vector<bool> bitmap(sh.width*sh.height, false);

        for (const auto& p : sh.squares) {
            bitmap[p.x + p.y*sh.width] = true;
        }
        rect_contour contour;
        contour.trace_bitmap(bitmap, sh.width, {(int)cell_side, (int)cell_side});
        contour.extrude(ext, ext);
        os << contour.svg_path();
    } else {
        for (const auto& p : sh.squares) {
            int x = p.x*cell_side;
            int y = p.y*cell_side;
            int x1 = x + cell_side;
            int y1 = y + cell_side;
            os << "M" << x << "," << y << " L" << x1 << "," << y << 
                " L" << x1 << "," << y1 << " L" << x << "," << y1 << " Z ";
        }
    }
}

void create_svg(std::ostream& os, const shape::variation_array& variations, 
        const std::vector<shape_pos>& positions, const shape* core, const vec2i* core_pos, int cell_side) 
{
    vec2i lt, rb;
    shape::get_bounds(variations, positions, lt, rb);
    
    int w = rb.x - lt.x + 1;
    int h = rb.y - lt.y + 1;

    os << "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" " <<
        "shape-rendering=\"crispEdges\""
        " width=\"" << (w*cell_side) << "\" height=\"" << (h*cell_side) << "\">\n";

    std::stringstream sqpath;
    sqpath << "M0,0 l" << cell_side << ",0 L" << cell_side << "," << cell_side << " L0," << cell_side << " Z";
    os << "<defs> <pattern id=\"squares\" patternUnits=\"userSpaceOnUse\" "
        "x=\"0\" y=\"0\" width=\"" << cell_side << "\" height=\"" << cell_side << "\">"
        "<g style=\"fill:none; stroke:#dde; stroke-width:1\">"
        "<path d=\""<< sqpath.str() << "\"/></g></pattern></defs>";
        
    os << "<style>\n/* <![CDATA[ */\n " <<
        ".core { fill: url(#squares) #fff; } \n" <<
        ".caption { fill: #aae; font-family:Arial; font-size:25px; font-weight:bold; dominant-baseline:central; text-anchor:middle; } \n" <<
        ".shape { stroke:#224a22; stroke-width:1; opacity:1; } \n" <<
        "\n/* ]]> */\n</style>";

    if (core && core_pos) {
        float x = (float)(core_pos->x - lt.x);
        float y = (float)(core_pos->y - lt.y);

        float dx = x*cell_side;
        float dy = y*cell_side;

        os << "\n  <path d=\""; 
        gen_shape_path(*core, os, 0, cell_side);
        os << "\" transform=\"translate(" << dx << "," << dy << ")\"";
        os << " class=\"core\"> </path>";

        float tx = dx + core->width*cell_side/2;
        float ty = dy + core->height*cell_side/2;
        os << "\n  <text class=\"caption\" x=\"" << tx << "\" y=\"" << ty << "\">" << 
            core->squares.size() << "</text> ";
    }

    const size_t nshapes = variations.size();
    for (size_t i = 0; i < nshapes; i++) {
        const shape_pos& pos = positions[i];
        const shape& sh = variations[pos.shape_idx][pos.var_idx];

        float x = (float)(pos.x - lt.x);
        float y = (float)(pos.y - lt.y);

        float dx = x*cell_side;
        float dy = y*cell_side;
        os << "\n  <path d=\""; 
        gen_shape_path(sh, os, 1, cell_side);
        const char* color = COLORS[pos.shape_idx%COLORS.size()];
        os << "\" fill=\"#" << color << "\" class=\"shape\" " << 
            "transform=\"translate(" << dx << "," << dy << ")\"" << ">" << "</path>";
    }

    os << "\n</svg>\n";
}
