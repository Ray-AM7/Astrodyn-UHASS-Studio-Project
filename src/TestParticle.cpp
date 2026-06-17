#include "astrodyn/TestParticle.hpp"
#include <algorithm>

namespace astrodyn {

TestParticle::TestParticle(std::string name, Vec2 position_m, Vec2 velocity_m_s)
    : SpaceObject(std::move(name), SpaceObjectKind::TestParticle, position_m, velocity_m_s) {
    mass_kg_ = 1.0;
    radius_m_ = 1.0;
    gravityEnabled_ = false;
    affectedByGravity_ = true;
    drawRadiusPx_ = 3.0f;
    setColor(255, 255, 255);
}

std::unique_ptr<SpaceObject> TestParticle::clone() const {
    return std::make_unique<TestParticle>(*this);
}

bool TestParticle::editObject(const std::string& variableName, double newValue) {
    if (SpaceObject::editObject(variableName, newValue)) return true;

    if (variableName == "particleArea") {
        particleArea_m2_ = std::max(0.0, newValue);
        return true;
    }
    if (variableName == "areaDensity") {
        areaDensity_kg_m2_ = std::max(0.0, newValue);
        return true;
    }
    if (variableName == "shape") {
        shapeId_ = static_cast<int>(newValue);
        return true;
    }

    return false;
}

} // namespace astrodyn