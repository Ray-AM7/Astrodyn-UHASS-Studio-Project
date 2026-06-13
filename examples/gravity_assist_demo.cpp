#include "astrodyn/astro.hpp"
#include "astrodyn/csv.hpp"
#include "astrodyn/integrators.hpp"
#include <cmath>
#include <iostream>
#include <vector>

using namespace astrodyn;

int main() {
    // Patch-conic-style visual demo of an asteroid passing a moving Jupiter.
    // Sun is fixed. Jupiter follows an analytic circular orbit. Spacecraft is a test particle.
    const double dt_s = 1800.0;
    const double duration_s = 220.0 * DAY_S;
    const int steps = static_cast<int>(duration_s / dt_s);
    const int log_every = 8;

    const double r_j_m = 5.2044 * AU_M;
    const double n_j_rad_s = std::sqrt(MU_SUN_M3_S2 / (r_j_m * r_j_m * r_j_m));

    auto jupiter_state = [&](double t_s) -> State2D {
        const double th = n_j_rad_s * t_s;
        return {{r_j_m * std::cos(th), r_j_m * std::sin(th)},
                {-r_j_m * n_j_rad_s * std::sin(th), r_j_m * n_j_rad_s * std::cos(th)},
                0.0};
    };

    // Start near Jupiter's orbit, offset so Jupiter bends the path.
    State2D sc;
    sc.r_m = {r_j_m - 0.45 * AU_M, -0.10 * AU_M};
    sc.v_m_s = {4500.0, circular_speed(MU_SUN_M3_S2, r_j_m) * 0.92};

    AccelFunction accel = [&](const State2D& s, double t_s) {
        Vec2 a = two_body_accel(MU_SUN_M3_S2, s.r_m);
        a += two_body_accel(MU_JUPITER_M3_S2, s.r_m, jupiter_state(t_s).r_m);
        return a;
    };

    std::vector<SampleRow> rows;
    for (int k = 0; k <= steps; ++k) {
        const double t_s = k * dt_s;
        if (k % log_every == 0) {
            rows.push_back(make_sample(t_s, "spacecraft", sc, MU_SUN_M3_S2));
            rows.push_back(make_sample(t_s, "Jupiter", jupiter_state(t_s), MU_SUN_M3_S2));
        }
        step_rk4(sc, t_s, dt_s, accel);
    }

    write_samples_csv("outputs/csv/gravity_assist_demo.csv", rows);
    std::cout << "Wrote outputs/csv/gravity_assist_demo.csv\n";
}
