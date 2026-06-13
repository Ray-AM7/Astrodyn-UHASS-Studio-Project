#include "astrodyn/astro.hpp"
#include "astrodyn/csv.hpp"
#include "astrodyn/integrators.hpp"
#include <cmath>
#include <iostream>
#include <vector>

using namespace astrodyn;

int main() {
    const double r1_m = R_EARTH_M + 400e3;
    const double r2_m = R_EARTH_M + 1200e3;
    const double dt_s = 5.0;
    const double duration_s = hohmann_coplanar(MU_EARTH_M3_S2, r1_m, r2_m).transfer_time_s + 1200.0;
    const int steps = static_cast<int>(duration_s / dt_s);
    const int log_every = 3;

    HohmannResult h = hohmann_coplanar(MU_EARTH_M3_S2, r1_m, r2_m);
    State2D sc = circular_orbit_state(R_EARTH_M, r1_m - R_EARTH_M, MU_EARTH_M3_S2, 0.0, 500.0);

    AccelFunction earth_gravity = [](const State2D& s, double) {
        return two_body_accel(MU_EARTH_M3_S2, s.r_m);
    };

    std::vector<SampleRow> rows;
    bool did_burn1 = false;
    bool did_burn2 = false;

    for (int k = 0; k <= steps; ++k) {
        const double t_s = k * dt_s;

        if (!did_burn1) {
            apply_impulsive_burn(sc, h.dv1_m_s * prograde_unit(sc));
            did_burn1 = true;
            std::cout << "Burn 1 dv_m_s = " << h.dv1_m_s << "\n";
        }

        if (!did_burn2 && t_s >= h.transfer_time_s) {
            apply_impulsive_burn(sc, h.dv2_m_s * prograde_unit(sc));
            did_burn2 = true;
            std::cout << "Burn 2 dv_m_s = " << h.dv2_m_s << " at t_s = " << t_s << "\n";
        }

        if (k % log_every == 0) {
            rows.push_back(make_sample(t_s, "spacecraft", sc, MU_EARTH_M3_S2));
        }

        step_velocity_verlet(sc, t_s, dt_s, earth_gravity);
    }

    write_samples_csv("outputs/csv/hohmann_transfer.csv", rows);

    OrbitalElements2D final_el = elements_from_state(MU_EARTH_M3_S2, sc);
    std::cout << "Wrote outputs/csv/hohmann_transfer.csv\n";
    std::cout << "Target radius m: " << r2_m << "\n";
    std::cout << "Final a m: " << final_el.semi_major_axis_m << "\n";
    std::cout << "Final e: " << final_el.eccentricity << "\n";
    std::cout << "Total Hohmann dv m/s: " << h.total_dv_m_s << "\n";
}
