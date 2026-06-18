#include "astrodyn/Presets.hpp"

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

using namespace astrodyn;

namespace {

struct Star {
    Vec2 worldPosition_m;
    float brightness = 1.0f;
};

struct AppState {
    Universe universe;
    PropagationMethod method = PropagationMethod::VelocityVerlet;

    double dt_s = 60.0;
    int stepsPerFrame = 1;

    double metersPerPixel = 1.0e7;
    Vec2 cameraCenter_m{0.0, 0.0};

    bool showLabels = true;
    bool showTrails = true;
    bool showStars = true;
    bool showOrigin = true;
    bool drawTrueSizes = true;

    double newObjectOrbitRadius_m = 1.0e7;
    double impulsiveBurnDeltaV_m_s = 10.0;
    double continuousBurnAccel_m_s2 = 0.001;

    bool isPanning = false;
    sf::Vector2i lastMousePos;

    std::vector<Star> stars;
};

std::vector<Star> makeStars(int count, double halfExtent_m) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> pos(-halfExtent_m, halfExtent_m);
    std::uniform_real_distribution<float> bright(0.25f, 1.0f);

    std::vector<Star> stars;
    stars.reserve(count);

    for (int i = 0; i < count; ++i) {
        stars.push_back({Vec2{pos(rng), pos(rng)}, bright(rng)});
    }

    return stars;
}

sf::Vector2f worldToScreen(const Vec2& r, const AppState& app, const sf::RenderWindow& window) {
    const auto size = window.getSize();
    const Vec2 origin = app.universe.barycenterPosition();

    const Vec2 displayR = r - origin;

    const double sx = static_cast<double>(size.x) * 0.5
        + (displayR.x - app.cameraCenter_m.x) / app.metersPerPixel;

    const double sy = static_cast<double>(size.y) * 0.5
        - (displayR.y - app.cameraCenter_m.y) / app.metersPerPixel;

    return sf::Vector2f(static_cast<float>(sx), static_cast<float>(sy));
}

Vec2 screenToWorld(const sf::Vector2i& p, const AppState& app, const sf::RenderWindow& window) {
    const auto size = window.getSize();
    const Vec2 origin = app.universe.barycenterPosition();

    const double x = origin.x + app.cameraCenter_m.x
        + (static_cast<double>(p.x) - static_cast<double>(size.x) * 0.5) * app.metersPerPixel;

    const double y = origin.y + app.cameraCenter_m.y
        - (static_cast<double>(p.y) - static_cast<double>(size.y) * 0.5) * app.metersPerPixel;

    return {x, y};
}

std::string sci(double v) {
    std::ostringstream s;
    s << std::scientific << std::setprecision(4) << v;
    return s.str();
}

int nearestObjectIndex(const AppState& app, const sf::Vector2i& mouse, const sf::RenderWindow& window) {
    int bestIndex = -1;
    double bestDistPx = 30.0;

    for (int i = 0; i < static_cast<int>(app.universe.objects().size()); ++i) {
        const auto& obj = *app.universe.objects()[i];

        const sf::Vector2f p = worldToScreen(obj.position(), app, window);
        const double dx = static_cast<double>(mouse.x) - p.x;
        const double dy = static_cast<double>(mouse.y) - p.y;
        const double d = std::sqrt(dx * dx + dy * dy);

        // Labels are always clickable even if the true-size body is tiny.
        if (d < bestDistPx) {
            bestDistPx = d;
            bestIndex = i;
        }
    }

    return bestIndex;
}

void drawOrigin(sf::RenderWindow& window, const AppState& app) {
    const sf::Vector2f o = worldToScreen(app.universe.barycenterPosition(), app, window);

    sf::VertexArray cross(sf::Lines, 4);
    cross[0].position = {o.x - 12.0f, o.y};
    cross[1].position = {o.x + 12.0f, o.y};
    cross[2].position = {o.x, o.y - 12.0f};
    cross[3].position = {o.x, o.y + 12.0f};

    for (int i = 0; i < 4; ++i) {
        cross[i].color = sf::Color(255, 255, 255, 180);
    }

    window.draw(cross);

    sf::CircleShape marker(3.0f);
    marker.setOrigin(3.0f, 3.0f);
    marker.setPosition(o);
    marker.setFillColor(sf::Color(255, 255, 255));
    window.draw(marker);
}

