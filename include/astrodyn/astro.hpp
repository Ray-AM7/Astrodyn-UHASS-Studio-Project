#pragma once

#include "astrodyn/state.hpp"
#include <vector>

namespace astrodyn {

constexpr double G_SI = 6.67430e-11;
constexpr double G0_M_S2 = 9.80665;
constexpr double MU_EARTH_M3_S2 = 3.986004418e14;
constexpr double R_EARTH_M = 6378137.0;
constexpr double MU_SUN_M3_S2 = 1.32712440018e20;
constexpr double MU_JUPITER_M3_S2 = 1.26686534e17;
constexpr double AU_M = 1.495978707e11;
constexpr double DAY_S = 86400.0;

struct OrbitalElements2D {
    double specific_energy_J_kg = 0.0;
    double specific_angular_momentum_m2_s = 0.0;
    double semi_major_axis_m = 0.0;
    double eccentricity = 0.0;
    double periapsis_m = 0.0;
    double apoapsis_m = 0.0;
    double true_anomaly_rad = 0.0;
};

double circular_speed(double mu_m3_s2, double radius_m);
double escape_speed(double mu_m3_s2, double radius_m);
double orbital_period(double mu_m3_s2, double semi_major_axis_m);
double specific_energy(double mu_m3_s2, const State2D& state);
double specific_angular_momentum_2d(const State2D& state);
OrbitalElements2D elements_from_state(double mu_m3_s2, const State2D& state);
State2D circular_orbit_state(double central_radius_m, double altitude_m, double mu_m3_s2, double phase_rad = 0.0, double mass_kg = 0.0);

Vec2 two_body_accel(double mu_m3_s2, const Vec2& r_sc_m, const Vec2& r_central_m = {0.0, 0.0});
Vec2 n_body_accel_on_test_particle(const Vec2& r_sc_m, const std::vector<Body2D>& grav_bodies);

struct HohmannResult {
    double dv1_m_s = 0.0;
    double dv2_m_s = 0.0;
    double transfer_time_s = 0.0;
    double total_dv_m_s = 0.0;
};
HohmannResult hohmann_coplanar(double mu_m3_s2, double r1_m, double r2_m);

void apply_impulsive_burn(State2D& state, const Vec2& delta_v_m_s);
Vec2 prograde_unit(const State2D& state);
Vec2 retrograde_unit(const State2D& state);
Vec2 radial_out_unit(const State2D& state);
Vec2 radial_in_unit(const State2D& state);

double rocket_final_mass(double m0_kg, double delta_v_m_s, double isp_s);
double rocket_delta_v(double m0_kg, double mf_kg, double isp_s);

SampleRow make_sample(double t_s, const std::string& object, const State2D& state, double central_mu_m3_s2 = 0.0);

} // namespace astrodyn
