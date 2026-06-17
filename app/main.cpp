#include "astrodyn/Presets.hpp"
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

struct Button {
    sf::FloatRect rect;
    std::string label;
};

struct TextField {
    sf::FloatRect rect;
    std::string label;
    std::string variableName;
    std::string text;
    bool active = false;
};

struct AppState {
    Universe universe;
    PropagationMethod method = PropagationMethod::VelocityVerlet;

    double dt_s = 60.0;
    int stepsPerFrame = 1;

    double metersPerPixel = 1.0e7;
    Vec2 cameraCenter_m{0.0, 0.0};

    bool showPanel = true;
    bool rightDragging = false;
    sf::Vector2i lastMouse;

    std::vector<Button> buttons;
    std::vector<TextField> fields;

    int activeFieldIndex = -1;
};

bool loadFont(sf::Font& font) {
    return font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
}

sf::Vector2f worldToScreen(const Vec2& r, const AppState& app, const sf::RenderWindow& window) {
    const auto size = window.getSize();

    const double sx =
        static_cast<double>(size.x) * 0.5
        + (r.x - app.cameraCenter_m.x) / app.metersPerPixel;

    const double sy =
        static_cast<double>(size.y) * 0.5
        - (r.y - app.cameraCenter_m.y) / app.metersPerPixel;

    return sf::Vector2f(static_cast<float>(sx), static_cast<float>(sy));
}

Vec2 screenToWorld(const sf::Vector2i& p, const AppState& app, const sf::RenderWindow& window) {
    const auto size = window.getSize();

    const double x =
        app.cameraCenter_m.x
        + (static_cast<double>(p.x) - static_cast<double>(size.x) * 0.5) * app.metersPerPixel;

    const double y =
        app.cameraCenter_m.y
        - (static_cast<double>(p.y) - static_cast<double>(size.y) * 0.5) * app.metersPerPixel;

    return {x, y};
}

std::string sci(double v) {
    std::ostringstream s;
    s << std::scientific << std::setprecision(6) << v;
    return s.str();
}

double parseDoubleOrOld(const std::string& text, double oldValue) {
    try {
        size_t used = 0;
        double value = std::stod(text, &used);
        if (used == 0) return oldValue;
        return value;
    } catch (...) {
        return oldValue;
    }
}

double currentValueForField(const AppState& app, const std::string& variableName) {
    const SpaceObject* obj = app.universe.selected();

    if (variableName == "dt") return app.dt_s;
    if (variableName == "stepsPerFrame") return static_cast<double>(app.stepsPerFrame);
    if (variableName == "spawnDistance") return app.universe.spawnDistanceFromSelected_m;

    if (!obj) return 0.0;

    if (variableName == "x") return obj->position().x;
    if (variableName == "y") return obj->position().y;
    if (variableName == "vx") return obj->velocity().x;
    if (variableName == "vy") return obj->velocity().y;
    if (variableName == "px") return obj->momentum().x;
    if (variableName == "py") return obj->momentum().y;
    if (variableName == "mass") return obj->massKg();
    if (variableName == "radius") return obj->radiusM();
    if (variableName == "drawRadius") return obj->drawRadiusPx();

    return 0.0;
}

void refreshFieldText(AppState& app) {
    for (int i = 0; i < static_cast<int>(app.fields.size()); ++i) {
        if (i == app.activeFieldIndex) continue;
        app.fields[i].text = sci(currentValueForField(app, app.fields[i].variableName));
    }
}

void applyField(AppState& app, int fieldIndex) {
    if (fieldIndex < 0 || fieldIndex >= static_cast<int>(app.fields.size())) return;

    TextField& field = app.fields[fieldIndex];
    const double oldValue = currentValueForField(app, field.variableName);
    const double newValue = parseDoubleOrOld(field.text, oldValue);

    if (field.variableName == "dt") {
        app.dt_s = std::max(1e-6, newValue);
        return;
    }

    if (field.variableName == "stepsPerFrame") {
        app.stepsPerFrame = std::max(1, static_cast<int>(newValue));
        return;
    }

    if (field.variableName == "spawnDistance") {
        app.universe.spawnDistanceFromSelected_m = std::max(0.0, newValue);
        return;
    }

    SpaceObject* obj = app.universe.selected();
    if (!obj) return;

    obj->editObject(field.variableName, newValue);
}

