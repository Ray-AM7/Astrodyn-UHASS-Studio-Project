#pragma once

#include "astrodyn/astro.hpp"
#include <functional>

namespace astrodyn {

using AccelFunction = std::function<Vec2(const State2D& state, double t_s)>;

struct FiniteBurnCommand {
    bool active = false;
    double thrust_N = 0.0;
    double isp_s = 0.0;
    enum class DirectionMode { Prograde, Retrograde, RadialOut, RadialIn, InertialFixed } direction_mode = DirectionMode::Prograde;
    Vec2 inertial_unit = {1.0, 0.0};
};

Derivative2D derivative(const State2D& state, double t_s, const AccelFunction& accel, const FiniteBurnCommand& burn = {});

void step_explicit_euler(State2D& state, double t_s, double dt_s, const AccelFunction& accel);
void step_symplectic_euler(State2D& state, double t_s, double dt_s, const AccelFunction& accel);
void step_velocity_verlet(State2D& state, double t_s, double dt_s, const AccelFunction& accel);
void step_rk4(State2D& state, double t_s, double dt_s, const AccelFunction& accel, const FiniteBurnCommand& burn = {});

// Drift-kick-drift leapfrog for many mutually gravitating bodies.
// This is useful for long-term N-body experiments where all bodies move.
void step_nbody_velocity_verlet(std::vector<Body2D>& bodies, double dt_s);

} // namespace astrodyn
