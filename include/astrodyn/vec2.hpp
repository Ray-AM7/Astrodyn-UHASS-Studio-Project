#pragma once

#include <cmath>
#include <ostream>
#include <stdexcept>

namespace astrodyn {

struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    Vec2() = default;
    Vec2(double x_in, double y_in) : x(x_in), y(y_in) {}

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(double scalar) const { return {x * scalar, y * scalar}; }
    Vec2 operator/(double scalar) const {
        if (scalar == 0.0) throw std::runtime_error("Vec2 divide by zero");
        return {x / scalar, y / scalar};
    }

    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2& operator*=(double scalar) { x *= scalar; y *= scalar; return *this; }
};

Vec2 operator*(double scalar, const Vec2& v);
double dot(const Vec2& a, const Vec2& b);
double cross_z(const Vec2& a, const Vec2& b);
double norm2(const Vec2& v);
double norm(const Vec2& v);
Vec2 unit(const Vec2& v);
Vec2 rotate90_ccw(const Vec2& v);
Vec2 rotate(const Vec2& v, double angle_rad);

std::ostream& operator<<(std::ostream& os, const Vec2& v);

} // namespace astrodyn
