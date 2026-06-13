#include "astrodyn/astro.hpp"
#include "astrodyn/body_catalog.hpp"
#include "astrodyn/csv.hpp"
#include "astrodyn/integrators.hpp"
#include "astrodyn/sandbox_world.hpp"

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

std::string integrator_name(IntegratorChoice integrator) {
    switch (integrator) {
        case IntegratorChoice::ExplicitEuler: return "Explicit Euler";
        case IntegratorChoice::SymplecticEuler: return "Symplectic Euler";
        case IntegratorChoice::VelocityVerlet: return "Velocity Verlet";
        case IntegratorChoice::RK4: return "RK4";
    }
    return "Unknown";
}


std::vector<Body2D> enabled_gravity_bodies(const SandboxWorld2D& world) {
    std::vector<Body2D> out;

    for (const auto& obj : world.objects) {
        if (!obj.gravity_source) continue;
        if (!obj.gravity_enabled) continue;

        Body2D b;
        b.name = obj.name;
        b.mu_m3_s2 = obj.mu_m3_s2;
        b.radius_m = obj.radius_m;
        b.state = obj.state;
        b.fixed = !obj.dynamic;

        out.push_back(b);
    }

    return out;
}

sf::Vector2f world_to_screen(
    const Vec2& r_m,
    const SandboxWorld2D& world,
    const sf::RenderWindow& window
) {
    const auto size = window.getSize();

    const double sx =
        static_cast<double>(size.x) * 0.5
        + (r_m.x - world.camera_center_m.x) / world.meters_per_pixel;

    const double sy =
        static_cast<double>(size.y) * 0.5
        - (r_m.y - world.camera_center_m.y) / world.meters_per_pixel;

    return sf::Vector2f(static_cast<float>(sx), static_cast<float>(sy));
}

void push_trail_point(std::vector<Vec2>& trail, const Vec2& r_m, std::size_t max_points = 8000) {
    trail.push_back(r_m);
    if (trail.size() > max_points) {
        trail.erase(trail.begin(), trail.begin() + static_cast<long>(trail.size() - max_points));
    }
}


void step_single_object(
    SimObject2D& obj,
    double t_s,
    double dt_s,
    const SandboxWorld2D& world,
    IntegratorChoice integrator,
    const FiniteBurnCommand& burn
) {
    State2D state = obj.state;

    const auto accel = [&](const State2D& s, double) {
        return n_body_accel_on_test_particle(s.r_m, enabled_gravity_bodies(world));
    };

    switch (integrator) {
        case IntegratorChoice::ExplicitEuler:
            if (burn.active) step_rk4(state, t_s, dt_s, accel, burn);
            else step_explicit_euler(state, t_s, dt_s, accel);
            break;

        case IntegratorChoice::SymplecticEuler:
            if (burn.active) step_rk4(state, t_s, dt_s, accel, burn);
            else step_symplectic_euler(state, t_s, dt_s, accel);
            break;

        case IntegratorChoice::VelocityVerlet:
            if (burn.active) step_rk4(state, t_s, dt_s, accel, burn);
            else step_velocity_verlet(state, t_s, dt_s, accel);
            break;

        case IntegratorChoice::RK4:
            step_rk4(state, t_s, dt_s, accel, burn);
            break;
    }

    obj.state = state;
}

void step_world(SandboxWorld2D& world) {
    for (int i = 0; i < static_cast<int>(world.objects.size()); ++i) {
        auto& obj = world.objects[i];

        if (!obj.dynamic) continue;

        FiniteBurnCommand burn;
        burn.active = false;

        if (i == world.active_vehicle_index && world.finite_burn_on && obj.state.m_kg > 1.0) {
            burn.active = true;
            burn.thrust_N = 250.0;
            burn.isp_s = 230.0;
            burn.direction_mode = FiniteBurnCommand::DirectionMode::Prograde;
        }

        step_single_object(obj, world.t_s, world.dt_s, world, world.integrator, burn);
        push_trail_point(obj.trail_m, obj.state.r_m);
    }

    world.t_s += world.dt_s;
}

std::string integrator_name(IntegratorChoice integrator) {
    switch (integrator) {
        case IntegratorChoice::ExplicitEuler: return "Explicit Euler";
        case IntegratorChoice::SymplecticEuler: return "Symplectic Euler";
        case IntegratorChoice::VelocityVerlet: return "Velocity Verlet";
        case IntegratorChoice::RK4: return "RK4";
    }
    return "Unknown";
}

void push_trail_point(std::vector<Vec2>& trail, const Vec2& r_m, std::size_t max_points = 8000) {
    trail.push_back(r_m);
    if (trail.size() > max_points) {
        trail.erase(trail.begin(), trail.begin() + static_cast<long>(trail.size() - max_points));
    }
}