void drawStars(sf::RenderWindow& window, const AppState& app) {
    if (!app.showStars) return;

    sf::VertexArray points(sf::Points, app.stars.size());

    for (std::size_t i = 0; i < app.stars.size(); ++i) {
        const auto p = worldToScreen(app.stars[i].worldPosition_m, app, window);

        sf::Uint8 b = static_cast<sf::Uint8>(90 + 165 * app.stars[i].brightness);
        points[i].position = p;
        points[i].color = sf::Color(b, b, b, 180);
    }

    window.draw(points);
}

void drawTrails(sf::RenderWindow& window, const AppState& app) {
    if (!app.showTrails) return;

    for (const auto& objPtr : app.universe.objects()) {
        const SpaceObject& obj = *objPtr;
        const auto& trail = obj.trail();

        if (trail.size() < 2 || !obj.isTrailEnabled()) continue;

        sf::VertexArray line(sf::LineStrip, trail.size());

        for (std::size_t i = 0; i < trail.size(); ++i) {
            line[i].position = worldToScreen(trail[i], app, window);
            line[i].color = sf::Color(
                static_cast<sf::Uint8>(obj.colorR()),
                static_cast<sf::Uint8>(obj.colorG()),
                static_cast<sf::Uint8>(obj.colorB()),
                100
            );
        }

        window.draw(line);
    }
}

void drawObjects(sf::RenderWindow& window, const AppState& app) {
    for (const auto& objPtr : app.universe.objects()) {
        const SpaceObject& obj = *objPtr;

        const sf::Vector2f p = worldToScreen(obj.position(), app, window);

        float radiusPx = app.drawTrueSizes
            ? static_cast<float>(obj.radiusM() / app.metersPerPixel)
            : obj.drawRadiusPx();

        // True-size mode means objects can disappear when zoomed out.
        // Still draw at least a 1px dot for selected objects so you don't lose them.
        if (obj.isSelected()) radiusPx = std::max(radiusPx, 3.0f);

        if (radiusPx >= 0.5f) {
            sf::CircleShape shape(radiusPx);
            shape.setOrigin(radiusPx, radiusPx);
            shape.setPosition(p);

            sf::Color color(
                static_cast<sf::Uint8>(obj.colorR()),
                static_cast<sf::Uint8>(obj.colorG()),
                static_cast<sf::Uint8>(obj.colorB())
            );

            if (!obj.isGravityEnabled() && obj.kind() != SpaceObjectKind::SpaceCraft && obj.kind() != SpaceObjectKind::TestParticle) {
                color = sf::Color(100, 100, 100);
            }

            shape.setFillColor(color);

            if (obj.isSelected()) {
                shape.setOutlineColor(sf::Color::White);
                shape.setOutlineThickness(2.0f);
            }

            window.draw(shape);
        } else if (obj.isSelected()) {
            sf::CircleShape dot(3.0f);
            dot.setOrigin(3.0f, 3.0f);
            dot.setPosition(p);
            dot.setFillColor(sf::Color::White);
            window.draw(dot);
        }
    }
}

void drawLabels(sf::RenderWindow& window, const AppState& app, sf::Font& font) {
    if (!app.showLabels) return;

    for (const auto& objPtr : app.universe.objects()) {
        const SpaceObject& obj = *objPtr;
        const sf::Vector2f p = worldToScreen(obj.position(), app, window);

        sf::Text text;
        text.setFont(font);
        text.setCharacterSize(obj.isSelected() ? 15 : 12);
        text.setString(obj.name());
        text.setPosition(p.x + 8.0f, p.y - 8.0f);
        text.setFillColor(obj.isSelected() ? sf::Color::White : sf::Color(220, 220, 230, 210));

        window.draw(text);
    }
}

void drawCanvas(sf::RenderWindow& window, AppState& app, sf::Font& font) {
    window.clear(sf::Color(5, 7, 14));

    drawStars(window, app);
    if (app.showOrigin) drawOrigin(window, app);
    drawTrails(window, app);
    drawObjects(window, app);
    drawLabels(window, app, font);
}

