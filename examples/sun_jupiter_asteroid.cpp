#include "astrodyn/astro.hpp"
#include "astrodyn/csv.hpp"
#include "astrodyn/integrators.hpp"
#include <cmath>
#include <iostream>
#include <vector>

using namespace astrodyn;

int main() {
    // Simple Sun-Jupiter-asteroid model.
    // Sun is fixed at origin, Jupiter and asteroid are propagated as bodies.
    // This is not ephemeris-grade; it is a starter long-term multibody dynamics example.
    const double dt_s = 3.0 * DAY_S;
    const double duration_s = 12.0 * 365.25 * DAY_S;
    const int steps = static_cast<int>(duration_s / dt_s);
    const int log_every = 2;

    Body2D sun{"Sun", MU_SUN_M3_S2, 696340e3, {{0,0},{0,0},0}, true};

    const double r_j_m = 5.2044 * AU_M;
    Body2D jupiter;
    jupiter.name = "Jupiter";
    jupiter.mu_m3_s2 = MU_JUPITER_M3_S2;
    jupiter.radius_m = 69911e3;
    jupiter.state.r_m = {r_j_m, 0.0};
    jupiter.state.v_m_s = {0.0, circular_speed(MU_SUN_M3_S2, r_j_m)};
    jupiter.fixed = false;

    // Start the asteroid near a 3:1 resonance-ish inner belt distance.
    const double r_a_m = 2.50 * AU_M;
    Body2D asteroid;
    asteroid.name = "Asteroid";
    asteroid.mu_m3_s2 = 0.0; // massless test body
    asteroid.radius_m = 0.0;
    asteroid.state.r_m = {r_a_m, 0.0};
    asteroid.state.v_m_s = {0.0, circular_speed(MU_SUN_M3_S2, r_a_m)};
    asteroid.fixed = false;

    std::vector<Body2D> bodies = {sun, jupiter, asteroid};
    std::vector<SampleRow> rows;

    for (int k = 0; k <= steps; ++k) {
        const double t_s = k * dt_s;
        if (k % log_every == 0) {
            rows.push_back(make_sample(t_s, "Jupiter", bodies[1].state, MU_SUN_M3_S2));
            rows.push_back(make_sample(t_s, "Asteroid", bodies[2].state, MU_SUN_M3_S2));
        }
        step_nbody_velocity_verlet(bodies, dt_s);
    }

    write_samples_csv("outputs/csv/sun_jupiter_asteroid.csv", rows);
    std::cout << "Wrote outputs/csv/sun_jupiter_asteroid.csv\n";
}
