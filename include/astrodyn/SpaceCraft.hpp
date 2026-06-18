#pragma once

#include "astrodyn/SpaceObject.hpp"

namespace astrodyn {

struct Quaternion {
    double w = 1.0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

class SpaceCraft : public SpaceObject {
public:
    SpaceCraft(
        std::string name = "SpaceCraft",
        Vec2 position_m = {0.0, 0.0},
        Vec2 velocity_m_s = {0.0, 0.0}
    );

    std::unique_ptr<SpaceObject> clone() const override;

    bool editObject(const std::string& variableName, double newValue) override;

    double batteryChargeFraction() const;
    double fuelKg() const;
    double engineImpulseNs() const;
    double thrustN() const;
    double dragCoefficient() const;

    Vec2 sunVector() const;
    Vec2 earthVector() const;
    Vec2 starVector() const;

private:
    double batteryCharge_fraction_ = 1.0;
    double fuel_kg_ = 100.0;
    double engineImpulse_N_s_ = 0.0;
    double thrust_N_ = 0.0;
    double dragCoefficient_ = 2.2;

    Vec2 sunVector_{1.0, 0.0};
    Vec2 earthVector_{1.0, 0.0};
    Vec2 starVector_{1.0, 0.0};

    Quaternion attitude_;
};

} // namespace astrodyn