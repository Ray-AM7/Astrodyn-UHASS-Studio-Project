#include "astrodyn/astro.hpp"
#include "astrodyn/csv.hpp"
#include "astrodyn/integrators.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace astrodyn;

namespace {

enum class IntegratorChoice {
    ExplicitEuler,
    SymplecticEuler,
    VelocityVerlet,
    RK4
};

struct ActiveBody {
    Body2D body;
    bool gravity_enabled = true;
    sf::Color color = sf::Color::White;
    float draw_radius_px = 8.0f;
};

struct SandboxScenario {
    std::string name;
    std::vector<ActiveBody> bodies;
    State2D spacecraft;
    double dt_s = 10.0;
    double max_time_s = 2.0 * 3600.0;
    double meters_per_pixel = 20000.0;
    Vec2 camera_center_m{0.0, 0.0};
    double default_energy_mu_m3_s2 = MU_EARTH_M3_S2;
};

std::string integrator_name(IntegratorChoice integrator) {
    switch (integrator) {
        case IntegratorChoice::ExplicitEuler: return "Explicit Euler";
        case IntegratorChoice::SymplecticEuler: return "Symplectic Euler";
        case IntegratorChoice::VelocityVerlet: return "Velocity Verlet";
        case IntegratorChoice::RK4: return "RK4";
    }
    return "Unknown";
}

std::vector<Body2D> enabled_gravity_bodies(const std::vector<ActiveBody>& active_bodies) {
    std::vector<Body2D> out;
    for (const auto& active : active_bodies) {
        if (active.gravity_enabled) out.push_back(active.body);
    }
    return out;
}

sf::Vector2f world_to_screen(const Vec2& r_m, const SandboxScenario& scenario, const sf::RenderWindow& window) {
    const auto size = window.getSize();
    const double sx = static_cast<double>(size.x) * 0.5 + (r_m.x - scenario.camera_center_m.x) / scenario.meters_per_pixel;
    const double sy = static_cast<double>(size.y) * 0.5 - (r_m.y - scenario.camera_center_m.y) / scenario.meters_per_pixel;
    return sf::Vector2f(static_cast<float>(sx), static_cast<float>(sy));
}

void push_trail_point(std::vector<Vec2>& trail, const Vec2& r_m, std::size_t max_points = 8000) {
    trail.push_back(r_m);
    if (trail.size() > max_points) {
        trail.erase(trail.begin(), trail.begin() + static_cast<long>(trail.size() - max_points));
    }
}

SandboxScenario make_earth_leo() {
    SandboxScenario s;
    s.name = "Earth LEO - single spacecraft";
    s.dt_s = 10.0;
    s.max_time_s = 6.0 * 5400.0;
    s.meters_per_pixel = 18000.0;
    s.default_energy_mu_m3_s2 = MU_EARTH_M3_S2;

    ActiveBody earth;
    earth.body.name = "Earth";
    earth.body.mu_m3_s2 = MU_EARTH_M3_S2;
    earth.body.radius_m = R_EARTH_M;
    earth.body.state.r_m = {0.0, 0.0};
    earth.body.state.v_m_s = {0.0, 0.0};
    earth.body.fixed = true;
    earth.color = sf::Color(70, 120, 255);
    earth.draw_radius_px = 18.0f;
    s.bodies.push_back(earth);

    s.spacecraft = circular_orbit_state(R_EARTH_M, 500e3, MU_EARTH_M3_S2, 0.0, 500.0);
    return s;
}

SandboxScenario make_earth_moon() {
    SandboxScenario s;
    s.name = "Earth + Moon perturbation";
    s.dt_s = 60.0;
    s.max_time_s = 10.0 * DAY_S;
    s.meters_per_pixel = 1.4e6;
    s.default_energy_mu_m3_s2 = MU_EARTH_M3_S2;

    ActiveBody earth;
    earth.body.name = "Earth";
    earth.body.mu_m3_s2 = MU_EARTH_M3_S2;
    earth.body.radius_m = R_EARTH_M;
    earth.body.fixed = true;
    earth.color = sf::Color(70, 120, 255);
    earth.draw_radius_px = 13.0f;
    s.bodies.push_back(earth);

    const double moon_r_m = 384400e3;
    ActiveBody moon;
    moon.body.name = "Moon";
    moon.body.mu_m3_s2 = 4.9048695e12;
    moon.body.radius_m = 1737.4e3;
    moon.body.state.r_m = {moon_r_m, 0.0};
    moon.body.state.v_m_s = {0.0, std::sqrt(MU_EARTH_M3_S2 / moon_r_m)};
    moon.body.fixed = false;
    moon.color = sf::Color(190, 190, 190);
    moon.draw_radius_px = 7.0f;
    s.bodies.push_back(moon);

    s.spacecraft = circular_orbit_state(R_EARTH_M, 250000e3, MU_EARTH_M3_S2, 0.3, 500.0);
    return s;
}

SandboxScenario make_sun_jupiter_asteroid() {
    SandboxScenario s;
    s.name = "Sun + Jupiter + asteroid";
    s.dt_s = 0.5 * DAY_S;
    s.max_time_s = 12.0 * 365.25 * DAY_S;
    s.meters_per_pixel = 2.0e9;
    s.default_energy_mu_m3_s2 = MU_SUN_M3_S2;

    ActiveBody sun;
    sun.body.name = "Sun";
    sun.body.mu_m3_s2 = MU_SUN_M3_S2;
    sun.body.radius_m = 696340e3;
    sun.body.fixed = true;
    sun.color = sf::Color(255, 210, 60);
    sun.draw_radius_px = 16.0f;
    s.bodies.push_back(sun);

    const double jupiter_a_m = 5.2 * AU_M;
    ActiveBody jupiter;
    jupiter.body.name = "Jupiter";
    jupiter.body.mu_m3_s2 = MU_JUPITER_M3_S2;
    jupiter.body.radius_m = 69911e3;
    jupiter.body.state.r_m = {jupiter_a_m, 0.0};
    jupiter.body.state.v_m_s = {0.0, std::sqrt(MU_SUN_M3_S2 / jupiter_a_m)};
    jupiter.body.fixed = false;
    jupiter.color = sf::Color(220, 160, 100);
    jupiter.draw_radius_px = 8.0f;
    s.bodies.push_back(jupiter);

    const double asteroid_a_m = 3.25 * AU_M;
    s.spacecraft.r_m = {asteroid_a_m, 0.0};
    s.spacecraft.v_m_s = {0.0, std::sqrt(MU_SUN_M3_S2 / asteroid_a_m) * 0.985};
    s.spacecraft.m_kg = 1.0;
    return s;
}

SandboxScenario make_gravity_assist() {
    SandboxScenario s;
    s.name = "Moving planet flyby";
    s.dt_s = 120.0;
    s.max_time_s = 14.0 * DAY_S;
    s.meters_per_pixel = 2.2e7;
    s.default_energy_mu_m3_s2 = MU_EARTH_M3_S2;

    ActiveBody earth;
    earth.body.name = "Earth";
    earth.body.mu_m3_s2 = MU_EARTH_M3_S2;
    earth.body.radius_m = R_EARTH_M;
    earth.body.fixed = false;
    earth.body.state.r_m = {0.0, 0.0};
    earth.body.state.v_m_s = {0.0, 0.0};
    earth.color = sf::Color(70, 120, 255);
    earth.draw_radius_px = 13.0f;
    s.bodies.push_back(earth);

    s.spacecraft.r_m = {-70.0 * R_EARTH_M, -12.0 * R_EARTH_M};
    s.spacecraft.v_m_s = {10500.0, 1700.0};
    s.spacecraft.m_kg = 500.0;
    return s;
}

SandboxScenario make_finite_burn_raise() {
    SandboxScenario s = make_earth_leo();
    s.name = "Earth LEO with manual finite/prograde burns";
    s.dt_s = 2.0;
    s.max_time_s = 3.0 * 5400.0;
    s.spacecraft.m_kg = 600.0;
    return s;
}

void step_spacecraft(
    State2D& spacecraft,
    double t_s,
    double dt_s,
    const std::vector<ActiveBody>& active_bodies,
    IntegratorChoice integrator,
    const FiniteBurnCommand& burn
) {
    const auto accel = [&](const State2D& sc, double) {
        return n_body_accel_on_test_particle(sc.r_m, enabled_gravity_bodies(active_bodies));
    };

    switch (integrator) {
        case IntegratorChoice::ExplicitEuler:
            if (burn.active) step_rk4(spacecraft, t_s, dt_s, accel, burn);
            else step_explicit_euler(spacecraft, t_s, dt_s, accel);
            break;
        case IntegratorChoice::SymplecticEuler:
            if (burn.active) step_rk4(spacecraft, t_s, dt_s, accel, burn);
            else step_symplectic_euler(spacecraft, t_s, dt_s, accel);
            break;
        case IntegratorChoice::VelocityVerlet:
            if (burn.active) step_rk4(spacecraft, t_s, dt_s, accel, burn);
            else step_velocity_verlet(spacecraft, t_s, dt_s, accel);
            break;
        case IntegratorChoice::RK4:
            step_rk4(spacecraft, t_s, dt_s, accel, burn);
            break;
    }
}

void export_csv(const std::vector<SampleRow>& rows) {
    const std::string path = "outputs/csv/orbit_sandbox_export.csv";
    write_samples_csv(path, rows);
    std::cout << "Exported " << rows.size() << " rows to " << path << "\n";
}

std::string body_status_string(const std::vector<ActiveBody>& bodies) {
    std::ostringstream ss;
    for (const auto& active : bodies) {
        ss << active.body.name << ':' << (active.gravity_enabled ? "on" : "off") << ' ';
    }
    return ss.str();
}

} // namespace

