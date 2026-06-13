#include "astrodyn/body_catalog.hpp"
#include "astrodyn/astro.hpp"
#include <cmath>

namespace astrodyn {

namespace {

Vec2 rotate_pos(double r_m, double phase_rad) {
    return {r_m * std::cos(phase_rad), r_m * std::sin(phase_rad)};
}

Vec2 circular_velocity_vec(double mu_m3_s2, double r_m, double phase_rad) {
    const double v = std::sqrt(mu_m3_s2 / r_m);
    return {-v * std::sin(phase_rad), v * std::cos(phase_rad)};
}

SimObject2D make_planet(
    const std::string& name,
    double mu_m3_s2,
    double radius_m,
    double orbit_radius_m,
    double phase_rad,
    int r,
    int g,
    int b,
    float draw_radius_px
) {
    SimObject2D obj;
    obj.name = name;
    obj.type = SimObjectType::MassiveBody;
    obj.mu_m3_s2 = mu_m3_s2;
    obj.radius_m = radius_m;
    obj.state.r_m = rotate_pos(orbit_radius_m, phase_rad);
    obj.state.v_m_s = circular_velocity_vec(MU_SUN_M3_S2, orbit_radius_m, phase_rad);
    obj.gravity_source = true;
    obj.gravity_enabled = true;
    obj.dynamic = true;
    obj.color_r = r;
    obj.color_g = g;
    obj.color_b = b;
    obj.draw_radius_px = draw_radius_px;
    return obj;
}

SimObject2D make_moon_relative(
    const std::string& name,
    const SimObject2D& parent,
    double mu_parent_m3_s2,
    double mu_moon_m3_s2,
    double moon_radius_m,
    double orbit_radius_m,
    double phase_rad,
    int r,
    int g,
    int b,
    float draw_radius_px
) {
    SimObject2D moon;
    moon.name = name;
    moon.type = SimObjectType::MassiveBody;
    moon.mu_m3_s2 = mu_moon_m3_s2;
    moon.radius_m = moon_radius_m;

    const Vec2 rel_r = rotate_pos(orbit_radius_m, phase_rad);
    const Vec2 rel_v = circular_velocity_vec(mu_parent_m3_s2, orbit_radius_m, phase_rad);

    moon.state.r_m = parent.state.r_m + rel_r;
    moon.state.v_m_s = parent.state.v_m_s + rel_v;

    moon.gravity_source = true;
    moon.gravity_enabled = true;
    moon.dynamic = true;
    moon.color_r = r;
    moon.color_g = g;
    moon.color_b = b;
    moon.draw_radius_px = draw_radius_px;
    return moon;
}

} // namespace

SimObject2D make_sun() {
    SimObject2D sun;
    sun.name = "Sun";
    sun.type = SimObjectType::MassiveBody;
    sun.mu_m3_s2 = MU_SUN_M3_S2;
    sun.radius_m = 696340e3;
    sun.state.r_m = {0.0, 0.0};
    sun.state.v_m_s = {0.0, 0.0};
    sun.gravity_source = true;
    sun.gravity_enabled = true;
    sun.dynamic = false;
    sun.color_r = 255;
    sun.color_g = 210;
    sun.color_b = 60;
    sun.draw_radius_px = 16.0f;
    return sun;
}

SimObject2D make_mercury(double phase_rad) {
    return make_planet(
        "Mercury",
        2.2032e13,
        2439.7e3,
        0.387098 * AU_M,
        phase_rad,
        170, 160, 150,
        4.0f
    );
}

SimObject2D make_venus(double phase_rad) {
    return make_planet(
        "Venus",
        3.24859e14,
        6051.8e3,
        0.723332 * AU_M,
        phase_rad,
        230, 190, 120,
        6.0f
    );
}

SimObject2D make_earth(double phase_rad) {
    return make_planet(
        "Earth",
        MU_EARTH_M3_S2,
        R_EARTH_M,
        1.000000 * AU_M,
        phase_rad,
        70, 120, 255,
        7.0f
    );
}

SimObject2D make_moon_around_earth(const SimObject2D& earth, double phase_rad) {
    return make_moon_relative(
        "Moon",
        earth,
        MU_EARTH_M3_S2,
        4.9048695e12,
        1737.4e3,
        384400e3,
        phase_rad,
        190, 190, 190,
        4.0f
    );
}

SimObject2D make_mars(double phase_rad) {
    return make_planet(
        "Mars",
        4.282837e13,
        3389.5e3,
        1.523679 * AU_M,
        phase_rad,
        230, 90, 60,
        6.0f
    );
}

SimObject2D make_phobos_around_mars(const SimObject2D& mars, double phase_rad) {
    return make_moon_relative(
        "Phobos",
        mars,
        4.282837e13,
        7.087e5,
        11.267e3,
        9376e3,
        phase_rad,
        160, 150, 140,
        3.0f
    );
}

SimObject2D make_deimos_around_mars(const SimObject2D& mars, double phase_rad) {
    return make_moon_relative(
        "Deimos",
        mars,
        4.282837e13,
        9.62e4,
        6.2e3,
        23463e3,
        phase_rad,
        150, 145, 135,
        3.0f
    );
}

SimObject2D make_ceres(double phase_rad) {
    return make_planet(
        "Ceres",
        6.26325e10,
        469.7e3,
        2.7675 * AU_M,
        phase_rad,
        160, 160, 160,
        3.5f
    );
}

SimObject2D make_spacecraft_near_earth_leo(const SimObject2D& earth, double altitude_m, double phase_rad) {
    SimObject2D sc;
    sc.name = "Spacecraft";
    sc.type = SimObjectType::Spacecraft;
    sc.mu_m3_s2 = 0.0;
    sc.radius_m = 0.0;
    sc.state.m_kg = 500.0;

    const double r_m = R_EARTH_M + altitude_m;
    const Vec2 rel_r = rotate_pos(r_m, phase_rad);
    const Vec2 rel_v = circular_velocity_vec(MU_EARTH_M3_S2, r_m, phase_rad);

    sc.state.r_m = earth.state.r_m + rel_r;
    sc.state.v_m_s = earth.state.v_m_s + rel_v;

    sc.gravity_source = false;
    sc.gravity_enabled = false;
    sc.dynamic = true;

    sc.color_r = 100;
    sc.color_g = 255;
    sc.color_b = 160;
    sc.draw_radius_px = 5.0f;
    return sc;
}

SandboxWorld2D make_empty_world() {
    SandboxWorld2D w;
    w.name = "Empty Sandbox";
    w.dt_s = 60.0;
    w.steps_per_frame = 4;
    w.meters_per_pixel = 1.0e9;
    w.camera_center_m = {0.0, 0.0};
    w.default_energy_mu_m3_s2 = MU_SUN_M3_S2;
    return w;
}

SandboxWorld2D make_solar_system_inner_preset() {
    SandboxWorld2D w;
    w.name = "Inner Solar System Sandbox";
    w.dt_s = 0.25 * DAY_S;
    w.steps_per_frame = 2;
    w.meters_per_pixel = 1.1e9;
    w.camera_center_m = {0.0, 0.0};
    w.default_energy_mu_m3_s2 = MU_SUN_M3_S2;

    auto sun = make_sun();
    auto mercury = make_mercury(0.2);
    auto venus = make_venus(1.4);
    auto earth = make_earth(2.3);
    auto moon = make_moon_around_earth(earth, 0.7);
    auto mars = make_mars(3.8);
    auto phobos = make_phobos_around_mars(mars, 1.0);
    auto deimos = make_deimos_around_mars(mars, 2.2);
    auto ceres = make_ceres(5.0);

    w.objects.push_back(sun);
    w.objects.push_back(mercury);
    w.objects.push_back(venus);
    w.objects.push_back(earth);
    w.objects.push_back(moon);
    w.objects.push_back(mars);
    w.objects.push_back(phobos);
    w.objects.push_back(deimos);
    w.objects.push_back(ceres);

    w.selected_index = 3; // Earth
    return w;
}

SandboxWorld2D make_earth_sandbox_preset() {
    SandboxWorld2D w;
    w.name = "Earth Editable Sandbox";
    w.dt_s = 10.0;
    w.steps_per_frame = 4;
    w.meters_per_pixel = 20000.0;
    w.camera_center_m = {0.0, 0.0};
    w.default_energy_mu_m3_s2 = MU_EARTH_M3_S2;

    SimObject2D earth;
    earth.name = "Earth";
    earth.type = SimObjectType::MassiveBody;
    earth.mu_m3_s2 = MU_EARTH_M3_S2;
    earth.radius_m = R_EARTH_M;
    earth.state.r_m = {0.0, 0.0};
    earth.state.v_m_s = {0.0, 0.0};
    earth.gravity_source = true;
    earth.gravity_enabled = true;
    earth.dynamic = false;
    earth.color_r = 70;
    earth.color_g = 120;
    earth.color_b = 255;
    earth.draw_radius_px = 18.0f;

    w.objects.push_back(earth);
    w.objects.push_back(make_spacecraft_near_earth_leo(w.objects[0], 500e3, 0.0));

    w.selected_index = 1;
    w.active_vehicle_index = 1;
    return w;
}

} // namespace astrodyn