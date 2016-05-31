#ifndef __SHAPE__
#define __SHAPE__

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <queue>
#include <cassert>

#include <vec2.hpp>

const double PI = 3.14159265358979323846;
const double MAX_DIST = 1e5;
const std::vector<vec2i> OFFS = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
const std::vector<vec2i> COFFS = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, -1}, {1, 1}, {-1, 1}, {-1, -1}};

//  returns true if the firest angle is "greater or equal" 
// (assuming that two of them form a non-concave angle themselves)
inline bool angle_greater(double lhs, double rhs) {
    if (lhs < rhs && rhs - lhs > PI) return true;
    return lhs > rhs && lhs - rhs < PI;
}

enum class rotation {
    None     = 0,
    CW_90    = 1,
    CW_180   = 2,
    CW_270   = 3
};

enum class overlap {
    Overlap     = 0,    //  two shapes overlap
    Border      = 1,    //  two shapes don't overlap, but have a common edge
    Disjoint    = 2,    //  shapes neither overlap nor have a common edge
};

struct shape_pos {
    int x, y;
    uint16_t shape_idx; 
    uint16_t var_idx; 

    vec2i p() const { return {x, y}; }
    bool operator == (const shape_pos& rhs) const {
        return x == rhs.x && y == rhs.y && 
            shape_idx == rhs.shape_idx && var_idx == rhs.var_idx; 
    }
};

struct shape {
    typedef std::vector< std::vector<shape> > variation_array;

    std::vector<vec2i> squares;
    int width, height;


    shape() : width(0), height(0) {}

    bool is_set(int x, int y) const {
        if (x < 0 || y < 0  || x >= width || y >= height) return false;
        return mask[x + y*width] != 0;
    }


    bool operator ==(const shape& rhs) const { return width == rhs.width && mask == rhs.mask; }

    shape mirrored() const {
        shape res;
        size_t nsq = squares.size();
        res.squares.resize(nsq);
        const int h = height;
        for (size_t i = 0; i < nsq; i++) {
            const auto& sq = squares[i];
            res.squares[i] = vec2i(width - sq.x - 1, sq.y);
        }
        res.setup();
        return res;
    }

    shape rotated(rotation rot) const {
        shape res;
        size_t nsq = squares.size();
        res.squares.resize(nsq);
        const int w = width;
        const int h = height;
        for (size_t i = 0; i < nsq; i++) {
            const auto& sq = squares[i];
            int x, y;
            switch (rot) {
            case rotation::CW_90:   x = h - sq.y - 1; y = sq.x;         break;
            case rotation::CW_180:  x = w - sq.x - 1; y = h - sq.y - 1; break;
            case rotation::CW_270:  x = sq.y;         y = w - sq.x - 1; break;
            default:                x = sq.x;         y = sq.y; 
            }
            res.squares[i] = vec2i(x, y);
        }

        res.setup();
        return res;
    }

    std::vector<shape> get_variations() const {
        std::vector<shape> res;
        auto add_shape = [&](const shape& sh) {
            if (std::find(res.begin(), res.end(), sh) == res.end()) {
                res.push_back(sh);
            }
        };
        add_shape(*this);
        add_shape(rotated(rotation::CW_90));
        add_shape(rotated(rotation::CW_180));
        add_shape(rotated(rotation::CW_270));

        shape m = mirrored();
        add_shape(m);
        add_shape(m.rotated(rotation::CW_90));
        add_shape(m.rotated(rotation::CW_180));
        add_shape(m.rotated(rotation::CW_270));
        return res;
    }

    static bool parse(std::istream& is, shape& sh) {
        int row = 0;
        std::string line;
        int nparsed = 0;
        while (std::getline(is, line)) {
            if (line.empty()) break;
            const int n = (int)line.size();
            for (int i = 0; i < n; i++) {
                if (line[i] != ' ') {
                    sh.squares.push_back(vec2i(i, row));
                    nparsed++;
                }
            }
            row++;
        }
        sh.setup();
        return nparsed != 0;
    }

    static std::vector<shape> parse(std::istream& is) {
        std::vector<shape> res;
        while (true) {
            shape sh;
            if (!shape::parse(is, sh)) break;
            res.push_back(sh);
        }
        return res;
    }
    
    //  returns manhattan distance between two shapes' squares
    // -1 if they overlap, 0 if border
    friend int distance(const shape& sh1, const vec2i& pos1, 
        const shape& sh2, const vec2i& pos2) 
    {
        overlap ov = overlap_status(sh1, pos1, sh2, pos2);
        if (ov == overlap::Border) return 0;
        if (ov == overlap::Overlap) return -1;

        int dx = pos1.x - pos2.x;
        int dy = pos1.y - pos2.y;

        int min_dist = std::numeric_limits<int>::max();
        for (const auto& sq1 : sh1.squares) {
            const int x = sq1.x  + dx;
            const int y = sq1.y + dy;
            for (const auto& sq2 : sh2.squares) {
                min_dist = std::min(min_dist, abs(x - sq2.x) + abs(y - sq2.y));
            }
        }
        return min_dist - 1;
    }