void setActiveField(AppState& app, int index) {
    if (app.activeFieldIndex >= 0 && app.activeFieldIndex < static_cast<int>(app.fields.size())) {
        applyField(app, app.activeFieldIndex);
        app.fields[app.activeFieldIndex].active = false;
    }

    app.activeFieldIndex = index;

    if (index >= 0 && index < static_cast<int>(app.fields.size())) {
        app.fields[index].active = true;
        app.fields[index].text = sci(currentValueForField(app, app.fields[index].variableName));
    }
}

void rebuildPanel(AppState& app) {
    app.buttons.clear();
    app.fields.clear();

    float x = 12.0f;
    float y = 12.0f;
    const float bw = 126.0f;
    const float bh = 26.0f;
    const float gap = 6.0f;

    auto addButton = [&](const std::string& label) {
        app.buttons.push_back({sf::FloatRect(x, y, bw, bh), label});
        x += bw + gap;
    };

    addButton("Blank");
    addButton("Earth-Moon");
    addButton("Solar System");
    addButton(app.universe.paused ? "Play" : "Pause");

    x = 12.0f;
    y += bh + gap;

    addButton("Add LargeBody");
    addButton("Add SmallBody");
    addButton("Add SpaceCraft");
    addButton("Add Particle");

    x = 12.0f;
    y += bh + gap;

    addButton("Select Prev");
    addButton("Select Next");
    addButton("Delete");
    addButton("Clear Trails");

    x = 12.0f;
    y += bh + gap;

    addButton("Symplectic");
    addButton("Verlet");
    addButton("RK4");
    addButton(app.universe.collisionsEnabled ? "Collisions ON" : "Collisions OFF");

    x = 12.0f;
    y += bh + gap;

    SpaceObject* selected = app.universe.selected();

    addButton(selected && selected->isGravityEnabled() ? "Gravity ON" : "Gravity OFF");
    addButton(selected && selected->isAffectedByGravity() ? "Affected ON" : "Affected OFF");
    addButton(selected && selected->isDynamic() ? "Dynamic ON" : "Dynamic OFF");
    addButton(selected && selected->isCollisionEnabled() ? "ObjColl ON" : "ObjColl OFF");

    x = 12.0f;
    y += bh + gap;

    addButton(selected && selected->isTrailEnabled() ? "Trail ON" : "Trail OFF");
    addButton("Center View");
    addButton("Origin View");
    addButton(app.showPanel ? "Hide Panel" : "Show Panel");

    // Text fields
    y += bh + 14.0f;
    x = 12.0f;

    const float labelW = 118.0f;
    const float fieldW = 190.0f;
    const float fieldH = 24.0f;

    auto addField = [&](const std::string& label, const std::string& variableName) {
        TextField f;
        f.rect = sf::FloatRect(x + labelW, y, fieldW, fieldH);
        f.label = label;
        f.variableName = variableName;
        f.text = sci(currentValueForField(app, variableName));
        f.active = false;
        app.fields.push_back(f);
        y += fieldH + 5.0f;
    };

    addField("x [m]", "x");
    addField("y [m]", "y");
    addField("vx [m/s]", "vx");
    addField("vy [m/s]", "vy");
    addField("px [kg*m/s]", "px");
    addField("py [kg*m/s]", "py");
    addField("mass [kg]", "mass");
    addField("radius [m]", "radius");
    addField("draw radius [px]", "drawRadius");
    addField("dt [s]", "dt");
    addField("steps/frame", "stepsPerFrame");
    addField("spawn dist [m]", "spawnDistance");
}

