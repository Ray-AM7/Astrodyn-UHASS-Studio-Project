#include "astrodyn/vec2.hpp"

namespace astrodyn {

Vec2 operator*(double scalar, const Vec2& v) { return v * scalar; }

double dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }

double cross_z(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }

double norm2(const Vec2& v) { return dot(v, v); }

double norm(const Vec2& v) { return std::sqrt(norm2(v)); }

Vec2 unit(const Vec2& v) {
    const double n = norm(v);
    if (n == 0.0) throw std::runtime_error("Cannot normalize zero vector");
    return v / n;
}

Vec2 rotate90_ccw(const Vec2& v) { return {-v.y, v.x}; }

Vec2 rotate(const Vec2& v, double angle_rad) {
    const double c = std::cos(angle_rad);
    const double s = std::sin(angle_rad);
    return {c * v.x - s * v.y, s * v.x + c * v.y};
}

std::ostream& operator<<(std::ostream& os, const Vec2& v) {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
}

} // namespace astrodyn
