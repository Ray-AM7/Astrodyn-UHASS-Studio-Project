#include "astrodyn/Universe.hpp"
#include <algorithm>
#include <cmath>
#include <set>

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

int Universe::addObject(std::unique_ptr<SpaceObject> obj, bool placeRelativeToSelected) {
    if (!obj) return -1;

    if (placeRelativeToSelected &&
        selectedIndex_ >= 0 &&
        selectedIndex_ < static_cast<int>(objects_.size())) {
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
    std::set<int> removeIndices;

    for (int i = 0; i < n; ++i) {
        if (removeIndices.count(i)) continue;

        for (int j = i + 1; j < n; ++j) {
            if (removeIndices.count(j)) continue;

            SpaceObject& a = *objects_[i];
            SpaceObject& b = *objects_[j];

            if (!a.isCollisionEnabled() || !b.isCollisionEnabled()) continue;
            if (a.radiusM() <= 0.0 || b.radiusM() <= 0.0) continue;

            Vec2 delta = b.position() - a.position();
            double d = norm(delta);
            const double minD = a.radiusM() + b.radiusM();

            if (d > minD) continue;

            if (d <= 1e-9) {
                delta = {1.0, 0.0};
                d = 1.0;
            }

            const Vec2 nHat = delta / d;

            SpaceObject* big = &a;
            SpaceObject* small = &b;
            int bigIndex = i;
            int smallIndex = j;

            if (b.massKg() > a.massKg()) {
                big = &b;
                small = &a;
                bigIndex = j;
                smallIndex = i;
            }

            const double massRatio = big->massKg() / std::max(1e-9, small->massKg());
            const double radiusRatio = big->radiusM() / std::max(1e-9, small->radiusM());

            const bool veryDifferent = (massRatio > 100.0 || radiusRatio > 20.0);

            if (veryDifferent) {
                // Stick/merge smaller object into larger body.
                // Conservation of linear momentum:
                const double mBig = std::max(1e-9, big->massKg());
                const double mSmall = std::max(1e-9, small->massKg());
                const double mNew = mBig + mSmall;

                const Vec2 vNew = (mBig * big->velocity() + mSmall * small->velocity()) / mNew;

                // Approximate volume-equivalent radius if densities are comparable.
                const double rNew = std::cbrt(
                    big->radiusM() * big->radiusM() * big->radiusM()
                    + small->radiusM() * small->radiusM() * small->radiusM()
                );

                big->setVelocity(vNew);
                big->setMassKg(mNew);
                big->setRadiusM(std::max(big->radiusM(), rNew));

                // Keep the bigger object selected if selected object was swallowed.
                if (selectedIndex_ == smallIndex) {
                    selectedIndex_ = bigIndex;
                    big->setSelected(true);
                }

                removeIndices.insert(smallIndex);
            } else {
                // Comparable bodies: perfectly elastic collision along contact normal.
                const bool aMovable = a.isDynamic();
                const bool bMovable = b.isDynamic();

                const double ma = std::max(1e-9, a.massKg());
                const double mb = std::max(1e-9, b.massKg());

                const Vec2 va = a.velocity();
                const Vec2 vb = b.velocity();

                const double vaN = dot(va, nHat);
                const double vbN = dot(vb, nHat);

                const double vaNNew = (vaN * (ma - mb) + 2.0 * mb * vbN) / (ma + mb);
                const double vbNNew = (vbN * (mb - ma) + 2.0 * ma * vaN) / (ma + mb);

                if (aMovable) a.setVelocity(va + (vaNNew - vaN) * nHat);
                if (bMovable) b.setVelocity(vb + (vbNNew - vbN) * nHat);

                // Small positional correction only to prevent repeated overlap.
                const double overlap = minD - d;
                if (overlap > 0.0) {
                    if (aMovable && bMovable) {
                        a.setPosition(a.position() - 0.5 * overlap * nHat);
                        b.setPosition(b.position() + 0.5 * overlap * nHat);
                    } else if (aMovable) {
                        a.setPosition(a.position() - overlap * nHat);
                    } else if (bMovable) {
                        b.setPosition(b.position() + overlap * nHat);
                    }
                }
            }
        }
    }

    if (!removeIndices.empty()) {
        std::vector<std::unique_ptr<SpaceObject>> kept;
        kept.reserve(objects_.size());

        for (int i = 0; i < static_cast<int>(objects_.size()); ++i) {
            if (!removeIndices.count(i)) kept.push_back(std::move(objects_[i]));
        }

        objects_ = std::move(kept);

        if (objects_.empty()) {
            selectedIndex_ = -1;
        } else {
            selectedIndex_ = std::clamp(selectedIndex_, 0, static_cast<int>(objects_.size()) - 1);
            selectIndex(selectedIndex_);
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