void editSelectedObjectPanel(AppState& app) {
    SpaceObject* obj = app.universe.selected();

    ImGui::Begin("Selected Object");

    if (!obj) {
        ImGui::Text("No object selected.");
        ImGui::Text("Left-click a label/object or add an object.");
        ImGui::End();
        return;
    }

    ImGui::Text("Name: %s", obj->name().c_str());
    ImGui::Text("Type: %s", objectKindName(obj->kind()).c_str());

    bool gravity = obj->isGravityEnabled();
    bool affected = obj->isAffectedByGravity();
    bool dynamic = obj->isDynamic();
    bool collision = obj->isCollisionEnabled();
    bool trail = obj->isTrailEnabled();

    if (ImGui::Checkbox("Gravity Source", &gravity)) obj->setGravityEnabled(gravity);
    if (ImGui::Checkbox("Affected By Gravity", &affected)) obj->setAffectedByGravity(affected);
    if (ImGui::Checkbox("Dynamic / Propagated", &dynamic)) obj->setDynamic(dynamic);
    if (ImGui::Checkbox("Collisions Enabled", &collision)) obj->setCollisionEnabled(collision);
    if (ImGui::Checkbox("Trail Enabled", &trail)) obj->setTrailEnabled(trail);

    ImGui::Separator();

    double x = obj->position().x;
    double y = obj->position().y;
    double vx = obj->velocity().x;
    double vy = obj->velocity().y;
    double mass = obj->massKg();
    double radius = obj->radiusM();

    Vec2 momentum = obj->momentum();
    double px = momentum.x;
    double py = momentum.y;

    float drawRadius = obj->drawRadiusPx();

    if (ImGui::InputDouble("x position (m)", &x, 0.0, 0.0, "%.6e")) obj->editObject("x", x);
    if (ImGui::InputDouble("y position (m)", &y, 0.0, 0.0, "%.6e")) obj->editObject("y", y);
    if (ImGui::InputDouble("vx velocity (m/s)", &vx, 0.0, 0.0, "%.6e")) obj->editObject("vx", vx);
    if (ImGui::InputDouble("vy velocity (m/s)", &vy, 0.0, 0.0, "%.6e")) obj->editObject("vy", vy);
    if (ImGui::InputDouble("px momentum", &px, 0.0, 0.0, "%.6e")) obj->editObject("px", px);
    if (ImGui::InputDouble("py momentum", &py, 0.0, 0.0, "%.6e")) obj->editObject("py", py);
    if (ImGui::InputDouble("mass (kg)", &mass, 0.0, 0.0, "%.6e")) obj->editObject("mass", mass);
    if (ImGui::InputDouble("physical radius (m)", &radius, 0.0, 0.0, "%.6e")) obj->editObject("radius", radius);
    if (ImGui::InputFloat("fallback draw radius (px)", &drawRadius)) obj->editObject("drawRadius", drawRadius);

    ImGui::Separator();
    ImGui::Text("Burn Controls");

    ImGui::InputDouble("Impulse delta-v (m/s)", &app.impulsiveBurnDeltaV_m_s, 0.0, 0.0, "%.6e");
    app.impulsiveBurnDeltaV_m_s = std::max(0.0, app.impulsiveBurnDeltaV_m_s);

    if (ImGui::Button("Prograde Impulse")) {
        app.universe.applyImpulsiveBurnToSelected(app.impulsiveBurnDeltaV_m_s, true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Retrograde Impulse")) {
        app.universe.applyImpulsiveBurnToSelected(app.impulsiveBurnDeltaV_m_s, false);
    }

    bool contBurn = obj->continuousBurnEnabled();
    if (ImGui::Checkbox("Continuous Burn Enabled", &contBurn)) {
        obj->setContinuousBurnEnabled(contBurn);
    }

    double burnAccel = obj->continuousBurnAccelerationMps2();
    if (ImGui::InputDouble("Continuous burn accel (m/s^2)", &burnAccel, 0.0, 0.0, "%.6e")) {
        obj->editObject("continuousBurnAcceleration", burnAccel);
    }

    const int idx = app.universe.selectedIndex();

    //velocity stuff
    if (idx >= 0) {
        ImGui::Separator();
        ImGui::Text("Relative Kinematics");

        const auto bary = app.universe.relativeToBarycenter(idx);
        ImGui::Text("Relative to barycenter:");
        ImGui::Text("  v = [%.6e, %.6e] m/s", bary.velocityVector_m_s.x, bary.velocityVector_m_s.y);
        ImGui::Text("  speed = %.6e m/s", bary.speed_m_s);
        ImGui::Text("  tangential speed = %.6e m/s", bary.tangentialSpeed_m_s);
        ImGui::Text("  tangential v = [%.6e, %.6e] m/s",
            bary.tangentialVelocityVector_m_s.x,
            bary.tangentialVelocityVector_m_s.y
        );

        const int strongest = app.universe.strongestGravitySourceIndexFor(idx);
        if (strongest >= 0) {
            const auto rel = app.universe.relativeToObject(idx, strongest);
            ImGui::Text("Relative to strongest gravity source: %s",
                app.universe.objects()[strongest]->name().c_str()
            );
            ImGui::Text("  v = [%.6e, %.6e] m/s", rel.velocityVector_m_s.x, rel.velocityVector_m_s.y);
            ImGui::Text("  speed = %.6e m/s", rel.speed_m_s);
            ImGui::Text("  tangential speed = %.6e m/s", rel.tangentialSpeed_m_s);
            ImGui::Text("  tangential v = [%.6e, %.6e] m/s",
                rel.tangentialVelocityVector_m_s.x,
                rel.tangentialVelocityVector_m_s.y
            );
        }

        ImGui::Text("Orbital angular momentum z about barycenter: %.6e",
            obj->orbitalAngularMomentumZ(app.universe.barycenterPosition())
        );
    }

    //orbit planning/determination predecessor:

    if (obj->kind() == SpaceObjectKind::SpaceCraft) {
    ImGui::Separator();
    ImGui::Text("Spacecraft Mission References");

    int originIdx = obj->originBodyIndex();
    int targetIdx = obj->targetBodyIndex();

    auto objectNameAt = [&](int index) -> const char* {
        if (index < 0 || index >= static_cast<int>(app.universe.objects().size())) return "None";
        return app.universe.objects()[index]->name().c_str();
    };

    if (ImGui::BeginCombo("Origin body", objectNameAt(originIdx))) {
        for (int i = 0; i < static_cast<int>(app.universe.objects().size()); ++i) {
            bool selected = (i == originIdx);
            if (ImGui::Selectable(app.universe.objects()[i]->name().c_str(), selected)) {
                obj->setOriginBodyIndex(i);
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Target body", objectNameAt(targetIdx))) {
        for (int i = 0; i < static_cast<int>(app.universe.objects().size()); ++i) {
            bool selected = (i == targetIdx);
            if (ImGui::Selectable(app.universe.objects()[i]->name().c_str(), selected)) {
                obj->setTargetBodyIndex(i);
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (originIdx >= 0) {
        const auto relOrigin = app.universe.relativeToObject(idx, originIdx);
        ImGui::Text("Velocity relative to origin body:");
        ImGui::Text("  v = [%.6e, %.6e] m/s", relOrigin.velocityVector_m_s.x, relOrigin.velocityVector_m_s.y);
        ImGui::Text("  speed = %.6e m/s", relOrigin.speed_m_s);
    }

    if (targetIdx >= 0) {
        const auto relTarget = app.universe.relativeToObject(idx, targetIdx);
        ImGui::Text("Velocity relative to target body:");
        ImGui::Text("  v = [%.6e, %.6e] m/s", relTarget.velocityVector_m_s.x, relTarget.velocityVector_m_s.y);
        ImGui::Text("  speed = %.6e m/s", relTarget.speed_m_s);
    }
}

    ImGui::Separator();

    if (ImGui::Button("Clear Trail")) obj->clearTrail();

    ImGui::End();
}

void objectListPanel(AppState& app) {
    ImGui::Begin("Objects");

    ImGui::InputDouble("New object orbital radius (m)", &app.newObjectOrbitRadius_m, 0.0, 0.0, "%.6e");
    app.newObjectOrbitRadius_m = std::max(1.0, app.newObjectOrbitRadius_m);
    app.universe.spawnDistanceFromSelected_m = app.newObjectOrbitRadius_m;

    if (ImGui::Button("Add LargeBody")) app.universe.addLargeBody();
    ImGui::SameLine();
    if (ImGui::Button("Add SmallBody")) app.universe.addSmallBody();

    if (ImGui::Button("Add SpaceCraft")) app.universe.addSpaceCraft();
    ImGui::SameLine();
    if (ImGui::Button("Add TestParticle")) app.universe.addTestParticle();

    if (ImGui::Button("Remove Selected")) app.universe.removeSelected();

    ImGui::Separator();

    for (int i = 0; i < static_cast<int>(app.universe.objects().size()); ++i) {
        const auto& obj = *app.universe.objects()[i];

        bool selected = (i == app.universe.selectedIndex());

        std::string label = obj.name() + "##" + std::to_string(i);

        if (ImGui::Selectable(label.c_str(), selected)) {
            app.universe.selectIndex(i);
        }
    }

    ImGui::End();
}

void simulationPanel(AppState& app) {
    ImGui::Begin("Simulation");

    if (ImGui::Button(app.universe.paused ? "Play" : "Pause")) {
        app.universe.paused = !app.universe.paused;
    }

    ImGui::SameLine();

    if (ImGui::Button("Step Once")) {
        app.universe.propagate(app.dt_s, app.method);
    }

    ImGui::InputDouble("dt (s)", &app.dt_s, 0.0, 0.0, "%.6e");
    ImGui::InputInt("Steps / frame", &app.stepsPerFrame);
    app.stepsPerFrame = std::max(1, app.stepsPerFrame);

    int methodIndex = static_cast<int>(app.method);
    const char* methods[] = {"Symplectic Euler", "Velocity Verlet-like", "RK4-like"};

    if (ImGui::Combo("Integrator", &methodIndex, methods, 3)) {
        app.method = static_cast<PropagationMethod>(methodIndex);
    }

    ImGui::Text("Time: %.6f days", app.universe.timeS() / DAY);
    ImGui::Text("Object count: %zu", app.universe.objects().size());

    ImGui::Checkbox("Global collisions", &app.universe.collisionsEnabled);

    ImGui::Separator();

if (ImGui::Button("Blank Preset")) {
    loadBlankPreset(app.universe);
    app.cameraCenter_m = {0.0, 0.0};
    app.metersPerPixel = 1.0e7;
    app.dt_s = 60.0;
    app.stepsPerFrame = 1;
}

if (ImGui::Button("Earth-Moon Preset")) {
    loadBlankPreset(app.universe);
    loadEarthMoonPreset(app.universe);

    app.cameraCenter_m = {0.0, 0.0};

    // Good scale for seeing Earth-Moon separation.
    app.metersPerPixel = 1.0e6;

    // Smaller timestep helps lunar stability.
    app.dt_s = 60.0;
    app.stepsPerFrame = 10;
}

if (ImGui::Button("Solar System Preset")) {
    loadBlankPreset(app.universe);
    loadSolarSystemPreset(app.universe);

    app.cameraCenter_m = {0.0, 0.0};

    // Good scale for seeing inner solar system first.
    // Zoom out to see outer planets.
    app.metersPerPixel = 2.0e9;

    // Keep dt moderate. Large timesteps are why moons/close bodies explode.
    app.dt_s = 300.0;
    app.stepsPerFrame = 20;
}

    ImGui::End();
}

void viewPanel(AppState& app) {
    ImGui::Begin("View");

    ImGui::InputDouble("Meters / pixel", &app.metersPerPixel, 0.0, 0.0, "%.6e");
    ImGui::InputDouble("Camera center x", &app.cameraCenter_m.x, 0.0, 0.0, "%.6e");
    ImGui::InputDouble("Camera center y", &app.cameraCenter_m.y, 0.0, 0.0, "%.6e");

    ImGui::Checkbox("Show labels", &app.showLabels);
    ImGui::Checkbox("Show trails", &app.showTrails);
    ImGui::Checkbox("Show stars", &app.showStars);
    ImGui::Checkbox("Show origin", &app.showOrigin);
    ImGui::Checkbox("Draw true physical sizes", &app.drawTrueSizes);

    if (ImGui::Button("Center on Origin")) {
        app.cameraCenter_m = {0.0, 0.0};
    }

    SpaceObject* selected = app.universe.selected();
    if (selected && ImGui::Button("Center on Selected")) {
        app.cameraCenter_m = selected->position();
    }

    ImGui::End();
}

void handleMouseCanvasEvents(AppState& app, sf::RenderWindow& window, const sf::Event& event) {
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse) return;

    if (event.type == sf::Event::MouseWheelScrolled) {
        const sf::Vector2i mouse = sf::Mouse::getPosition(window);
        const Vec2 before = screenToWorld(mouse, app, window);

        const double zoomFactor = event.mouseWheelScroll.delta > 0.0f ? 0.85 : 1.15;
        app.metersPerPixel *= zoomFactor;
        app.metersPerPixel = std::clamp(app.metersPerPixel, 1.0, 1.0e14);

        const Vec2 after = screenToWorld(mouse, app, window);
        app.cameraCenter_m += before - after;
    }

    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Right) {
            app.isPanning = true;
            app.lastMousePos = sf::Mouse::getPosition(window);
        }

        if (event.mouseButton.button == sf::Mouse::Left) {
            const sf::Vector2i mouse = sf::Mouse::getPosition(window);
            const int idx = nearestObjectIndex(app, mouse, window);
            if (idx >= 0) app.universe.selectIndex(idx);
        }
    }

    if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Right) {
            app.isPanning = false;
        }
    }

    if (event.type == sf::Event::MouseMoved && app.isPanning) {
        const sf::Vector2i mouse = sf::Mouse::getPosition(window);
        const sf::Vector2i delta = mouse - app.lastMousePos;
        app.lastMousePos = mouse;

        app.cameraCenter_m.x -= static_cast<double>(delta.x) * app.metersPerPixel;
        app.cameraCenter_m.y += static_cast<double>(delta.y) * app.metersPerPixel;
    }
}

bool loadFont(sf::Font& font) {
    return font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
}

} // namespace

