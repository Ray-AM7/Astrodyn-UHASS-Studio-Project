#include "astrodyn/Presets.hpp"
#include <cmath>

namespace astrodyn {

namespace {

Vec2 positionFromElements(double a_m, double e, double trueAnomaly_rad) {
    const double r = a_m * (1.0 - e * e) / (1.0 + e * std::cos(trueAnomaly_rad));
    return {r * std::cos(trueAnomaly_rad), r * std::sin(trueAnomaly_rad)};
}

Vec2 velocityFromElements(double muCentral, double a_m, double e, double trueAnomaly_rad) {
    const double p = a_m * (1.0 - e * e);
    const double h = std::sqrt(muCentral * p);

    const double vr = (muCentral / h) * e * std::sin(trueAnomaly_rad);
    const double vt = (muCentral / h) * (1.0 + e * std::cos(trueAnomaly_rad));

    const Vec2 rHat{std::cos(trueAnomaly_rad), std::sin(trueAnomaly_rad)};
    const Vec2 tHat{-std::sin(trueAnomaly_rad), std::cos(trueAnomaly_rad)};

    return vr * rHat + vt * tHat;
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
    float drawRadius
) {
    auto obj = std::make_unique<LargeBody>(name, r_m, v_m_s);
    obj->setMassKg(mass_kg);
    obj->setRadiusM(radius_m);
    obj->setColor(cr, cg, cb);
    obj->setDrawRadiusPx(drawRadius);
    obj->setGravityEnabled(true);
    obj->setAffectedByGravity(true);
    obj->setCollisionEnabled(true);
    obj->setTrailEnabled(true);
    return obj;
}

std::unique_ptr<LargeBody> makeOrbitingLarge(
    const std::string& name,
    double mass_kg,
    double radius_m,
    double centralMu,
    double a_m,
    double e,
    double f_rad,
    int cr,
    int cg,
    int cb,
    float drawRadius
) {
    return makeLarge(
        name,
        mass_kg,
        radius_m,
        positionFromElements(a_m, e, f_rad),
        velocityFromElements(centralMu, a_m, e, f_rad),
        cr,
        cg,
        cb,
        drawRadius
    );
}

std::unique_ptr<LargeBody> makeMoonAround(
    const std::string& name,
    const SpaceObject& parent,
    double mass_kg,
    double radius_m,
    double a_m,
    double e,
    double f_rad,
    int cr,
    int cg,
    int cb,
    float drawRadius
) {
    const double parentMu = parent.gravitationalParameter();

    Vec2 relR = positionFromElements(a_m, e, f_rad);
    Vec2 relV = velocityFromElements(parentMu, a_m, e, f_rad);

    return makeLarge(
        name,
        mass_kg,
        radius_m,
        parent.position() + relR,
        parent.velocity() + relV,
        cr,
        cg,
        cb,
        drawRadius
    );
}

} // namespace

void loadBlankPreset(Universe& universe) {
    universe.clear();
    universe.spawnDistanceFromSelected_m = 1.0e7;
}

void loadEarthMoonPreset(Universe& universe) {
    universe.clear();

    auto earth = makeLarge(
        "Earth",
        5.9722e24,
        6371.0e3,
        {0.0, 0.0},
        {0.0, 0.0},
        70, 120, 255,
        18.0f
    );

    earth->setDynamic(false);
    earth->setAffectedByGravity(false);
    universe.addObject(std::move(earth));

    const SpaceObject& earthRef = *universe.objects()[0];

    auto moon = makeMoonAround(
        "Moon",
        earthRef,
        7.342e22,
        1737.4e3,
        384400e3,
        0.0549,
        0.7,
        190, 190, 190,
        6.0f
    );

    universe.addObject(std::move(moon));

    auto craft = std::make_unique<SpaceCraft>(
        "SpaceCraft",
        Vec2{6371.0e3 + 500e3, 0.0},
        Vec2{0.0, std::sqrt(earthRef.gravitationalParameter() / (6371.0e3 + 500e3))}
    );

    craft->setMassKg(500.0);
    craft->setRadiusM(2.0);
    universe.addObject(std::move(craft));

    universe.selectIndex(2);
    universe.spawnDistanceFromSelected_m = 1.0e7;
}

void loadSolarSystemPreset(Universe& universe) {
    universe.clear();

    auto sun = makeLarge(
        "Sun",
        1.9885e30,
        696340e3,
        {0.0, 0.0},
        {0.0, 0.0},
        255, 210, 60,
        20.0f
    );

    sun->setDynamic(false);
    sun->setAffectedByGravity(false);
    sun->setTrailEnabled(false);

    universe.addObject(std::move(sun));

    const double muSun = universe.objects()[0]->gravitationalParameter();

    // Planets: mass, radius, semi-major axis, eccentricity, arbitrary true anomaly.
    universe.addObject(makeOrbitingLarge("Mercury", 3.3011e23, 2439.7e3, muSun, 57.909227e9, 0.2056, 0.2, 170, 160, 150, 4.0f));
    universe.addObject(makeOrbitingLarge("Venus",   4.8675e24, 6051.8e3, muSun, 108.209475e9, 0.0068, 1.1, 230, 190, 120, 6.0f));
    universe.addObject(makeOrbitingLarge("Earth",   5.9722e24, 6371.0e3, muSun, 149.598262e9, 0.0167, 2.0, 70, 120, 255, 7.0f));
    universe.addObject(makeOrbitingLarge("Mars",    6.4171e23, 3389.5e3, muSun, 227.943824e9, 0.0934, 3.0, 230, 90, 60, 6.0f));
    universe.addObject(makeOrbitingLarge("Jupiter", 1.8982e27, 69911e3,  muSun, 778.340821e9, 0.0489, 4.0, 220, 170, 120, 11.0f));
    universe.addObject(makeOrbitingLarge("Saturn",  5.6834e26, 58232e3,  muSun, 1426.666422e9, 0.0565, 5.0, 220, 190, 130, 10.0f));
    universe.addObject(makeOrbitingLarge("Uranus",  8.6810e25, 25362e3,  muSun, 2870.658186e9, 0.0463, 0.8, 140, 210, 220, 8.0f));
    universe.addObject(makeOrbitingLarge("Neptune", 1.02413e26, 24622e3, muSun, 4498.396441e9, 0.0097, 2.6, 80, 120, 230, 8.0f));

    // Asteroid belt marker.
    universe.addObject(makeOrbitingLarge("Ceres", 9.3835e20, 469.7e3, muSun, 413.7e9, 0.0758, 3.8, 160, 160, 160, 4.0f));

    // Inner-planet moons only: Earth Moon + Mars Phobos/Deimos.
    SpaceObject& earth = *universe.objects()[3];
    universe.addObject(makeMoonAround("Moon", earth, 7.342e22, 1737.4e3, 384400e3, 0.0549, 0.4, 190, 190, 190, 5.0f));

    SpaceObject& mars = *universe.objects()[4];
    universe.addObject(makeMoonAround("Phobos", mars, 1.0659e16, 11.267e3, 9376e3, 0.0151, 1.2, 170, 160, 150, 3.0f));
    universe.addObject(makeMoonAround("Deimos", mars, 1.4762e15, 6.2e3, 23463e3, 0.00033, 2.4, 160, 150, 140, 3.0f));

    universe.selectIndex(0);
    universe.spawnDistanceFromSelected_m = 1.0e10;
}

} // namespace astrodyn