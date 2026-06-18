#pragma once

#include "astrodyn/SpaceObject.hpp"

namespace astrodyn {

class TestParticle : public SpaceObject {
public:
    TestParticle(
        std::string name = "TestParticle",
        Vec2 position_m = {0.0, 0.0},
        Vec2 velocity_m_s = {0.0, 0.0}
    );

    std::unique_ptr<SpaceObject> clone() const override;

    bool editObject(const std::string& variableName, double newValue) override;

    double particleAreaM2() const;
    double areaDensityKgM2() const;
    int shapeId() const;

    TestParticleMode mode() const;
    void setMode(TestParticleMode mode);

private:
    double particleArea_m2_ = 1.0;
    double areaDensity_kg_m2_ = 1.0;
    int shapeId_ = 0;

    TestParticleMode mode_ = TestParticleMode::MonteCarlo;
};

} // namespace astrodyn