#include "astrodyn/TestParticle.hpp"
#include <algorithm>

namespace astrodyn {

TestParticle::TestParticle(std::string name, Vec2 position_m, Vec2 velocity_m_s)
    : SpaceObject(std::move(name), SpaceObjectKind::TestParticle, position_m, velocity_m_s) {
    particleArea_m2_ = 1.0;
    areaDensity_kg_m2_ = 1.0;
    mass_kg_ = particleArea_m2_ * areaDensity_kg_m2_;

    radius_m_ = 1.0;
    gravityEnabled_ = false;
    affectedByGravity_ = true;
    continuousBurnAcceleration_m_s2_ = 0.001;
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
        mass_kg_ = particleArea_m2_ * areaDensity_kg_m2_;
        return true;
    }
    if (variableName == "areaDensity") {
        areaDensity_kg_m2_ = std::max(0.0, newValue);
        mass_kg_ = particleArea_m2_ * areaDensity_kg_m2_;
        return true;
    }
    if (variableName == "shape") {
        shapeId_ = static_cast<int>(newValue);
        return true;
    }
    if (variableName == "mode") {
        mode_ = newValue < 0.5 ? TestParticleMode::MonteCarlo : TestParticleMode::StatisticalTracing;
        return true;
    }

    return false;
}

double TestParticle::particleAreaM2() const { return particleArea_m2_; }
double TestParticle::areaDensityKgM2() const { return areaDensity_kg_m2_; }
int TestParticle::shapeId() const { return shapeId_; }

TestParticleMode TestParticle::mode() const { return mode_; }
void TestParticle::setMode(TestParticleMode mode) { mode_ = mode; }

} // namespace astrodyn