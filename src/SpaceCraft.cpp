#include "astrodyn/SpaceCraft.hpp"
#include <algorithm>

namespace astrodyn {

SpaceCraft::SpaceCraft(std::string name, Vec2 position_m, Vec2 velocity_m_s)
    : SpaceObject(std::move(name), SpaceObjectKind::SpaceCraft, position_m, velocity_m_s) {
    mass_kg_ = 500.0;
    radius_m_ = 2.0;
    gravityEnabled_ = false;
    affectedByGravity_ = true;
    drawRadiusPx_ = 5.0f;
    setColor(80, 255, 160);
}

std::unique_ptr<SpaceObject> SpaceCraft::clone() const {
    return std::make_unique<SpaceCraft>(*this);
}

bool SpaceCraft::editObject(const std::string& variableName, double newValue) {
    if (SpaceObject::editObject(variableName, newValue)) return true;

    if (variableName == "battery") {
        batteryCharge_fraction_ = std::clamp(newValue, 0.0, 1.0);
        return true;
    }
    if (variableName == "fuel") {
        fuel_kg_ = std::max(0.0, newValue);
        return true;
    }
    if (variableName == "impulse") {
        impulse_N_s_ = newValue;
        return true;
    }
    if (variableName == "thrust") {
        thrust_N_ = newValue;
        return true;
    }
    if (variableName == "dragCoefficient") {
        dragCoefficient_ = std::max(0.0, newValue);
        return true;
    }

    return false;
}

} // namespace astrodyn