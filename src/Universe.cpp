#include "astrodyn/Universe.hpp"
#include <algorithm>
#include <cmath>

namespace astrodyn {

namespace {

constexpr double SOFTENING_M = 1.0;
constexpr double RESTITUTION = 0.15;

} // namespace

Universe::Universe() = default;

void Universe::clear() {
    objects_.clear();
    selectedIndex_ = -1;
    time_s_ = 0.0;
}

int Universe::addObject(std::unique_ptr<SpaceObject> obj) {
    if (!obj) return -1;

    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(objects_.size())) {
        const SpaceObject* parent = objects_[selectedIndex_].get();
        obj->setPosition(parent->position() + Vec2{spawnDistanceFromSelected_m, 0.0});
        obj->setVelocity(parent->velocity());
    }

    objects_.push_back(std::move(obj));
    selectIndex(static_cast<int>(objects_.size()) - 1);
    return selectedIndex_;
}

int Universe::addLargeBody() {
    return addObject(std::make_unique<LargeBody>());
}

int Universe::addSmallBody() {
    return addObject(std::make_unique<SmallBody>());
}

int Universe::addSpaceCraft() {
    return addObject(std::make_unique<SpaceCraft>());
}

int Universe::addTestParticle() {
    return addObject(std::make_unique<TestParticle>());
}

void Universe::removeSelected() {
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(objects_.size())) return;

    objects_.erase(objects_.begin() + selectedIndex_);

    if (objects_.empty()) {
        selectedIndex_ = -1;
        return;
    }

    selectedIndex_ = std::min(selectedIndex_, static_cast<int>(objects_.size()) - 1);
    selectIndex(selectedIndex_);
}

std::vector<std::unique_ptr<SpaceObject>>& Universe::objects() {
    return objects_;
}

const std::vector<std::unique_ptr<SpaceObject>>& Universe::objects() const {
    return objects_;
}

int Universe::selectedIndex() const {
    return selectedIndex_;
}

void Universe::selectNext() {
    if (objects_.empty()) {
        selectedIndex_ = -1;
        return;
    }
    selectIndex((selectedIndex_ + 1) % static_cast<int>(objects_.size()));
}

void Universe::selectPrevious() {
    if (objects_.empty()) {
        selectedIndex_ = -1;
        return;
    }

    int next = selectedIndex_ - 1;
    if (next < 0) next = static_cast<int>(objects_.size()) - 1;
    selectIndex(next);
}

void Universe::selectIndex(int index) {
    if (objects_.empty()) {
        selectedIndex_ = -1;
        return;
    }

    selectedIndex_ = std::clamp(index, 0, static_cast<int>(objects_.size()) - 1);

    for (int i = 0; i < static_cast<int>(objects_.size()); ++i) {
        objects_[i]->setSelected(i == selectedIndex_);
    }
}

SpaceObject* Universe::selected() {
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(objects_.size())) return nullptr;
    return objects_[selectedIndex_].get();
}

const SpaceObject* Universe::selected() const {
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(objects_.size())) return nullptr;
    return objects_[selectedIndex_].get();
}

ForceState Universe::computeForcesOn(int objectIndex) const {
    ForceState total;

    if (objectIndex < 0 || objectIndex >= static_cast<int>(objects_.size())) return total;

    const SpaceObject& target = *objects_[objectIndex];

    if (!target.isAffectedByGravity()) return total;

    for (int i = 0; i < static_cast<int>(objects_.size()); ++i) {
        if (i == objectIndex) continue;

        const SpaceObject& source = *objects_[i];

        if (!source.isGravityEnabled()) continue;

        const double mu = source.gravitationalParameter();
        if (mu <= 0.0) continue;

        const Vec2 dr = source.position() - target.position();
        const double r2 = norm2(dr) + SOFTENING_M * SOFTENING_M;
        const double r = std::sqrt(r2);

        total.acceleration_m_s2 += (mu / (r2 * r)) * dr;
    }

    return total;
}

void Universe::propagate(double dt_s, PropagationMethod method) {
    const int n = static_cast<int>(objects_.size());

    std::vector<ForceState> forces(n);

    for (int i = 0; i < n; ++i) {
        forces[i] = computeForcesOn(i);
    }

    for (int i = 0; i < n; ++i) {
        objects_[i]->propagateObj(dt_s, method, forces[i]);
    }

    resolveCollisions();

    time_s_ += dt_s;
}

void Universe::resolveCollisions() {
    if (!collisionsEnabled) return;

    const int n = static_cast<int>(objects_.size());

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            SpaceObject& a = *objects_[i];
            SpaceObject& b = *objects_[j];

            if (!a.isCollisionEnabled() || !b.isCollisionEnabled()) continue;
            if (a.radiusM() <= 0.0 || b.radiusM() <= 0.0) continue;

            Vec2 delta = b.position() - a.position();
            double d = norm(delta);
            const double minD = a.radiusM() + b.radiusM();

            if (d >= minD) continue;

            if (d <= 1e-9) {
                delta = {1.0, 0.0};
                d = 1.0;
            }

            const Vec2 nHat = delta / d;
            const double overlap = minD - d;

            const bool aMovable = a.isDynamic();
            const bool bMovable = b.isDynamic();

            if (aMovable && bMovable) {
                a.setPosition(a.position() - 0.5 * overlap * nHat);
                b.setPosition(b.position() + 0.5 * overlap * nHat);
            } else if (aMovable) {
                a.setPosition(a.position() - overlap * nHat);
            } else if (bMovable) {
                b.setPosition(b.position() + overlap * nHat);
            }

            Vec2 relativeVelocity = b.velocity() - a.velocity();
            const double normalSpeed = dot(relativeVelocity, nHat);

            if (normalSpeed >= 0.0) continue;

            const double ma = std::max(1e-9, a.massKg());
            const double mb = std::max(1e-9, b.massKg());

            const double invMa = aMovable ? 1.0 / ma : 0.0;
            const double invMb = bMovable ? 1.0 / mb : 0.0;

            const double denom = invMa + invMb;
            if (denom <= 0.0) continue;

            const double impulse = -(1.0 + RESTITUTION) * normalSpeed / denom;

            if (aMovable) a.setVelocity(a.velocity() - impulse * invMa * nHat);
            if (bMovable) b.setVelocity(b.velocity() + impulse * invMb * nHat);
        }
    }
}

double Universe::timeS() const {
    return time_s_;
}

void Universe::resetTime() {
    time_s_ = 0.0;
}

} // namespace astrodyn