void clickButton(AppState& app, const std::string& label) {
    if (label == "Blank") {
        loadBlankPreset(app.universe);
    } else if (label == "Earth-Moon") {
        loadEarthMoonPreset(app.universe);
        app.dt_s = 60.0;
        app.stepsPerFrame = 2;
        app.metersPerPixel = 2.0e6;
        app.cameraCenter_m = {0.0, 0.0};
    } else if (label == "Solar System") {
        loadSolarSystemPreset(app.universe);
        app.dt_s = 3600.0;
        app.stepsPerFrame = 12;
        app.metersPerPixel = 3.0e9;
        app.cameraCenter_m = {0.0, 0.0};
    } else if (label == "Play" || label == "Pause") {
        app.universe.paused = !app.universe.paused;
    } else if (label == "Add LargeBody") {
        app.universe.addLargeBody();
    } else if (label == "Add SmallBody") {
        app.universe.addSmallBody();
    } else if (label == "Add SpaceCraft") {
        app.universe.addSpaceCraft();
    } else if (label == "Add Particle") {
        app.universe.addTestParticle();
    } else if (label == "Select Prev") {
        app.universe.selectPrevious();
    } else if (label == "Select Next") {
        app.universe.selectNext();
    } else if (label == "Delete") {
        app.universe.removeSelected();
    } else if (label == "Clear Trails") {
        for (auto& obj : app.universe.objects()) obj->clearTrail();
    } else if (label == "Symplectic") {
        app.method = PropagationMethod::SymplecticEuler;
    } else if (label == "Verlet") {
        app.method = PropagationMethod::VelocityVerlet;
    } else if (label == "RK4") {
        app.method = PropagationMethod::RK4;
    } else if (label == "Collisions ON" || label == "Collisions OFF") {
        app.universe.collisionsEnabled = !app.universe.collisionsEnabled;
    } else if (label == "Gravity ON" || label == "Gravity OFF") {
        if (auto* obj = app.universe.selected()) obj->setGravityEnabled(!obj->isGravityEnabled());
    } else if (label == "Affected ON" || label == "Affected OFF") {
        if (auto* obj = app.universe.selected()) obj->setAffectedByGravity(!obj->isAffectedByGravity());
    } else if (label == "Dynamic ON" || label == "Dynamic OFF") {
        if (auto* obj = app.universe.selected()) obj->setDynamic(!obj->isDynamic());
    } else if (label == "ObjColl ON" || label == "ObjColl OFF") {
        if (auto* obj = app.universe.selected()) obj->setCollisionEnabled(!obj->isCollisionEnabled());
    } else if (label == "Trail ON" || label == "Trail OFF") {
        if (auto* obj = app.universe.selected()) obj->setTrailEnabled(!obj->isTrailEnabled());
    } else if (label == "Center View") {
        if (auto* obj = app.universe.selected()) app.cameraCenter_m = obj->position();
    } else if (label == "Origin View") {
        app.cameraCenter_m = {0.0, 0.0};
    } else if (label == "Hide Panel" || label == "Show Panel") {
        app.showPanel = !app.showPanel;
    }

    setActiveField(app, -1);
}

void handleTextEntered(AppState& app, sf::Uint32 unicode) {
    if (app.activeFieldIndex < 0 || app.activeFieldIndex >= static_cast<int>(app.fields.size())) return;

    TextField& field = app.fields[app.activeFieldIndex];

    if (unicode == 8) { // backspace
        if (!field.text.empty()) field.text.pop_back();
        return;
    }

    if (unicode == 13) { // enter
        applyField(app, app.activeFieldIndex);
        setActiveField(app, -1);
        return;
    }

    char c = static_cast<char>(unicode);

    const bool allowed =
        (c >= '0' && c <= '9') ||
        c == '-' ||
        c == '+' ||
        c == '.' ||
        c == 'e' ||
        c == 'E';

    if (allowed) {
        field.text.push_back(c);
    }
}

