#include "astrodyn/astro.hpp"
#include "astrodyn/csv.hpp"
#include "astrodyn/integrators.hpp"
#include <iostream>
#include <vector>

using namespace astrodyn;

int main() {
    const double altitude_m = 500e3;
    const double dt_s = 10.0;
    const double duration_s = 6.0 * orbital_period(MU_EARTH_M3_S2, R_EARTH_M + altitude_m);
    const int steps = static_cast<int>(duration_s / dt_s);
    const int log_every = 5;

    State2D euler = circular_orbit_state(R_EARTH_M, altitude_m, MU_EARTH_M3_S2);
    State2D symp = euler;
    State2D verlet = euler;
    State2D rk4 = euler;

    AccelFunction earth_gravity = [](const State2D& s, double) {
        return two_body_accel(MU_EARTH_M3_S2, s.r_m);
    };

    std::vector<SampleRow> rows;
    rows.reserve((steps / log_every + 1) * 4);

    for (int k = 0; k <= steps; ++k) {
        const double t_s = k * dt_s;
        if (k % log_every == 0) {
            rows.push_back(make_sample(t_s, "explicit_euler", euler, MU_EARTH_M3_S2));
            rows.push_back(make_sample(t_s, "symplectic_euler", symp, MU_EARTH_M3_S2));
            rows.push_back(make_sample(t_s, "velocity_verlet", verlet, MU_EARTH_M3_S2));
            rows.push_back(make_sample(t_s, "rk4", rk4, MU_EARTH_M3_S2));
        }

        step_explicit_euler(euler, t_s, dt_s, earth_gravity);
        step_symplectic_euler(symp, t_s, dt_s, earth_gravity);
        step_velocity_verlet(verlet, t_s, dt_s, earth_gravity);
        step_rk4(rk4, t_s, dt_s, earth_gravity);
    }

    write_samples_csv("outputs/csv/two_body_compare.csv", rows);
    std::cout << "Wrote outputs/csv/two_body_compare.csv\n";
}
