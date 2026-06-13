#include "astrodyn/astro.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace astrodyn {

static double clamp(double x, double lo, double hi) {
    return std::max(lo, std::min(hi, x));
}

double circular_speed(double mu_m3_s2, double radius_m) {
    return std::sqrt(mu_m3_s2 / radius_m);
}

double escape_speed(double mu_m3_s2, double radius_m) {
    return std::sqrt(2.0 * mu_m3_s2 / radius_m);
}

double orbital_period(double mu_m3_s2, double semi_major_axis_m) {
    return 2.0 * M_PI * std::sqrt((semi_major_axis_m * semi_major_axis_m * semi_major_axis_m) / mu_m3_s2);
}

double specific_energy(double mu_m3_s2, const State2D& state) {
    const double r = norm(state.r_m);
    const double v2 = norm2(state.v_m_s);
    return 0.5 * v2 - mu_m3_s2 / r;
}

double specific_angular_momentum_2d(const State2D& state) {
    return cross_z(state.r_m, state.v_m_s);
}

OrbitalElements2D elements_from_state(double mu_m3_s2, const State2D& state) {
    OrbitalElements2D e;
    const double r = norm(state.r_m);
    const double v2 = norm2(state.v_m_s);
    const double h = specific_angular_momentum_2d(state);
    const double energy = 0.5 * v2 - mu_m3_s2 / r;

    e.specific_energy_J_kg = energy;
    e.specific_angular_momentum_m2_s = h;

    if (std::abs(energy) > 1e-12) {
        e.semi_major_axis_m = -mu_m3_s2 / (2.0 * energy);
    } else {
        e.semi_major_axis_m = std::numeric_limits<double>::infinity();
    }

    // 2D eccentricity vector: e_vec = (v x h)/mu - r_hat.
    // With h = h_z k, v x h = h * (vy, -vx).
    Vec2 e_vec = Vec2{state.v_m_s.y * h, -state.v_m_s.x * h} / mu_m3_s2 - unit(state.r_m);
    e.eccentricity = norm(e_vec);

    if (e.eccentricity < 1.0 && e.semi_major_axis_m > 0.0) {
        e.periapsis_m = e.semi_major_axis_m * (1.0 - e.eccentricity);
        e.apoapsis_m = e.semi_major_axis_m * (1.0 + e.eccentricity);
    } else if (e.eccentricity >= 1.0 && e.semi_major_axis_m < 0.0) {
        e.periapsis_m = e.semi_major_axis_m * (1.0 - e.eccentricity);
        e.apoapsis_m = std::numeric_limits<double>::infinity();
    }

    if (e.eccentricity > 1e-10) {
        double cos_nu = clamp(dot(e_vec, state.r_m) / (e.eccentricity * r), -1.0, 1.0);
        double nu = std::acos(cos_nu);
        if (dot(state.r_m, state.v_m_s) < 0.0) nu = 2.0 * M_PI - nu;
        e.true_anomaly_rad = nu;
    } else {
        e.true_anomaly_rad = std::atan2(state.r_m.y, state.r_m.x);
    }

    return e;
}

State2D circular_orbit_state(double central_radius_m, double altitude_m, double mu_m3_s2, double phase_rad, double mass_kg) {
    const double r = central_radius_m + altitude_m;
    const Vec2 r_hat{std::cos(phase_rad), std::sin(phase_rad)};
    const Vec2 t_hat{-std::sin(phase_rad), std::cos(phase_rad)};
    return State2D{r_hat * r, t_hat * circular_speed(mu_m3_s2, r), mass_kg};
}

Vec2 two_body_accel(double mu_m3_s2, const Vec2& r_sc_m, const Vec2& r_central_m) {
    const Vec2 rel = r_sc_m - r_central_m;
    const double d = norm(rel);
    const double d3 = d * d * d;
    return (-mu_m3_s2 / d3) * rel;
}

Vec2 n_body_accel_on_test_particle(const Vec2& r_sc_m, const std::vector<Body2D>& grav_bodies) {
    Vec2 a{0.0, 0.0};
    for (const Body2D& body : grav_bodies) {
        a += two_body_accel(body.mu_m3_s2, r_sc_m, body.state.r_m);
    }
    return a;
}

HohmannResult hohmann_coplanar(double mu_m3_s2, double r1_m, double r2_m) {
    HohmannResult h;
    const double a_transfer = 0.5 * (r1_m + r2_m);
    const double v1_circ = std::sqrt(mu_m3_s2 / r1_m);
    const double v2_circ = std::sqrt(mu_m3_s2 / r2_m);
    const double v_peri_transfer = std::sqrt(mu_m3_s2 * (2.0 / r1_m - 1.0 / a_transfer));
    const double v_apo_transfer = std::sqrt(mu_m3_s2 * (2.0 / r2_m - 1.0 / a_transfer));
    h.dv1_m_s = v_peri_transfer - v1_circ;
    h.dv2_m_s = v2_circ - v_apo_transfer;
    h.transfer_time_s = M_PI * std::sqrt((a_transfer * a_transfer * a_transfer) / mu_m3_s2);
    h.total_dv_m_s = std::abs(h.dv1_m_s) + std::abs(h.dv2_m_s);
    return h;
}

void apply_impulsive_burn(State2D& state, const Vec2& delta_v_m_s) {
    state.v_m_s += delta_v_m_s;
}

Vec2 prograde_unit(const State2D& state) { return unit(state.v_m_s); }
Vec2 retrograde_unit(const State2D& state) { return -1.0 * unit(state.v_m_s); }
Vec2 radial_out_unit(const State2D& state) { return unit(state.r_m); }
Vec2 radial_in_unit(const State2D& state) { return -1.0 * unit(state.r_m); }

double rocket_final_mass(double m0_kg, double delta_v_m_s, double isp_s) {
    return m0_kg / std::exp(delta_v_m_s / (isp_s * G0_M_S2));
}

double rocket_delta_v(double m0_kg, double mf_kg, double isp_s) {
    return isp_s * G0_M_S2 * std::log(m0_kg / mf_kg);
}

SampleRow make_sample(double t_s, const std::string& object, const State2D& state, double central_mu_m3_s2) {
    SampleRow row;
    row.t_s = t_s;
    row.object = object;
    row.x_m = state.r_m.x;
    row.y_m = state.r_m.y;
    row.vx_m_s = state.v_m_s.x;
    row.vy_m_s = state.v_m_s.y;
    row.mass_kg = state.m_kg;
    row.radius_m = norm(state.r_m);
    row.speed_m_s = norm(state.v_m_s);
    if (central_mu_m3_s2 > 0.0) {
        row.energy_J_kg = specific_energy(central_mu_m3_s2, state);
        row.h_m2_s = specific_angular_momentum_2d(state);
    }
    return row;
}

} // namespace astrodyn
