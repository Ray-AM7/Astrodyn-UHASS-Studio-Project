#include "astrodyn/SpaceObject.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace astrodyn {

SpaceObject::SpaceObject(
    std::string name,
    SpaceObjectKind kind,
    Vec2 position_m,
    Vec2 velocity_m_s
)
    : name_(std::move(name)),
      kind_(kind),
      r_m_(position_m),
      v_m_s_(velocity_m_s) {}

const std::string& SpaceObject::name() const { return name_; }
void SpaceObject::setName(const std::string& name) { name_ = name; }

SpaceObjectKind SpaceObject::kind() const { return kind_; }

Vec2 SpaceObject::position() const { return r_m_; }
Vec2 SpaceObject::velocity() const { return v_m_s_; }

Vec2 SpaceObject::momentum() const {
    return massKg() * v_m_s_;
}

void SpaceObject::setPosition(Vec2 r_m) { r_m_ = r_m; }
void SpaceObject::setVelocity(Vec2 v_m_s) { v_m_s_ = v_m_s; }

void SpaceObject::setMomentum(Vec2 p_kg_m_s) {
    const double m = massKg();
    if (m > 0.0) v_m_s_ = p_kg_m_s / m;
}

double SpaceObject::massKg() const { return mass_kg_; }
double SpaceObject::radiusM() const { return radius_m_; }
double SpaceObject::gravitationalParameter() const { return G * mass_kg_; }

void SpaceObject::setMassKg(double mass_kg) {
    mass_kg_ = std::max(0.0, mass_kg);
}

void SpaceObject::setRadiusM(double radius_m) {
    radius_m_ = std::max(0.0, radius_m);
}

bool SpaceObject::isSelected() const { return selected_; }
void SpaceObject::setSelected(bool selected) { selected_ = selected; }

bool SpaceObject::isDynamic() const { return dynamic_; }
void SpaceObject::setDynamic(bool dynamic) { dynamic_ = dynamic; }

bool SpaceObject::isGravityEnabled() const { return gravityEnabled_; }
void SpaceObject::setGravityEnabled(bool enabled) { gravityEnabled_ = enabled; }

bool SpaceObject::isAffectedByGravity() const { return affectedByGravity_; }
void SpaceObject::setAffectedByGravity(bool affected) { affectedByGravity_ = affected; }

bool SpaceObject::isCollisionEnabled() const { return collisionEnabled_; }
void SpaceObject::setCollisionEnabled(bool enabled) { collisionEnabled_ = enabled; }

bool SpaceObject::isTrailEnabled() const { return trailEnabled_; }
void SpaceObject::setTrailEnabled(bool enabled) { trailEnabled_ = enabled; }

float SpaceObject::drawRadiusPx() const { return drawRadiusPx_; }

void SpaceObject::setDrawRadiusPx(float r) {
    drawRadiusPx_ = std::max(1.0f, r);
}

int SpaceObject::colorR() const { return colorR_; }
int SpaceObject::colorG() const { return colorG_; }
int SpaceObject::colorB() const { return colorB_; }

void SpaceObject::setColor(int r, int g, int b) {
    colorR_ = std::clamp(r, 0, 255);
    colorG_ = std::clamp(g, 0, 255);
    colorB_ = std::clamp(b, 0, 255);
}

const std::vector<Vec2>& SpaceObject::trail() const { return trail_m_; }

void SpaceObject::clearTrail() {
    trail_m_.clear();
}

void SpaceObject::pushTrailPoint() {
    if (!trailEnabled_) return;

    trail_m_.push_back(r_m_);

    constexpr std::size_t maxTrailPoints = 10000;
    if (trail_m_.size() > maxTrailPoints) {
        trail_m_.erase(
            trail_m_.begin(),
            trail_m_.begin() + static_cast<long>(trail_m_.size() - maxTrailPoints)
        );
    }
}

bool SpaceObject::editObject(const std::string& variableName, double newValue) {
    if (variableName == "x") {
        r_m_.x = newValue;
        return true;
    }
    if (variableName == "y") {
        r_m_.y = newValue;
        return true;
    }
    if (variableName == "vx") {
        v_m_s_.x = newValue;
        return true;
    }
    if (variableName == "vy") {
        v_m_s_.y = newValue;
        return true;
    }
    if (variableName == "px") {
        Vec2 p = momentum();
        p.x = newValue;
        setMomentum(p);
        return true;
    }
    if (variableName == "py") {
        Vec2 p = momentum();
        p.y = newValue;
        setMomentum(p);
        return true;
    }
    if (variableName == "mass") {
        setMassKg(newValue);
        return true;
    }
    if (variableName == "radius") {
        setRadiusM(newValue);
        return true;
    }
    if (variableName == "drawRadius") {
        setDrawRadiusPx(static_cast<float>(newValue));
        return true;
    }

    return false;
}

void SpaceObject::propagateObj(
    double timestep_s,
    PropagationMethod method,
    const ForceState& forces
) {
    if (!dynamic_) return;

    switch (method) {
        case PropagationMethod::SymplecticEuler:
            propagateSymplecticEuler(timestep_s, forces);
            break;
        case PropagationMethod::VelocityVerlet:
            propagateVelocityVerletLike(timestep_s, forces);
            break;
        case PropagationMethod::RK4:
            propagateRK4Simple(timestep_s, forces);
            break;
    }

    pushTrailPoint();
}

void SpaceObject::propagateSymplecticEuler(double dt_s, const ForceState& forces) {
    v_m_s_ += forces.acceleration_m_s2 * dt_s;
    r_m_ += v_m_s_ * dt_s;
}

void SpaceObject::propagateVelocityVerletLike(double dt_s, const ForceState& forces) {
    // This is a simplified Verlet-like step using current acceleration only.
    // A true velocity Verlet would need acceleration at both old and new positions,
    // which belongs in Universe, not inside one isolated object.
    r_m_ += v_m_s_ * dt_s + 0.5 * forces.acceleration_m_s2 * dt_s * dt_s;
    v_m_s_ += forces.acceleration_m_s2 * dt_s;
}

void SpaceObject::propagateRK4Simple(double dt_s, const ForceState& forces) {
    // For now this is constant-acceleration RK-like behavior.
    // Once forces are evaluated inside Universe multiple times, RK4 can become real RK4.
    r_m_ += v_m_s_ * dt_s + 0.5 * forces.acceleration_m_s2 * dt_s * dt_s;
    v_m_s_ += forces.acceleration_m_s2 * dt_s;
}

} // namespace astrodyn