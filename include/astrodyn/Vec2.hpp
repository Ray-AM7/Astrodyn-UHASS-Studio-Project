#pragma once

#include <cmath>
#include <ostream>

namespace astrodyn {

struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    Vec2() = default;
    Vec2(double x_, double y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(double s) const { return {x * s, y * s}; }
    Vec2 operator/(double s) const { return {x / s, y / s}; }

    Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2& operator-=(const Vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec2& operator*=(double s) {
        x *= s;
        y *= s;
        return *this;
    }
};

inline Vec2 operator*(double s, const Vec2& v) {
    return {s * v.x, s * v.y};
}

inline double dot(const Vec2& a, const Vec2& b) {
    return a.x * b.x + a.y * b.y;
}

inline double norm2(const Vec2& v) {
    return dot(v, v);
}

inline double norm(const Vec2& v) {
    return std::sqrt(norm2(v));
}

inline Vec2 unit(const Vec2& v) {
    const double n = norm(v);
    if (n <= 0.0) return {0.0, 0.0};
    return v / n;
}

inline Vec2 perpendicular_ccw(const Vec2& v) {
    return {-v.y, v.x};
}

inline std::ostream& operator<<(std::ostream& os, const Vec2& v) {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
}

} // namespace astrodyn