#include "astrodyn/SmallBody.hpp"
#include <algorithm>

namespace astrodyn {

SmallBody::SmallBody(std::string name, Vec2 position_m, Vec2 velocity_m_s)
    : SpaceObject(std::move(name), SpaceObjectKind::SmallBody, position_m, velocity_m_s) {
    mass_kg_ = 1.0e12;
    radius_m_ = 1000.0;
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

} // namespace astrodyn