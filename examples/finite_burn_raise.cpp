#include "astrodyn/astro.hpp"
#include "astrodyn/csv.hpp"
#include "astrodyn/integrators.hpp"
#include <iostream>
#include <vector>

using namespace astrodyn;

int main() {
    const double altitude_m = 400e3;
    const double dt_s = 1.0;
    const double burn_duration_s = 180.0;
    const double coast_duration_s = 4200.0;
    const double duration_s = burn_duration_s + coast_duration_s;
    const int steps = static_cast<int>(duration_s / dt_s);
    const int log_every = 5;

    State2D sc = circular_orbit_state(R_EARTH_M, altitude_m, MU_EARTH_M3_S2, 0.0, 500.0);

    AccelFunction earth_gravity = [](const State2D& s, double) {
        return two_body_accel(MU_EARTH_M3_S2, s.r_m);
    };

    FiniteBurnCommand burn;
    burn.active = true;
    burn.thrust_N = 250.0;
    burn.isp_s = 230.0;
    burn.direction_mode = FiniteBurnCommand::DirectionMode::Prograde;

    std::vector<SampleRow> rows;

    for (int k = 0; k <= steps; ++k) {
        const double t_s = k * dt_s;
        burn.active = (t_s <= burn_duration_s && sc.m_kg > 0.0);

        if (k % log_every == 0) rows.push_back(make_sample(t_s, "finite_burn_sc", sc, MU_EARTH_M3_S2));
        step_rk4(sc, t_s, dt_s, earth_gravity, burn);
    }

    write_samples_csv("outputs/csv/finite_burn_raise.csv", rows);

    OrbitalElements2D el = elements_from_state(MU_EARTH_M3_S2, sc);
    std::cout << "Wrote outputs/csv/finite_burn_raise.csv\n";
    std::cout << "Final mass kg: " << sc.m_kg << "\n";
    std::cout << "Propellant used kg: " << 500.0 - sc.m_kg << "\n";
    std::cout << "Final a km: " << el.semi_major_axis_m / 1000.0 << "\n";
    std::cout << "Final e: " << el.eccentricity << "\n";
}
