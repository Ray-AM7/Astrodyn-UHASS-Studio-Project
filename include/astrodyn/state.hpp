#pragma once

#include "astrodyn/vec2.hpp"
#include <string>
#include <vector>

namespace astrodyn {

struct State2D {
    Vec2 r_m;      // inertial position, meters
    Vec2 v_m_s;    // inertial velocity, meters/second
    double m_kg = 0.0; // optional spacecraft/test-particle mass. Use 0 for massless body.
};

struct Derivative2D {
    Vec2 dr_m_s;       // dr/dt = velocity
    Vec2 dv_m_s2;      // dv/dt = acceleration
    double dm_kg_s = 0.0;
};

struct Body2D {
    std::string name;
    double mu_m3_s2 = 0.0;
    double radius_m = 0.0;
    State2D state;
    bool fixed = false; // true means it acts as an immovable gravity source.
};

struct Spacecraft2D {
    std::string name = "spacecraft";
    State2D state;
    double dry_mass_kg = 0.0;
    double prop_mass_kg = 0.0;
    double isp_s = 0.0;
    double max_thrust_N = 0.0;
};

struct SampleRow {
    double t_s = 0.0;
    std::string object;
    double x_m = 0.0;
    double y_m = 0.0;
    double vx_m_s = 0.0;
    double vy_m_s = 0.0;
    double mass_kg = 0.0;
    double radius_m = 0.0;
    double speed_m_s = 0.0;
    double energy_J_kg = 0.0;
    double h_m2_s = 0.0;
};

} // namespace astrodyn
