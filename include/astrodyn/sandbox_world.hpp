#pragma once

#include "astrodyn/state.hpp"
#include <string>
#include <vector>

namespace astrodyn {

enum class SimObjectType {
    MassiveBody,
    Spacecraft,
    TestParticle
};

enum class IntegratorChoice {
    ExplicitEuler,
    SymplecticEuler,
    VelocityVerlet,
    RK4
};

struct SimObject2D {
    std::string name;
    SimObjectType type = SimObjectType::MassiveBody;

    State2D state;

    double mu_m3_s2 = 0.0;
    double radius_m = 0.0;

    bool gravity_source = true;
    bool gravity_enabled = true;
    bool dynamic = true;
    bool selected = false;

    // Visual settings
    float draw_radius_px = 6.0f;
    int color_r = 255;
    int color_g = 255;
    int color_b = 255;

    // Trail history
    std::vector<Vec2> trail_m;
};

struct SandboxWorld2D {
    std::string name = "Untitled Sandbox";

    std::vector<SimObject2D> objects;

    int selected_index = -1;
    int active_vehicle_index = -1;

    double t_s = 0.0;
    double dt_s = 10.0;
    int steps_per_frame = 4;

    double meters_per_pixel = 1.0e7;
    Vec2 camera_center_m{0.0, 0.0};

    IntegratorChoice integrator = IntegratorChoice::VelocityVerlet;

    bool paused = false;
    bool finite_burn_on = false;

    double default_energy_mu_m3_s2 = 0.0;
};

} // namespace astrodyn