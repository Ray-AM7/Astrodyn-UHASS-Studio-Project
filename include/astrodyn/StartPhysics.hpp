#pragma once

#include "astrodyn/PhysicsTypes.hpp"
#include <string>

namespace astrodyn {

class StartPhysics {
public:
    virtual ~StartPhysics() = default;

    virtual bool editObject(const std::string& variableName, double newValue) = 0;

    virtual void propagateObj(
        double timestep_s,
        PropagationMethod method,
        const ForceState& forces
    ) = 0;
};

} // namespace astrodyn