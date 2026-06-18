#pragma once

#include "astrodyn/SpaceObject.hpp"

namespace astrodyn {

class SmallBody : public SpaceObject {
public:
    SmallBody(
        std::string name = "SmallBody",
        Vec2 position_m = {0.0, 0.0},
        Vec2 velocity_m_s = {0.0, 0.0}
    );

    std::unique_ptr<SpaceObject> clone() const override;

    bool editObject(const std::string& variableName, double newValue) override;

    double densityKgM3() const;
    double iceContentFraction() const;
    double averageTempK() const;

private:
    double density_kg_m3_ = 2000.0;
    double iceContent_fraction_ = 0.0;
    double averageTemp_K_ = 0.0;
};

} // namespace astrodyn