int main() {
    std::vector<SandboxScenario> scenarios;
    scenarios.push_back(make_earth_leo());
    scenarios.push_back(make_earth_moon());
    scenarios.push_back(make_sun_jupiter_asteroid());
    scenarios.push_back(make_gravity_assist());
    scenarios.push_back(make_finite_burn_raise());

    int scenario_index = 0;
    SandboxScenario scenario = scenarios[scenario_index];
    IntegratorChoice integrator = IntegratorChoice::VelocityVerlet;

    bool paused = false;
    bool finite_burn_on = false;
    int steps_per_frame = 4;
    double t_s = 0.0;

    std::vector<Vec2> spacecraft_trail;
    std::vector<std::vector<Vec2>> body_trails(scenario.bodies.size());
    std::vector<SampleRow> log_rows;

    sf::RenderWindow window(sf::VideoMode(1200, 850), "Astrodyn Orbit Sandbox");
    window.setFramerateLimit(60);

    auto reset = [&]() {
        scenario = scenarios[scenario_index];
        t_s = 0.0;
        finite_burn_on = false;
        spacecraft_trail.clear();
        body_trails.assign(scenario.bodies.size(), {});
        log_rows.clear();
        std::cout << "\nLoaded scenario " << (scenario_index + 1) << ": " << scenario.name << "\n";
    };

    reset();

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::KeyPressed) {
                const auto key = event.key.code;
                if (key == sf::Keyboard::Escape) window.close();
                if (key == sf::Keyboard::Space) paused = !paused;
                if (key == sf::Keyboard::R) reset();
                if (key == sf::Keyboard::C) {
                    spacecraft_trail.clear();
                    body_trails.assign(scenario.bodies.size(), {});
                }
                if (key == sf::Keyboard::X) export_csv(log_rows);

                if (key == sf::Keyboard::Num1) { scenario_index = 0; reset(); }
                if (key == sf::Keyboard::Num2) { scenario_index = 1; reset(); }
                if (key == sf::Keyboard::Num3) { scenario_index = 2; reset(); }
                if (key == sf::Keyboard::Num4) { scenario_index = 3; reset(); }
                if (key == sf::Keyboard::Num5) { scenario_index = 4; reset(); }

                if (key == sf::Keyboard::F1) integrator = IntegratorChoice::ExplicitEuler;
                if (key == sf::Keyboard::F2) integrator = IntegratorChoice::SymplecticEuler;
                if (key == sf::Keyboard::F3) integrator = IntegratorChoice::VelocityVerlet;
                if (key == sf::Keyboard::F4) integrator = IntegratorChoice::RK4;

                if (key == sf::Keyboard::Add || key == sf::Keyboard::Equal) steps_per_frame = std::min(200, steps_per_frame + 1);
                if (key == sf::Keyboard::Subtract || key == sf::Keyboard::Hyphen) steps_per_frame = std::max(1, steps_per_frame - 1);
                if (key == sf::Keyboard::Up) scenario.meters_per_pixel *= 0.85;
                if (key == sf::Keyboard::Down) scenario.meters_per_pixel *= 1.15;
                if (key == sf::Keyboard::Left) scenario.camera_center_m.x -= 50.0 * scenario.meters_per_pixel;
                if (key == sf::Keyboard::Right) scenario.camera_center_m.x += 50.0 * scenario.meters_per_pixel;
                if (key == sf::Keyboard::PageUp) scenario.camera_center_m.y += 50.0 * scenario.meters_per_pixel;
                if (key == sf::Keyboard::PageDown) scenario.camera_center_m.y -= 50.0 * scenario.meters_per_pixel;

                if (key == sf::Keyboard::B) {
                    apply_impulsive_burn(scenario.spacecraft, 50.0 * prograde_unit(scenario.spacecraft));
                    std::cout << "Applied +50 m/s prograde impulse\n";
                }
                if (key == sf::Keyboard::N) {
                    apply_impulsive_burn(scenario.spacecraft, 50.0 * retrograde_unit(scenario.spacecraft));
                    std::cout << "Applied -50 m/s retrograde impulse\n";
                }
                if (key == sf::Keyboard::T) finite_burn_on = !finite_burn_on;

                if (key == sf::Keyboard::E && !scenario.bodies.empty()) scenario.bodies[0].gravity_enabled = !scenario.bodies[0].gravity_enabled;
                if (key == sf::Keyboard::M && scenario.bodies.size() > 1) scenario.bodies[1].gravity_enabled = !scenario.bodies[1].gravity_enabled;
                if (key == sf::Keyboard::J && scenario.bodies.size() > 1) scenario.bodies[1].gravity_enabled = !scenario.bodies[1].gravity_enabled;
            }
        }

        if (!paused && t_s <= scenario.max_time_s) {
            for (int i = 0; i < steps_per_frame; ++i) {
                // Move massive bodies first for scenarios where they are not fixed.
                std::vector<Body2D> body_copy;
                for (const auto& active : scenario.bodies) body_copy.push_back(active.body);
                if (body_copy.size() > 1) {
                    step_nbody_velocity_verlet(body_copy, scenario.dt_s);
                    for (std::size_t b = 0; b < body_copy.size(); ++b) {
                        scenario.bodies[b].body.state = body_copy[b].state;
                    }
                }

                FiniteBurnCommand burn;
                burn.active = finite_burn_on && scenario.spacecraft.m_kg > 1.0;
                burn.thrust_N = 250.0;
                burn.isp_s = 230.0;
                burn.direction_mode = FiniteBurnCommand::DirectionMode::Prograde;

                step_spacecraft(scenario.spacecraft, t_s, scenario.dt_s, scenario.bodies, integrator, burn);
                t_s += scenario.dt_s;

                push_trail_point(spacecraft_trail, scenario.spacecraft.r_m);
                for (std::size_t b = 0; b < scenario.bodies.size(); ++b) {
                    push_trail_point(body_trails[b], scenario.bodies[b].body.state.r_m, 3000);
                }

                if (static_cast<int>(log_rows.size()) % 1 == 0) {
                    log_rows.push_back(make_sample(t_s, "spacecraft", scenario.spacecraft, scenario.default_energy_mu_m3_s2));
                }
            }
        }

        std::ostringstream title;
        title << "Astrodyn Orbit Sandbox | " << scenario.name
              << " | " << integrator_name(integrator)
              << " | t=" << std::fixed << std::setprecision(2) << (t_s / DAY_S) << " d"
              << " | steps/frame=" << steps_per_frame
              << " | burn=" << (finite_burn_on ? "ON" : "off")
              << " | " << body_status_string(scenario.bodies)
              << " | keys: 1-5 scenarios, F1-F4 method, Space pause, B/N impulse, T thrust, X export";
        window.setTitle(title.str());

        window.clear(sf::Color(8, 10, 18));

        // Draw trails.
        if (spacecraft_trail.size() > 1) {
            sf::VertexArray trail(sf::LineStrip, spacecraft_trail.size());
            for (std::size_t i = 0; i < spacecraft_trail.size(); ++i) {
                trail[i].position = world_to_screen(spacecraft_trail[i], scenario, window);
                trail[i].color = sf::Color(80, 255, 150, 160);
            }
            window.draw(trail);
        }

        for (std::size_t b = 0; b < body_trails.size(); ++b) {
            if (body_trails[b].size() > 1) {
                sf::VertexArray trail(sf::LineStrip, body_trails[b].size());
                for (std::size_t i = 0; i < body_trails[b].size(); ++i) {
                    trail[i].position = world_to_screen(body_trails[b][i], scenario, window);
                    trail[i].color = sf::Color(scenario.bodies[b].color.r, scenario.bodies[b].color.g, scenario.bodies[b].color.b, 90);
                }
                window.draw(trail);
            }
        }

        // Draw bodies.
        for (const auto& active : scenario.bodies) {
            const auto p = world_to_screen(active.body.state.r_m, scenario, window);
            sf::CircleShape c(active.draw_radius_px);
            c.setOrigin(active.draw_radius_px, active.draw_radius_px);
            c.setPosition(p);
            c.setFillColor(active.gravity_enabled ? active.color : sf::Color(80, 80, 80));
            window.draw(c);
        }

        // Draw spacecraft.
        const auto scp = world_to_screen(scenario.spacecraft.r_m, scenario, window);
        sf::CircleShape sc(5.0f);
        sc.setOrigin(5.0f, 5.0f);
        sc.setPosition(scp);
        sc.setFillColor(sf::Color(100, 255, 160));
        window.draw(sc);

        // Draw velocity vector.
        Vec2 v_display = scenario.spacecraft.v_m_s * (1500.0 * scenario.dt_s);
        sf::VertexArray vel(sf::Lines, 2);
        vel[0].position = scp;
        vel[0].color = sf::Color(255, 255, 255, 150);
        vel[1].position = world_to_screen(scenario.spacecraft.r_m + v_display, scenario, window);
        vel[1].color = sf::Color(255, 255, 255, 150);
        window.draw(vel);

        window.display();
    }

    return 0;
}
