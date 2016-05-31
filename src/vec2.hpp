#ifndef __VEC2_HPP_
#define __VEC2_HPP_

template <typename T>
struct vec2 {
    T x, y;
    
    inline vec2() : x(0), y(0) {}
    inline vec2(T _x, T _y) : x(_x), y(_y) {}

    inline vec2 operator +(const vec2& rhs) const { return vec2{x + rhs.x, y + rhs.y}; }
    inline vec2 operator -(const vec2& rhs) const { return vec2{x - rhs.x, y - rhs.y}; }
    inline bool operator ==(const vec2& rhs) const { return x == rhs.x && y == rhs.y; }

    inline vec2& operator +=(const vec2& rhs) {  
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    inline T len2() const { return x*x + y*y; }
    inline double len() const { return sqrt((double)len2()); }
};

typedef vec2<float> vec2f;
typedef vec2<int> vec2i;

#endif