void drawText(sf::RenderWindow& window, const sf::Font& font, const std::string& text, float x, float y, unsigned size = 14) {
    sf::Text t;
    t.setFont(font);
    t.setString(text);
    t.setCharacterSize(size);
    t.setFillColor(sf::Color(235, 240, 250));
    t.setPosition(x, y);
    window.draw(t);
}

void drawButton(sf::RenderWindow& window, const sf::Font& font, const Button& button) {
    sf::RectangleShape r;
    r.setPosition(button.rect.left, button.rect.top);
    r.setSize({button.rect.width, button.rect.height});
    r.setFillColor(sf::Color(35, 42, 60, 230));
    r.setOutlineColor(sf::Color(110, 125, 160));
    r.setOutlineThickness(1.0f);
    window.draw(r);

    drawText(window, font, button.label, button.rect.left + 8.0f, button.rect.top + 4.0f, 13);
}

void drawField(sf::RenderWindow& window, const sf::Font& font, const TextField& field) {
    drawText(window, font, field.label, field.rect.left - 115.0f, field.rect.top + 3.0f, 13);

    sf::RectangleShape r;
    r.setPosition(field.rect.left, field.rect.top);
    r.setSize({field.rect.width, field.rect.height});
    r.setFillColor(field.active ? sf::Color(58, 65, 90, 240) : sf::Color(18, 22, 34, 235));
    r.setOutlineColor(field.active ? sf::Color::White : sf::Color(100, 110, 135));
    r.setOutlineThickness(1.0f);
    window.draw(r);

    drawText(window, font, field.text, field.rect.left + 6.0f, field.rect.top + 3.0f, 13);
}

void drawPanel(sf::RenderWindow& window, const sf::Font& font, AppState& app) {
    if (!app.showPanel) return;

    refreshFieldText(app);

    sf::RectangleShape panel;
    panel.setPosition(0.0f, 0.0f);
    panel.setSize({465.0f, static_cast<float>(window.getSize().y)});
    panel.setFillColor(sf::Color(0, 0, 0, 170));
    window.draw(panel);

    for (const auto& b : app.buttons) drawButton(window, font, b);
    for (const auto& f : app.fields) drawField(window, font, f);

    const SpaceObject* obj = app.universe.selected();

    float y = 600.0f;
    std::ostringstream info;

    info << "Selected: ";
    if (obj) {
        info << obj->name() << " [" << objectKindName(obj->kind()) << "]\n";
        info << "pos = " << sci(obj->position().x) << ", " << sci(obj->position().y) << "\n";
        info << "vel = " << sci(obj->velocity().x) << ", " << sci(obj->velocity().y) << "\n";
        info << "mass = " << sci(obj->massKg()) << " kg\n";
        info << "radius = " << sci(obj->radiusM()) << " m\n";
    } else {
        info << "none\n";
    }

    info << "\nTime: " << std::fixed << std::setprecision(3) << app.universe.timeS() / DAY << " days\n";
    info << "Method: " << propagationMethodName(app.method) << "\n";
    info << "Objects: " << app.universe.objects().size() << "\n";
    info << "Mouse wheel zoom | Right-drag pan | Space pause/play\n";

    drawText(window, font, info.str(), 16.0f, y, 13);
}

double hash01(int x, int y, int seed) {
    int n = x * 374761393 + y * 668265263 + seed * 1442695041;
    n = (n ^ (n >> 13)) * 1274126177;
    n = n ^ (n >> 16);
    return (n & 0x00FFFFFF) / static_cast<double>(0x01000000);
}