void clear_all_trails(SandboxWorld2D& world) {
    for (auto& obj : world.objects) {
        obj.trail_m.clear();
    }
}

void select_next_object(SandboxWorld2D& world) {
    if (world.objects.empty()) {
        world.selected_index = -1;
        return;
    }

    world.selected_index = (world.selected_index + 1) % static_cast<int>(world.objects.size());

    for (int i = 0; i < static_cast<int>(world.objects.size()); ++i) {
        world.objects[i].selected = (i == world.selected_index);
    }
}

void select_previous_object(SandboxWorld2D& world) {
    if (world.objects.empty()) {
        world.selected_index = -1;
        return;
    }

    world.selected_index -= 1;
    if (world.selected_index < 0) {
        world.selected_index = static_cast<int>(world.objects.size()) - 1;
    }

    for (int i = 0; i < static_cast<int>(world.objects.size()); ++i) {
        world.objects[i].selected = (i == world.selected_index);
    }
}

SimObject2D* selected_object(SandboxWorld2D& world) {
    if (world.selected_index < 0) return nullptr;
    if (world.selected_index >= static_cast<int>(world.objects.size())) return nullptr;
    return &world.objects[world.selected_index];
}

const SimObject2D* selected_object_const(const SandboxWorld2D& world) {
    if (world.selected_index < 0) return nullptr;
    if (world.selected_index >= static_cast<int>(world.objects.size())) return nullptr;
    return &world.objects[world.selected_index];
}

void toggle_selected_gravity(SandboxWorld2D& world) {
    SimObject2D* obj = selected_object(world);
    if (!obj) return;
    if (!obj->gravity_source) return;

    obj->gravity_enabled = !obj->gravity_enabled;
    std::cout << obj->name << " gravity "
              << (obj->gravity_enabled ? "enabled" : "disabled") << "\n";
}

void toggle_selected_dynamic(SandboxWorld2D& world) {
    SimObject2D* obj = selected_object(world);
    if (!obj) return;

    obj->dynamic = !obj->dynamic;
    std::cout << obj->name << " dynamic "
              << (obj->dynamic ? "enabled" : "disabled/fixed") << "\n";
}

void set_selected_as_active_vehicle(SandboxWorld2D& world) {
    SimObject2D* obj = selected_object(world);
    if (!obj) return;

    world.active_vehicle_index = world.selected_index;
    obj->type = SimObjectType::Spacecraft;
    obj->gravity_source = false;
    obj->gravity_enabled = false;
    obj->dynamic = true;

    std::cout << obj->name << " is now active vehicle\n";
}

void add_earth_at_origin(SandboxWorld2D& world) {
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
    earth.draw_radius_px = 16.0f;

    world.objects.push_back(earth);
    world.selected_index = static_cast<int>(world.objects.size()) - 1;
    select_next_object(world);
}

void add_test_particle_near_selected(SandboxWorld2D& world) {
    const SimObject2D* parent = selected_object_const(world);
    if (!parent) {
        std::cout << "Select a parent body first\n";
        return;
    }

    SimObject2D particle;
    particle.name = "TestParticle";
    particle.type = SimObjectType::TestParticle;
    particle.mu_m3_s2 = 0.0;
    particle.radius_m = 0.0;
    particle.state.m_kg = 1.0;

    const double r_m = parent->radius_m + 500e3;
    const Vec2 rel_r{r_m, 0.0};
    const Vec2 rel_v{0.0, std::sqrt(parent->mu_m3_s2 / r_m)};

    particle.state.r_m = parent->state.r_m + rel_r;
    particle.state.v_m_s = parent->state.v_m_s + rel_v;

    particle.gravity_source = false;
    particle.gravity_enabled = false;
    particle.dynamic = true;
    particle.color_r = 100;
    particle.color_g = 255;
    particle.color_b = 160;
    particle.draw_radius_px = 5.0f;

    world.objects.push_back(particle);
    world.selected_index = static_cast<int>(world.objects.size()) - 1;
    world.active_vehicle_index = world.selected_index;
    select_next_object(world);
}

void remove_selected_object(SandboxWorld2D& world) {
    if (world.selected_index < 0) return;
    if (world.selected_index >= static_cast<int>(world.objects.size())) return;

    std::cout << "Removed " << world.objects[world.selected_index].name << "\n";

    world.objects.erase(world.objects.begin() + world.selected_index);

    if (world.objects.empty()) {
        world.selected_index = -1;
        world.active_vehicle_index = -1;
        return;
    }

    world.selected_index = std::min(world.selected_index, static_cast<int>(world.objects.size()) - 1);

    if (world.active_vehicle_index >= static_cast<int>(world.objects.size())) {
        world.active_vehicle_index = -1;
    }

    for (int i = 0; i < static_cast<int>(world.objects.size()); ++i) {
        world.objects[i].selected = (i == world.selected_index);
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
