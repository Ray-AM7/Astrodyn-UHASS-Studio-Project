#include "astrodyn/LargeBody.hpp"
#include <algorithm>

namespace astrodyn {

LargeBody::LargeBody(std::string name, Vec2 position_m, Vec2 velocity_m_s)
    : SpaceObject(std::move(name), SpaceObjectKind::LargeBody, position_m, velocity_m_s) {
    mass_kg_ = 1.0e20;
    radius_m_ = 1.0e6;
    atmosphereRadius_m_ = radius_m_;
    atmosphereDensity_kg_m3_ = 0.0;
    averageTemp_K_ = 0.0;

    gravityEnabled_ = true;
    affectedByGravity_ = true;
    drawRadiusPx_ = 10.0f;
    setColor(120, 160, 255);
}

std::unique_ptr<SpaceObject> LargeBody::clone() const {
    return std::make_unique<LargeBody>(*this);
}

bool LargeBody::editObject(const std::string& variableName, double newValue) {
    if (SpaceObject::editObject(variableName, newValue)) return true;

    if (variableName == "atmosphereRadius") {
        setAtmosphereRadiusM(newValue);
        return true;
    }
    if (variableName == "atmosphereDensity") {
        setAtmosphereDensityKgM3(newValue);
        return true;
    }
    if (variableName == "averageTemp") {
        setAverageTempK(newValue);
        return true;
    }

    return false;
}

double LargeBody::atmosphereRadiusM() const { return atmosphereRadius_m_; }
double LargeBody::atmosphereDensityKgM3() const { return atmosphereDensity_kg_m3_; }
double LargeBody::averageTempK() const { return averageTemp_K_; }

void LargeBody::setAtmosphereRadiusM(double value) { atmosphereRadius_m_ = std::max(0.0, value); }
void LargeBody::setAtmosphereDensityKgM3(double value) { atmosphereDensity_kg_m3_ = std::max(0.0, value); }
void LargeBody::setAverageTempK(double value) { averageTemp_K_ = std::max(0.0, value); }

} // namespace astrodyn