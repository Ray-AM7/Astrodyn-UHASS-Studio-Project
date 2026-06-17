#pragma once

#include "astrodyn/Vec2.hpp"
#include <string>

namespace astrodyn {

constexpr double G = 6.67430e-11;
constexpr double AU = 1.495978707e11;
constexpr double DAY = 86400.0;
constexpr double G0 = 9.80665;

enum class PropagationMethod {
    SymplecticEuler,
    VelocityVerlet,
    RK4
};

enum class SpaceObjectKind {
    LargeBody,
    SmallBody,
    SpaceCraft,
    TestParticle
};

struct ForceState {
    Vec2 acceleration_m_s2{0.0, 0.0};
};

inline std::string propagationMethodName(PropagationMethod method) {
    switch (method) {
        case PropagationMethod::SymplecticEuler: return "Symplectic Euler";
        case PropagationMethod::VelocityVerlet: return "Velocity Verlet";
        case PropagationMethod::RK4: return "RK4";
    }
    return "Unknown";
}

inline std::string objectKindName(SpaceObjectKind kind) {
    switch (kind) {
        case SpaceObjectKind::LargeBody: return "LargeBody";
        case SpaceObjectKind::SmallBody: return "SmallBody";
        case SpaceObjectKind::SpaceCraft: return "SpaceCraft";
        case SpaceObjectKind::TestParticle: return "TestParticle";
    }
    return "Unknown";
}

} // namespace astrodyn