int main() {
    AppState app;
    loadBlankPreset(app.universe);
    app.stars = makeStars(5000, 6.0e12);

    sf::RenderWindow window(sf::VideoMode(1600, 950), "Astrodyn Dear ImGui Sandbox");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML.\n";
        return 1;
    }

    sf::Font labelFont;
    if (!loadFont(labelFont)) {
        std::cerr << "Warning: could not load label font.\n";
    }

    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event event{};

        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    app.universe.paused = !app.universe.paused;
                }

                if (event.key.code == sf::Keyboard::B) {
                    app.universe.applyImpulsiveBurnToSelected(app.impulsiveBurnDeltaV_m_s, true);
                }

                if (event.key.code == sf::Keyboard::N) {
                    app.universe.applyImpulsiveBurnToSelected(app.impulsiveBurnDeltaV_m_s, false);
                }

                if (event.key.code == sf::Keyboard::T) {
                    app.universe.toggleContinuousBurnForSelected();
                }
            }

            handleMouseCanvasEvents(app, window, event);
        }

        if (!app.universe.paused) {
            for (int i = 0; i < app.stepsPerFrame; ++i) {
                app.universe.propagate(app.dt_s, app.method);
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        simulationPanel(app);
        objectListPanel(app);
        editSelectedObjectPanel(app);
        viewPanel(app);

        drawCanvas(window, app, labelFont);

        ImGui::SFML::Render(window);

        std::ostringstream title;
        title << "Astrodyn | t=" << std::fixed << std::setprecision(3)
              << app.universe.timeS() / DAY << " days"
              << " | objects=" << app.universe.objects().size()
              << " | " << propagationMethodName(app.method);

        window.setTitle(title.str());

        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}