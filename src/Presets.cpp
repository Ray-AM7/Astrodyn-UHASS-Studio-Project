#include "astrodyn/Presets.hpp"
#include <cmath>
#include <memory>

namespace astrodyn {

namespace {

constexpr double PI = 3.141592653589793238462643383279502884;

// ------------------------
// Kepler helpers, 2D only.
// ------------------------

Vec2 positionFromElements(double a_m, double e, double trueAnomaly_rad) {
    const double r = a_m * (1.0 - e * e) / (1.0 + e * std::cos(trueAnomaly_rad));
    return {r * std::cos(trueAnomaly_rad), r * std::sin(trueAnomaly_rad)};
}

Vec2 velocityFromElements(double muCentral, double a_m, double e, double trueAnomaly_rad) {
    // Standard perifocal-frame velocity:
    // v = sqrt(mu / p) * [-sin(f), e + cos(f)]
    const double p = a_m * (1.0 - e * e);
    const double factor = std::sqrt(muCentral / p);

    return {
        -factor * std::sin(trueAnomaly_rad),
         factor * (e + std::cos(trueAnomaly_rad))
    };
}

std::unique_ptr<LargeBody> makeLarge(
    const std::string& name,
    double mass_kg,
    double radius_m,
    Vec2 r_m,
    Vec2 v_m_s,
    int cr,
    int cg,
    int cb,
    float fallbackDrawRadiusPx
) {
    auto obj = std::make_unique<LargeBody>(name, r_m, v_m_s);

    obj->setMassKg(mass_kg);
    obj->setRadiusM(radius_m);
    obj->setColor(cr, cg, cb);
    obj->setDrawRadiusPx(fallbackDrawRadiusPx);

    obj->setGravityEnabled(true);
    obj->setAffectedByGravity(true);
    obj->setCollisionEnabled(true);
    obj->setTrailEnabled(true);
    obj->setDynamic(true);

    return obj;
}

std::unique_ptr<LargeBody> makeOrbitingLarge(
    const std::string& name,
    double mass_kg,
    double radius_m,
    double centralMu,
    double a_m,
    double e,
    double trueAnomaly_rad,
    int cr,
    int cg,
    int cb,
    float fallbackDrawRadiusPx
) {
    return makeLarge(
        name,
        mass_kg,
        radius_m,
        positionFromElements(a_m, e, trueAnomaly_rad),
        velocityFromElements(centralMu, a_m, e, trueAnomaly_rad),
        cr,
        cg,
        cb,
        fallbackDrawRadiusPx
    );
}

std::unique_ptr<LargeBody> makeMoonAround(
    const std::string& name,
    const SpaceObject& parent,
    double mass_kg,
    double radius_m,
    double a_m,
    double e,
    double trueAnomaly_rad,
    int cr,
    int cg,
    int cb,
    float fallbackDrawRadiusPx
) {
    const double parentMu = parent.gravitationalParameter();

    const Vec2 relR = positionFromElements(a_m, e, trueAnomaly_rad);
    const Vec2 relV = velocityFromElements(parentMu, a_m, e, trueAnomaly_rad);

    return makeLarge(
        name,
        mass_kg,
        radius_m,
        parent.position() + relR,
        parent.velocity() + relV,
        cr,
        cg,
        cb,
        fallbackDrawRadiusPx
    );
}

std::unique_ptr<SpaceCraft> makeEarthParkingCraft(const SpaceObject& earth) {
    const double altitude_m = 500.0e3;
    const double r_m = earth.radiusM() + altitude_m;
    const double v_m_s = std::sqrt(earth.gravitationalParameter() / r_m);

    auto craft = std::make_unique<SpaceCraft>(
        "SpaceCraft",
        earth.position() + Vec2{r_m, 0.0},
        earth.velocity() + Vec2{0.0, v_m_s}
    );

    craft->setMassKg(500.0);
    craft->setRadiusM(2.0);
    craft->setDrawRadiusPx(5.0f);
    craft->setGravityEnabled(false);
    craft->setAffectedByGravity(true);
    craft->setCollisionEnabled(true);
    craft->setTrailEnabled(true);
    craft->setDynamic(true);

    return craft;
}

void addPresetObject(Universe& universe, std::unique_ptr<SpaceObject> obj) {
    // Critical: false means "do not spawn relative to selected object".
    // Keep the exact preset position and velocity.
    universe.addObject(std::move(obj), false);
}

} // namespace

void loadBlankPreset(Universe& universe) {
    universe.clear();
    universe.spawnDistanceFromSelected_m = 1.0e7;
}

void loadEarthMoonPreset(Universe& universe) {
    loadBlankPreset(universe);

    auto earth = makeLarge(
        "Earth",
        5.9722e24,     // kg
        6371.0e3,      // mean radius, m
        {0.0, 0.0},
        {0.0, 0.0},
        70, 120, 255,
        18.0f
    );

    earth->setDynamic(false);
    earth->setAffectedByGravity(false);
    earth->setTrailEnabled(false);

    addPresetObject(universe, std::move(earth));

    const SpaceObject& earthRef = *universe.objects()[0];

    auto moon = makeMoonAround(
        "Moon",
        earthRef,
        7.342e22,       // kg
        1737.4e3,       // m
        384400.0e3,     // semi-major axis from Earth, m
        0.0549,
        0.7,
        190, 190, 190,
        6.0f
    );

    addPresetObject(universe, std::move(moon));

    addPresetObject(universe, makeEarthParkingCraft(earthRef));

    universe.selectIndex(0);
    universe.spawnDistanceFromSelected_m = 1.0e7;
}

void loadSolarSystemPreset(Universe& universe) {
    loadBlankPreset(universe);

    // Sun fixed at origin.
    auto sun = makeLarge(
        "Sun",
        1.9885e30,     // kg
        696340.0e3,    // m
        {0.0, 0.0},
        {0.0, 0.0},
        255, 210, 60,
        20.0f
    );

    sun->setDynamic(false);
    sun->setAffectedByGravity(false);
    sun->setTrailEnabled(false);

    addPresetObject(universe, std::move(sun));

    const double muSun = universe.objects()[0]->gravitationalParameter();

    // Planet values:
    // mass kg, mean radius m, semi-major axis m, eccentricity.
    // True anomalies are arbitrary but non-overlapping.
    addPresetObject(universe, makeOrbitingLarge(
        "Mercury",
        3.3011e23,
        2439.7e3,
        muSun,
        57.909227e9,
        0.2056,
        0.20,
        170, 160, 150,
        4.0f
    ));

    addPresetObject(universe, makeOrbitingLarge(
        "Venus",
        4.8675e24,
        6051.8e3,
        muSun,
        108.209475e9,
        0.0068,
        1.10,
        230, 190, 120,
        6.0f
    ));

    addPresetObject(universe, makeOrbitingLarge(
        "Earth",
        5.9722e24,
        6371.0e3,
        muSun,
        149.598262e9,
        0.0167,
        2.00,
        70, 120, 255,
        7.0f
    ));

    addPresetObject(universe, makeOrbitingLarge(
        "Mars",
        6.4171e23,
        3389.5e3,
        muSun,
        227.943824e9,
        0.0934,
        3.00,
        230, 90, 60,
        6.0f
    ));

    addPresetObject(universe, makeOrbitingLarge(
        "Jupiter",
        1.8982e27,
        69911.0e3,
        muSun,
        778.340821e9,
        0.0489,
        4.00,
        220, 170, 120,
        11.0f
    ));

    addPresetObject(universe, makeOrbitingLarge(
        "Saturn",
        5.6834e26,
        58232.0e3,
        muSun,
        1426.666422e9,
        0.0565,
        5.00,
        220, 190, 130,
        10.0f
    ));

    addPresetObject(universe, makeOrbitingLarge(
        "Uranus",
        8.6810e25,
        25362.0e3,
        muSun,
        2870.658186e9,
        0.0463,
        0.80,
        140, 210, 220,
        8.0f
    ));

    addPresetObject(universe, makeOrbitingLarge(
        "Neptune",
        1.02413e26,
        24622.0e3,
        muSun,
        4498.396441e9,
        0.0097,
        2.60,
        80, 120, 230,
        8.0f
    ));

    // Asteroid belt marker as normal LargeBody object.
    addPresetObject(universe, makeOrbitingLarge(
        "Ceres",
        9.3835e20,
        469.7e3,
        muSun,
        413.7e9,
        0.0758,
        3.80,
        160, 160, 160,
        4.0f
    ));

    // Inner-planet moons only: Earth's Moon + Mars' Phobos/Deimos.
    // Current object order:
    // 0 Sun, 1 Mercury, 2 Venus, 3 Earth, 4 Mars, 5 Jupiter, 6 Saturn, 7 Uranus, 8 Neptune, 9 Ceres.
    SpaceObject& earth = *universe.objects()[3];

    addPresetObject(universe, makeMoonAround(
        "Moon",
        earth,
        7.342e22,
        1737.4e3,
        384400.0e3,
        0.0549,
        0.40,
        190, 190, 190,
        5.0f
    ));

    SpaceObject& mars = *universe.objects()[4];

    // NASA gives Phobos as roughly 27 x 22 x 18 km and Deimos as roughly
    // 15 x 12 x 11 km; use mean-radius approximations for this 2D sandbox.
    addPresetObject(universe, makeMoonAround(
        "Phobos",
        mars,
        1.0659e16,
        11.1e3,
        9376.0e3,
        0.0151,
        1.20,
        170, 160, 150,
        3.0f
    ));

    addPresetObject(universe, makeMoonAround(
        "Deimos",
        mars,
        1.4762e15,
        6.2e3,
        23463.0e3,
        0.00033,
        2.40,
        160, 150, 140,
        3.0f
    ));

    universe.selectIndex(0);
    universe.spawnDistanceFromSelected_m = 1.0e10;
}

} // namespace astrodyn