void drawStarBackground(sf::RenderWindow& window, const AppState& app) {
    const auto size = window.getSize();

    const double cellPx = 90.0;
    const double cellWorld = cellPx * app.metersPerPixel;

    const Vec2 topLeft = screenToWorld({0, 0}, app, window);
    const Vec2 bottomRight = screenToWorld(
        {static_cast<int>(size.x), static_cast<int>(size.y)},
        app,
        window
    );

    const int minX = static_cast<int>(std::floor(std::min(topLeft.x, bottomRight.x) / cellWorld)) - 2;
    const int maxX = static_cast<int>(std::ceil(std::max(topLeft.x, bottomRight.x) / cellWorld)) + 2;
    const int minY = static_cast<int>(std::floor(std::min(topLeft.y, bottomRight.y) / cellWorld)) - 2;
    const int maxY = static_cast<int>(std::ceil(std::max(topLeft.y, bottomRight.y) / cellWorld)) + 2;

    for (int gx = minX; gx <= maxX; ++gx) {
        for (int gy = minY; gy <= maxY; ++gy) {
            const double rx = hash01(gx, gy, 1);
            const double ry = hash01(gx, gy, 2);
            const double bright = hash01(gx, gy, 3);

            Vec2 starWorld{
                (gx + rx) * cellWorld,
                (gy + ry) * cellWorld
            };

            sf::Vector2f p = worldToScreen(starWorld, app, window);

            if (p.x < 0 || p.y < 0 || p.x > size.x || p.y > size.y) continue;

            const float radius = bright > 0.93 ? 1.7f : 1.0f;
            const sf::Uint8 alpha = static_cast<sf::Uint8>(80 + 150 * bright);

            sf::CircleShape star(radius);
            star.setOrigin(radius, radius);
            star.setPosition(p);
            star.setFillColor(sf::Color(210, 220, 255, alpha));
            window.draw(star);
        }
    }
}

void drawOriginMarker(sf::RenderWindow& window, const AppState& app) {
    const sf::Vector2f p = worldToScreen({0.0, 0.0}, app, window);

    sf::VertexArray cross(sf::Lines, 4);

    cross[0].position = {p.x - 12.0f, p.y};
    cross[1].position = {p.x + 12.0f, p.y};
    cross[2].position = {p.x, p.y - 12.0f};
    cross[3].position = {p.x, p.y + 12.0f};

    for (int i = 0; i < 4; ++i) {
        cross[i].color = sf::Color(255, 80, 80, 220);
    }

    window.draw(cross);

    sf::CircleShape ring(18.0f);
    ring.setOrigin(18.0f, 18.0f);
    ring.setPosition(p);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(255, 80, 80, 180));
    ring.setOutlineThickness(1.0f);
    window.draw(ring);
}