    friend overlap overlap_status(const shape& sh1, const vec2i& pos1, 
       const shape& sh2, const vec2i& pos2) 
    {
        if (pos1.x > pos2.x + sh2.width  ||
            pos2.x > pos1.x + sh1.width  ||
            pos1.y > pos2.y + sh2.height ||
            pos2.y > pos1.y + sh1.height) return overlap::Disjoint;
        
        int dx = pos1.x - pos2.x;
        int dy = pos1.y - pos2.y;
   
        //  test for overlapping
        for (const auto& sq : sh1.squares) {
            int x = sq.x + dx;
            int y = sq.y + dy;
            if (sh2.is_set(x, y)) return overlap::Overlap;
        }

        //  no overlapping, test for bordering
        for (const auto& sq : sh1.squares) {
            for (const auto& offs : OFFS) {
                int x = sq.x + dx + offs.x;
                int y = sq.y + dy + offs.y;
                if (sh2.is_set(x, y)) return overlap::Border;
            }
        }
        return overlap::Disjoint;
    }

    double estimate_len() const {
        return std::max(width, height);
    }

    double dist2circle(double radius, const vec2i& pos) const {
        double d = 0.0f;
        for (const auto& sq : squares) {
            double dr = (pos + sq).len() - radius;
            d += dr*dr;
        }
        return d;
    }

    std::pair<double, double> angle_range(const vec2i& pos) const {
        double ang1 = std::numeric_limits<double>::max();
        double ang2 = -std::numeric_limits<double>::max();

        for (const auto& p : squares) {
            vec2i cpos = p + pos;
            double ang = std::atan2(cpos.y, cpos.x);
            if (ang < 0) ang += 2*PI;
            ang1 = std::min(ang1, ang);
            ang2 = std::max(ang2, ang);
        }
        return std::make_pair(ang1, ang2);
    }

    template <typename TFn>
    void best_fit(shape_pos& res, const vec2i& pos, const std::vector<shape>& variations, TFn fit_fn) const {
        const int nvar = (int)variations.size();
        double min_d = std::numeric_limits<double>::max();
        for (int var = 0; var < nvar; var++) {
            const shape& sh = variations[var];
            for (const auto& bpos : boundary) {
                for (const auto& cpos : sh.squares) {
                    vec2i p = pos + bpos - cpos;
                    double d = fit_fn(p, sh);
                    if (d < min_d) {
                        res.x = p.x;
                        res.y = p.y;
                        res.var_idx = var;
                        min_d = d;
                    }
                }
            }
        }
    }

    static void arrange_circle(double radius, const variation_array& variations, 
        std::vector<shape_pos>& positions) 
    {
        const int nshapes = (int)variations.size();
        assert(positions.size() == nshapes);

        for (int i = 0; i <= nshapes; i++) {
            if (i == 0) {
                shape_pos& pos = positions[i];
                const shape& sh = variations[pos.shape_idx][0];
                pos.x = (int)round(radius - sh.width*0.5);
                pos.y = (int)round(-sh.height*0.5);
                pos.var_idx = 0;
            } else {
                const shape_pos& prev_pos = positions[i - 1];
                const shape& prev_shape = variations[prev_pos.shape_idx][prev_pos.var_idx];
                const auto prev_angles = prev_shape.angle_range(prev_pos.p());

                int k = i%nshapes;
                const auto vars = variations[positions[k].shape_idx];
                prev_shape.best_fit(positions[k], prev_pos.p(), vars, 
                    [&](const vec2i& p, const shape& sh){
                    if (i == nshapes) {
                        int i1 = (i + 1)%nshapes;
                        const shape_pos& next_pos = positions[i1];
                        const shape& next_shape = variations[next_pos.shape_idx][next_pos.var_idx];
                        int d1 = distance(sh, p, next_shape, next_pos.p());
                        if (d1 != 0) return MAX_DIST + abs(d1);
                    } 
                    int d = distance(sh, p, prev_shape, prev_pos.p());
                    if (d != 0) return MAX_DIST;

                    auto angles = sh.angle_range(p);
                    if (angle_greater(angles.second, prev_angles.second)) return MAX_DIST;
                    double dist = sh.dist2circle(radius, p);
                    return dist/angles.second;
                });
            }
        }

    }

