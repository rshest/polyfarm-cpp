#ifndef __TRACE_BITMAP__
#define __TRACE_BITMAP__

#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <map>

class rect_contour {
public:
    struct point { 
        int x, y; 
        inline bool operator <(const point& rhs) const {
            if (x == rhs.x) return y < rhs.y;
            return x < rhs.x;
        }
        inline bool operator ==(const point& rhs) const {
            return (x == rhs.x && y == rhs.y);
        }


    };

    struct edge { point a, b; };

    typedef std::vector<point> chain;
    typedef std::vector<chain> chain_vec;

    static inline bool are_collinear(const edge& e1, const edge& e2) {
        return ((e1.a.x == e1.b.x) && (e1.b.x == e2.a.x) && (e2.a.x == e2.b.x)) ||
            ((e1.a.y == e1.b.y) && (e1.b.y == e2.a.y) && (e2.a.y == e2.b.y));
    }

    static inline bool are_clockwise(const edge& e1, const edge& e2) {
        int x1 = e1.b.x - e1.a.x;
        int y1 = e1.b.y - e1.a.y;
        int x2 = e2.b.x - e2.a.x;
        int y2 = e2.b.y - e2.a.y;
        int cp = (x1*y2) - (y1*x2);
        return cp < 0;
    };

    rect_contour() {}
    rect_contour(const chain_vec& _chains) : chains(_chains) {}

    inline void trace_bitmap(const std::vector<bool>& pixels, int bitmap_width, 
        point ext = {1, 1}, bool join_diagonals = true) {

        //  extract edges of every pixel's square, clockwise
        std::vector<edge> edges;
        const size_t npixels = pixels.size();
        for (size_t i = 0; i < npixels; i++) {
            if (pixels[i]) {
                int cx = i%bitmap_width;
                int cy = ((int)i)/bitmap_width;
                edges.push_back({{ext.x*(cx    ), ext.y*(cy    )}, {ext.x*(cx + 1), ext.y*(cy    )}});
                edges.push_back({{ext.x*(cx + 1), ext.y*(cy    )}, {ext.x*(cx + 1), ext.y*(cy + 1)}});
                edges.push_back({{ext.x*(cx + 1), ext.y*(cy + 1)}, {ext.x*(cx    ), ext.y*(cy + 1)}});
                edges.push_back({{ext.x*(cx    ), ext.y*(cy + 1)}, {ext.x*(cx    ), ext.y*(cy    )}});
            }
        }

        //  insert into the endpoints registry, eliminating double edges on the way
        std::multimap<point, edge*> start_points;
        for (auto& e : edges) {
            auto er = start_points.equal_range(e.b);
            bool present = false;
            for (auto it = er.first; it != er.second; ++it) {
                const edge* pe = it->second;
                if (pe->b == e.a) {
                    //  have the same edge, going in opposite direction
                    //  - cancel it out
                    start_points.erase(it);
                    present = true;
                    break;
                }
            }
            if (!present) start_points.emplace(e.a, &e);
        }

        //  traverse the contours, collapsing aligned segments
        edge cur_edge = { {-1, -1}, {-1, -1 } };
        while (!start_points.empty()) {
            auto er = start_points.equal_range(cur_edge.b);
            if (er.first == er.second || chains.empty()) {
                //  start new chain inside the rect_contour
                cur_edge.a = cur_edge.b = start_points.begin()->first;
                chains.push_back({cur_edge.a});
                continue;
            }
            auto it = er.first;
            auto it1 = it;
            ++it1;
            if (it1 != er.second) {
                //  at this point we still may have more than one outgoing edge, e.g:
                //  o|
                //   |o
                const edge* pe = it1->second;
                if (are_clockwise(cur_edge, *pe) == join_diagonals) it = it1;
                ++it1;
                assert(it1 == er.second);
            }
            const edge* pe = it->second;
            std::vector<point>& cc = chains.back();
            if (are_collinear(cur_edge, *pe)) {
                cur_edge.b = pe->b;
            } else {
                cc.push_back({cur_edge.b});
                cur_edge = *pe;
            }
            start_points.erase(it);
        }
    }

    std::string svg_path() const {
        std::stringstream ss;
        for (auto& chain : chains) {
            ss << "M";
            for (auto& p : chain) {
                ss << p.x << " " << p.y << " ";
            }
            ss << "z ";
        }
        return ss.str();
    }

    void extrude(int dx, int dy) {
        for (auto& chain : chains) {
            const size_t np = chain.size();
            for (size_t i = 1; i <= np; i++) {
                point& a = chain[i - 1];
                point& b = chain[i%np];
                int offx = 0, offy = 0;
                if (b.y == a.y) {
                    offy = (b.x > a.x) ? dy : -dy;
                } else if (b.x == a.x) {
                    offx = (b.y > a.y) ? -dx : dx;
                } else {
                    assert(!"Expecting only orthogonal segments in the contour!");
                }
                a.x += offx;
                a.y += offy;
                b.x += offx;
                b.y += offy;
            }
        }
    }

    chain_vec chains;
};





#endif // __TRACE_BITMAP__
