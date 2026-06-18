#include "astrodyn/SmallBody.hpp"
#include <algorithm>
#include <cmath>

namespace astrodyn {

SmallBody::SmallBody(std::string name, Vec2 position_m, Vec2 velocity_m_s)
    : SpaceObject(std::move(name), SpaceObjectKind::SmallBody, position_m, velocity_m_s) {
    radius_m_ = 1000.0;
    density_kg_m3_ = 2000.0;

    // 2D phase: use area-density-ish approximation for now.
    // Later in 3D, change to volume: 4/3*pi*r^3*rho.
    mass_kg_ = 3.141592653589793 * radius_m_ * radius_m_ * density_kg_m3_;

    gravityEnabled_ = true;
    affectedByGravity_ = true;
    drawRadiusPx_ = 4.0f;
    setColor(170, 170, 170);
}

std::unique_ptr<SpaceObject> SmallBody::clone() const {
    return std::make_unique<SmallBody>(*this);
}

bool SmallBody::editObject(const std::string& variableName, double newValue) {
    if (SpaceObject::editObject(variableName, newValue)) return true;

    if (variableName == "density") {
        density_kg_m3_ = std::max(0.0, newValue);
        mass_kg_ = 3.141592653589793 * radius_m_ * radius_m_ * density_kg_m3_;
        return true;
    }
    if (variableName == "iceContent") {
        iceContent_fraction_ = std::clamp(newValue, 0.0, 1.0);
        return true;
    }
    if (variableName == "averageTemp") {
        averageTemp_K_ = std::max(0.0, newValue);
        return true;
    }

    return false;
}

double SmallBody::densityKgM3() const { return density_kg_m3_; }
double SmallBody::iceContentFraction() const { return iceContent_fraction_; }
double SmallBody::averageTempK() const { return averageTemp_K_; }

} // namespace astrodyn