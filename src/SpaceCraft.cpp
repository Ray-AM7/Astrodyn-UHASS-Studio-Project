#include "astrodyn/SpaceCraft.hpp"
#include <algorithm>

namespace astrodyn {

SpaceCraft::SpaceCraft(std::string name, Vec2 position_m, Vec2 velocity_m_s)
    : SpaceObject(std::move(name), SpaceObjectKind::SpaceCraft, position_m, velocity_m_s) {
    mass_kg_ = 500.0;
    radius_m_ = 2.0;
    gravityEnabled_ = false;
    affectedByGravity_ = true;
    continuousBurnAcceleration_m_s2_ = 0.001;
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
    if (variableName == "engineImpulse") {
        engineImpulse_N_s_ = newValue;
        return true;
    }
    if (variableName == "thrust") {
        thrust_N_ = newValue;
        if (mass_kg_ > 0.0) {
            continuousBurnAcceleration_m_s2_ = std::max(0.0, thrust_N_ / mass_kg_);
        }
        return true;
    }
    if (variableName == "dragCoefficient") {
        dragCoefficient_ = std::max(0.0, newValue);
        return true;
    }

    return false;
}

double SpaceCraft::batteryChargeFraction() const { return batteryCharge_fraction_; }
double SpaceCraft::fuelKg() const { return fuel_kg_; }
double SpaceCraft::engineImpulseNs() const { return engineImpulse_N_s_; }
double SpaceCraft::thrustN() const { return thrust_N_; }
double SpaceCraft::dragCoefficient() const { return dragCoefficient_; }

Vec2 SpaceCraft::sunVector() const { return sunVector_; }
Vec2 SpaceCraft::earthVector() const { return earthVector_; }
Vec2 SpaceCraft::starVector() const { return starVector_; }

} // namespace astrodyn