    static double score(const variation_array& variations, const std::vector<shape_pos>& positions) {
        //  find the are of the closed space
        int area = flood_fill(variations, positions, [](int, int){});
        if (area > 0) return area;
        double dist = 0.0;
        const size_t nshapes = variations.size();
        for (int i = 0; i < nshapes; i++) {
            const shape_pos& pos1 = positions[i];
            const shape_pos& pos2 = positions[(i + 1)%nshapes];
            const shape& sh1 = variations[pos1.shape_idx][pos1.var_idx];
            const shape& sh2 = variations[pos2.shape_idx][pos2.var_idx];
            dist += abs(distance(sh1, pos1.p(), sh2, pos2.p()));
        }

        return -dist;
    }

    static void get_bounds(const variation_array& variations, 
        const std::vector<shape_pos>& positions, vec2i& lt, vec2i& rb) 
    {
        lt = {std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
        rb = {std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};

        const int n = (int)variations.size();
        for (int i = 0; i < n; i++) {
            const shape_pos& pos = positions[i];
            const shape& sh = variations[pos.shape_idx][pos.var_idx];
            lt.x = std::min(lt.x, pos.x);
            lt.y = std::min(lt.y, pos.y);
            rb.x = std::max(rb.x, pos.x + sh.width);
            rb.y = std::max(rb.y, pos.y + sh.height);
        }
    }

    static void center(const variation_array& variations, std::vector<shape_pos>& positions) 
    {
        vec2i lt, rb;
        get_bounds(variations, positions, lt, rb);
        int cx = (rb.x + lt.x)/2;
        int cy = (rb.y + lt.y)/2;
        for (auto& pos : positions) {
            pos.x -= cx;
            pos.y -= cy;
        }
    }

    template <typename TFn>
    static int flood_fill(const variation_array& variations, 
        const std::vector<shape_pos>& positions, TFn hit_fn)
    {
        vec2i lt, rb;
        get_bounds(variations, positions, lt, rb);
        int w = rb.x - lt.x + 1;
        int h = rb.y - lt.y + 1;

        //  create the mask
        const int n = (int)variations.size();
        std::vector<char> mask(w*h, 0);
        for (int i = 0; i < n; i++) {
            const shape_pos& pos = positions[i];
            const shape& sh = variations[pos.shape_idx][pos.var_idx];
            for (const auto& sq : sh.squares) {
                int x = pos.x + sq.x - lt.x;
                int y = pos.y + sq.y - lt.y;
                mask[x + y*w] = 1;
            }
        }

        // compute the starting point
        vec2i start{w/2, h/2};
        if (mask[start.x + start.y*w]) {
            for (const vec2i& offs : COFFS) {
                vec2i c = start + offs;
                if (mask[c.x + c.y*w] == 0) {
                    start = c;
                    break;
                }
            }
        }

        //  flood-fill
        std::vector<vec2i> cellq;
        vec2i c0 = start;
        cellq.push_back(c0);
        mask[c0.x + c0.y*w] = 1;
        int nvisited = 0;
        while (!cellq.empty()) {
            vec2i c = cellq.back();
            cellq.pop_back();
            hit_fn(c.x + lt.x, c.y + lt.y);
            nvisited++;
            for (const vec2i& offs : COFFS) {
                vec2i c1 = c + offs;
                if (c1.x < 0 || c1.y < 0 || 
                    c1.x >= w || c1.y >= h) {
                    return -1;
                }
                int idx = c1.x + c1.y*w;
                if (mask[idx] == 0) {
                    cellq.push_back(c1);
                    mask[idx] = 1;
                }
            }
        }

        return nvisited;
    }

    static bool extract_core(const variation_array& variations, 
        const std::vector<shape_pos>& positions, shape& sh, vec2i& pos) 
    {
        pos.x = pos.y = std::numeric_limits<int>::max();
        int num_visited = flood_fill(variations, positions, [&](int x, int y) {
            pos.x = std::min(pos.x, x);
            pos.y = std::min(pos.y, y);
            sh.squares.push_back({x, y});
        });
        
        for (vec2i& sq : sh.squares) sq = sq - pos;
        sh.setup();

        return num_visited > 0;
    }

private:
    std::vector<char> mask;
    std::vector<vec2i> boundary;

    void setup() {
        //  compute extents
        const int n = (int)squares.size();
        width = height = 0;
        for (int i = 0; i < n; i++) {
            const auto& sq = squares[i];
            width  = std::max(width,  sq.x  + 1);
            height = std::max(height, sq.y + 1);
        }

        //  cache the mask
        mask.resize(width*height, 0);
        for (int i = 0; i < n; i++) {
            const auto& sq = squares[i];
            mask[sq.x + sq.y*width] = 1;
        }

        //  compute the boundary cells
        boundary.clear();
        for (const auto& sq : squares) {
            for (const auto& offs : OFFS) {
                int x = sq.x + offs.x;
                int y = sq.y + offs.y;
                if (!is_set(x, y)) {
                    boundary.push_back(vec2i(x, y));
                }
            }
        }
    }

};

#endif