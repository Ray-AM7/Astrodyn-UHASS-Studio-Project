#pragma once

#include "astrodyn/SpaceObject.hpp"

namespace astrodyn {

class LargeBody : public SpaceObject {
public:
    LargeBody(
        std::string name = "LargeBody",
        Vec2 position_m = {0.0, 0.0},
        Vec2 velocity_m_s = {0.0, 0.0}
    );

    std::unique_ptr<SpaceObject> clone() const override;

    bool editObject(const std::string& variableName, double newValue) override;

    double atmosphereRadiusM() const;
    double atmosphereDensityKgM3() const;
    double averageTempK() const;

    void setAtmosphereRadiusM(double value);
    void setAtmosphereDensityKgM3(double value);
    void setAverageTempK(double value);

private:
    double atmosphereRadius_m_ = 0.0;
    double atmosphereDensity_kg_m3_ = 0.0;
    double averageTemp_K_ = 0.0;
};

} // namespace astrodyn