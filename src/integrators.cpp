#include "astrodyn/integrators.hpp"
#include <cmath>
#include <stdexcept>
#include <vector>

namespace astrodyn {

static Vec2 burn_direction(const State2D& state, const FiniteBurnCommand& burn) {
    switch (burn.direction_mode) {
        case FiniteBurnCommand::DirectionMode::Prograde: return prograde_unit(state);
        case FiniteBurnCommand::DirectionMode::Retrograde: return retrograde_unit(state);
        case FiniteBurnCommand::DirectionMode::RadialOut: return radial_out_unit(state);
        case FiniteBurnCommand::DirectionMode::RadialIn: return radial_in_unit(state);
        case FiniteBurnCommand::DirectionMode::InertialFixed: return unit(burn.inertial_unit);
    }
    return {1.0, 0.0};
}

Derivative2D derivative(const State2D& state, double t_s, const AccelFunction& accel, const FiniteBurnCommand& burn) {
    Derivative2D d;
    d.dr_m_s = state.v_m_s;
    d.dv_m_s2 = accel(state, t_s);
    d.dm_kg_s = 0.0;

    if (burn.active && burn.thrust_N > 0.0) {
        if (state.m_kg <= 0.0) throw std::runtime_error("Finite burn requires positive spacecraft mass");
        if (burn.isp_s <= 0.0) throw std::runtime_error("Finite burn requires positive Isp");
        const Vec2 u = burn_direction(state, burn);
        d.dv_m_s2 += (burn.thrust_N / state.m_kg) * u;
        d.dm_kg_s = -burn.thrust_N / (burn.isp_s * G0_M_S2);
    }

    return d;
}

void step_explicit_euler(State2D& state, double t_s, double dt_s, const AccelFunction& accel) {
    const Vec2 a = accel(state, t_s);
    // Intentionally uses old position and old velocity for both updates.
    state.r_m += state.v_m_s * dt_s;
    state.v_m_s += a * dt_s;
}

void step_symplectic_euler(State2D& state, double t_s, double dt_s, const AccelFunction& accel) {
    const Vec2 a = accel(state, t_s);
    // Velocity first, then position using the fresh velocity.
    state.v_m_s += a * dt_s;
    state.r_m += state.v_m_s * dt_s;
}

void step_velocity_verlet(State2D& state, double t_s, double dt_s, const AccelFunction& accel) {
    const Vec2 a0 = accel(state, t_s);
    state.r_m += state.v_m_s * dt_s + 0.5 * a0 * dt_s * dt_s;
    State2D tmp = state;
    const Vec2 a1 = accel(tmp, t_s + dt_s);
    state.v_m_s += 0.5 * (a0 + a1) * dt_s;
}

static State2D add_scaled(const State2D& s, const Derivative2D& d, double scale) {
    State2D out = s;
    out.r_m += d.dr_m_s * scale;
    out.v_m_s += d.dv_m_s2 * scale;
    out.m_kg += d.dm_kg_s * scale;
    if (out.m_kg < 0.0) out.m_kg = 0.0;
    return out;
}

void step_rk4(State2D& state, double t_s, double dt_s, const AccelFunction& accel, const FiniteBurnCommand& burn) {
    const Derivative2D k1 = derivative(state, t_s, accel, burn);
    const Derivative2D k2 = derivative(add_scaled(state, k1, 0.5 * dt_s), t_s + 0.5 * dt_s, accel, burn);
    const Derivative2D k3 = derivative(add_scaled(state, k2, 0.5 * dt_s), t_s + 0.5 * dt_s, accel, burn);
    const Derivative2D k4 = derivative(add_scaled(state, k3, dt_s), t_s + dt_s, accel, burn);

    state.r_m += (dt_s / 6.0) * (k1.dr_m_s + 2.0 * k2.dr_m_s + 2.0 * k3.dr_m_s + k4.dr_m_s);
    state.v_m_s += (dt_s / 6.0) * (k1.dv_m_s2 + 2.0 * k2.dv_m_s2 + 2.0 * k3.dv_m_s2 + k4.dv_m_s2);
    state.m_kg += (dt_s / 6.0) * (k1.dm_kg_s + 2.0 * k2.dm_kg_s + 2.0 * k3.dm_kg_s + k4.dm_kg_s);
    if (state.m_kg < 0.0) state.m_kg = 0.0;
}

static std::vector<Vec2> nbody_accels(const std::vector<Body2D>& bodies) {
    std::vector<Vec2> accels(bodies.size(), Vec2{0.0, 0.0});

    for (size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].fixed) continue;
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (i == j) continue;
            accels[i] += two_body_accel(bodies[j].mu_m3_s2, bodies[i].state.r_m, bodies[j].state.r_m);
        }
    }
    return accels;
}

void step_nbody_velocity_verlet(std::vector<Body2D>& bodies, double dt_s) {
    const auto a0 = nbody_accels(bodies);

    for (size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].fixed) continue;
        bodies[i].state.r_m += bodies[i].state.v_m_s * dt_s + 0.5 * a0[i] * dt_s * dt_s;
    }

    const auto a1 = nbody_accels(bodies);

    for (size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].fixed) continue;
        bodies[i].state.v_m_s += 0.5 * (a0[i] + a1[i]) * dt_s;
    }
}

} // namespace astrodyn