void drawUniverse(sf::RenderWindow& window, const AppState& app) {
    drawStarBackground(window, app);
    drawOriginMarker(window, app);

    for (const auto& objPtr : app.universe.objects()) {
        const SpaceObject& obj = *objPtr;

        const auto& trail = obj.trail();

        if (trail.size() > 1 && obj.isTrailEnabled()) {
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

    for (const auto& objPtr : app.universe.objects()) {
        const SpaceObject& obj = *objPtr;

        float radius = obj.drawRadiusPx();
        if (obj.isSelected()) radius += 3.0f;

        sf::CircleShape shape(radius);
        shape.setOrigin(radius, radius);
        shape.setPosition(worldToScreen(obj.position(), app, window));

        sf::Color color(
            static_cast<sf::Uint8>(obj.colorR()),
            static_cast<sf::Uint8>(obj.colorG()),
            static_cast<sf::Uint8>(obj.colorB())
        );

        if (!obj.isGravityEnabled() && obj.kind() != SpaceObjectKind::SpaceCraft && obj.kind() != SpaceObjectKind::TestParticle) {
            color = sf::Color(90, 90, 90);
        }

        shape.setFillColor(color);

        if (obj.isSelected()) {
            shape.setOutlineColor(sf::Color::White);
            shape.setOutlineThickness(2.0f);
        }

        window.draw(shape);
    }
}

void handleLeftClick(AppState& app, const sf::Vector2i& mouse, const sf::RenderWindow& window) {
    const sf::Vector2f m(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

    if (app.showPanel) {
        for (const auto& b : app.buttons) {
            if (b.rect.contains(m)) {
                clickButton(app, b.label);
                rebuildPanel(app);
                return;
            }
        }

        for (int i = 0; i < static_cast<int>(app.fields.size()); ++i) {
            if (app.fields[i].rect.contains(m)) {
                setActiveField(app, i);
                return;
            }
        }
    }

    // If clicking in the canvas, select nearest visible object.
    double bestDist2 = 1.0e100;
    int bestIndex = -1;

    for (int i = 0; i < static_cast<int>(app.universe.objects().size()); ++i) {
        const auto& obj = *app.universe.objects()[i];
        sf::Vector2f p = worldToScreen(obj.position(), app, window);

        const double dx = static_cast<double>(p.x) - mouse.x;
        const double dy = static_cast<double>(p.y) - mouse.y;
        const double d2 = dx * dx + dy * dy;

        const double pickRadius = std::max(12.0, static_cast<double>(obj.drawRadiusPx()) + 6.0);

        if (d2 < pickRadius * pickRadius && d2 < bestDist2) {
            bestDist2 = d2;
            bestIndex = i;
        }
    }

    if (bestIndex >= 0) {
        app.universe.selectIndex(bestIndex);
        rebuildPanel(app);
    }
}

} // namespace

int main() {
    AppState app;
    loadBlankPreset(app.universe);

    sf::RenderWindow window(sf::VideoMode(1450, 900), "Astrodyn Mouse Sandbox");
    window.setFramerateLimit(60);

    sf::Font font;
    const bool hasFont = loadFont(font);

    rebuildPanel(app);

    while (window.isOpen()) {
        sf::Event event{};

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) window.close();

                // Keep Space as the one keyboard control.
                if (event.key.code == sf::Keyboard::Space) {
                    app.universe.paused = !app.universe.paused;
                    rebuildPanel(app);
                }
            }

            if (event.type == sf::Event::TextEntered) {
                handleTextEntered(app, event.text.unicode);
            }

            if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.delta > 0) app.metersPerPixel *= 0.85;
                if (event.mouseWheelScroll.delta < 0) app.metersPerPixel *= 1.15;
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                const sf::Vector2i mouse{event.mouseButton.x, event.mouseButton.y};

                if (event.mouseButton.button == sf::Mouse::Left) {
                    handleLeftClick(app, mouse, window);
                }

                if (event.mouseButton.button == sf::Mouse::Right) {
                    app.rightDragging = true;
                    app.lastMouse = mouse;
                }
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Right) {
                    app.rightDragging = false;
                }
            }

            if (event.type == sf::Event::MouseMoved) {
                const sf::Vector2i mouse{event.mouseMove.x, event.mouseMove.y};

                if (app.rightDragging) {
                    const sf::Vector2i delta = mouse - app.lastMouse;

                    app.cameraCenter_m.x -= static_cast<double>(delta.x) * app.metersPerPixel;
                    app.cameraCenter_m.y += static_cast<double>(delta.y) * app.metersPerPixel;

                    app.lastMouse = mouse;
                }
            }
        }

        if (!app.universe.paused) {
            for (int i = 0; i < app.stepsPerFrame; ++i) {
                app.universe.propagate(app.dt_s, app.method);
            }
        }

        refreshFieldText(app);
        rebuildPanel(app);

        window.clear(sf::Color(5, 7, 14));
        drawUniverse(window, app);

        if (hasFont) {
            drawPanel(window, font, app);
        }

        std::ostringstream title;
        title << "Astrodyn Mouse Sandbox | "
              << "t=" << std::fixed << std::setprecision(2) << app.universe.timeS() / DAY
              << " days | objects=" << app.universe.objects().size()
              << " | " << propagationMethodName(app.method)
              << " | Space pause/play | Right-drag pan | Wheel zoom";

        window.setTitle(title.str());

        window.display();
    }

    return 0;
}