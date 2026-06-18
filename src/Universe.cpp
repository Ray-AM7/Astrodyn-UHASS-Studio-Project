#include "astrodyn/Universe.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <set>

namespace astrodyn {

namespace {

constexpr double SOFTENING_M = 1.0;

Vec2 safeUnit(Vec2 v) {
    const double n = norm(v);
    if (n <= 0.0) return {1.0, 0.0};
    return v / n;
}

Vec2 tangentialCCW(Vec2 radial) {
    return perpendicular_ccw(safeUnit(radial));
}

bool canReceiveBurn(const SpaceObject& obj) {
    return obj.kind() == SpaceObjectKind::SpaceCraft ||
           obj.kind() == SpaceObjectKind::TestParticle;
}

} // namespace

Universe::Universe() = default;

void Universe::clear() {
    objects_.clear();
    selectedIndex_ = -1;
    time_s_ = 0.0;
}

int Universe::addObject(std::unique_ptr<SpaceObject> obj, bool placeRelativeToSelected) {
    if (!obj) return -1;

    if (placeRelativeToSelected) {
        int anchorIndex = selectedIndex_;

        if (anchorIndex < 0 || anchorIndex >= static_cast<int>(objects_.size())) {
            anchorIndex = defaultSpawnAnchorIndex();
        }

        if (anchorIndex >= 0) {
            obj->setOriginBodyIndex(anchorIndex);
            obj->setTargetBodyIndex(anchorIndex);
            placeObjectInCircularOrbitAroundAnchor(*obj, anchorIndex, spawnDistanceFromSelected_m);
        } else {
            // Blank sandbox fallback: random-ish position, zero velocity.
            static std::mt19937 rng(12345);
            std::uniform_real_distribution<double> dist(-1.0e7, 1.0e7);
            obj->setPosition({dist(rng), dist(rng)});
            obj->setVelocity({0.0, 0.0});
            obj->setOriginBodyIndex(-1);
            obj->setTargetBodyIndex(-1);
        }
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

int Universe::defaultSpawnAnchorIndex() const {
    int best = -1;
    double bestMu = -1.0;

    for (int i = 0; i < static_cast<int>(objects_.size()); ++i) {
        const double mu = objects_[i]->gravitationalParameter();
        if (objects_[i]->isGravityEnabled() && mu > bestMu) {
            bestMu = mu;
            best = i;
        }
    }

    return best;
}

void Universe::placeObjectInCircularOrbitAroundAnchor(SpaceObject& obj, int anchorIndex, double radius_m) {
    if (anchorIndex < 0 || anchorIndex >= static_cast<int>(objects_.size())) return;

    const SpaceObject& anchor = *objects_[anchorIndex];

    const double mu = anchor.gravitationalParameter();
    const double safeRadius = std::max(radius_m, anchor.radiusM() + obj.radiusM() + 1.0);

    const Vec2 radial = {safeRadius, 0.0};
    const Vec2 tangential = tangentialCCW(radial);

    double circularSpeed = 0.0;
    if (mu > 0.0 && safeRadius > 0.0) {
        circularSpeed = std::sqrt(mu / safeRadius);
    }

    obj.setPosition(anchor.position() + radial);
    obj.setVelocity(anchor.velocity() + circularSpeed * tangential);
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

    total.acceleration_m_s2 += continuousBurnAccelerationForObject(target);

    return total;
}

std::vector<Vec2> Universe::computeAccelerationsForState(
    const std::vector<Vec2>& positions_m,
    const std::vector<Vec2>& velocities_m_s
) const {
    const int n = static_cast<int>(objects_.size());
    std::vector<Vec2> a(n, {0.0, 0.0});

    for (int i = 0; i < n; ++i) {
        const SpaceObject& target = *objects_[i];

        if (!target.isAffectedByGravity()) {
            a[i] += continuousBurnAccelerationForObject(target);
            continue;
        }

        for (int j = 0; j < n; ++j) {
            if (i == j) continue;

            const SpaceObject& source = *objects_[j];

            if (!source.isGravityEnabled()) continue;

            const double mu = source.gravitationalParameter();
            if (mu <= 0.0) continue;

            const Vec2 dr = positions_m[j] - positions_m[i];
            const double r2 = norm2(dr) + SOFTENING_M * SOFTENING_M;
            const double r = std::sqrt(r2);

            a[i] += (mu / (r2 * r)) * dr;
        }

        // Continuous burn uses current/intermediate velocity direction.
        if (target.continuousBurnEnabled() && canReceiveBurn(target)) {
            const Vec2 vhat = safeUnit(velocities_m_s[i]);
            a[i] += target.continuousBurnAccelerationMps2() * vhat;
        }
    }

    return a;
}

Vec2 Universe::continuousBurnAccelerationForObject(const SpaceObject& obj) const {
    if (!obj.continuousBurnEnabled()) return {0.0, 0.0};
    if (!canReceiveBurn(obj)) return {0.0, 0.0};
    return obj.continuousBurnAccelerationMps2() * safeUnit(obj.velocity());
}

void Universe::writeStateBack(
    const std::vector<Vec2>& positions_m,
    const std::vector<Vec2>& velocities_m_s
) {
    const int n = static_cast<int>(objects_.size());

    for (int i = 0; i < n; ++i) {
        if (!objects_[i]->isDynamic()) continue;

        objects_[i]->setPosition(positions_m[i]);
        objects_[i]->setVelocity(velocities_m_s[i]);
    }
}

void Universe::pushAllTrails() {
    for (auto& obj : objects_) {
        obj->pushTrailPoint();
    }
}

void Universe::propagate(double dt_s, PropagationMethod method) {
    switch (method) {
        case PropagationMethod::SymplecticEuler:
            propagateSymplecticEuler(dt_s);
            break;
        case PropagationMethod::VelocityVerlet:
            propagateVelocityVerlet(dt_s);
            break;
        case PropagationMethod::RK4:
            propagateRK4(dt_s);
            break;
    }

    resolveCollisions();
    time_s_ += dt_s;
    pushAllTrails();
}

void Universe::propagateSymplecticEuler(double dt_s) {
    const int n = static_cast<int>(objects_.size());
    std::vector<Vec2> r(n);
    std::vector<Vec2> v(n);

    for (int i = 0; i < n; ++i) {
        r[i] = objects_[i]->position();
        v[i] = objects_[i]->velocity();
    }

    const auto a = computeAccelerationsForState(r, v);

    for (int i = 0; i < n; ++i) {
        if (!objects_[i]->isDynamic()) continue;
        v[i] += a[i] * dt_s;
        r[i] += v[i] * dt_s;
    }

    writeStateBack(r, v);
}

void Universe::propagateVelocityVerlet(double dt_s) {
    const int n = static_cast<int>(objects_.size());
    std::vector<Vec2> r0(n), v0(n), r1(n), v1(n);

    for (int i = 0; i < n; ++i) {
        r0[i] = objects_[i]->position();
        v0[i] = objects_[i]->velocity();
        r1[i] = r0[i];
        v1[i] = v0[i];
    }

    const auto a0 = computeAccelerationsForState(r0, v0);

    for (int i = 0; i < n; ++i) {
        if (!objects_[i]->isDynamic()) continue;
        r1[i] = r0[i] + v0[i] * dt_s + 0.5 * a0[i] * dt_s * dt_s;
    }

    const auto a1 = computeAccelerationsForState(r1, v0);

    for (int i = 0; i < n; ++i) {
        if (!objects_[i]->isDynamic()) continue;
        v1[i] = v0[i] + 0.5 * (a0[i] + a1[i]) * dt_s;
    }

    writeStateBack(r1, v1);
}

void Universe::propagateRK4(double dt_s) {
    const int n = static_cast<int>(objects_.size());

    std::vector<Vec2> r0(n), v0(n);
    for (int i = 0; i < n; ++i) {
        r0[i] = objects_[i]->position();
        v0[i] = objects_[i]->velocity();
    }

    auto deriv = [&](const std::vector<Vec2>& r, const std::vector<Vec2>& v) {
        std::vector<Vec2> drdt(n), dvdt(n);
        const auto a = computeAccelerationsForState(r, v);

        for (int i = 0; i < n; ++i) {
            if (!objects_[i]->isDynamic()) {
                drdt[i] = {0.0, 0.0};
                dvdt[i] = {0.0, 0.0};
            } else {
                drdt[i] = v[i];
                dvdt[i] = a[i];
            }
        }

        return std::pair<std::vector<Vec2>, std::vector<Vec2>>(drdt, dvdt);
    };

    const auto [k1r, k1v] = deriv(r0, v0);

    std::vector<Vec2> r2(n), v2(n);
    for (int i = 0; i < n; ++i) {
        r2[i] = r0[i] + 0.5 * dt_s * k1r[i];
        v2[i] = v0[i] + 0.5 * dt_s * k1v[i];
    }

    const auto [k2r, k2v] = deriv(r2, v2);

    std::vector<Vec2> r3(n), v3(n);
    for (int i = 0; i < n; ++i) {
        r3[i] = r0[i] + 0.5 * dt_s * k2r[i];
        v3[i] = v0[i] + 0.5 * dt_s * k2v[i];
    }

    const auto [k3r, k3v] = deriv(r3, v3);

    std::vector<Vec2> r4(n), v4(n);
    for (int i = 0; i < n; ++i) {
        r4[i] = r0[i] + dt_s * k3r[i];
        v4[i] = v0[i] + dt_s * k3v[i];
    }

    const auto [k4r, k4v] = deriv(r4, v4);

    std::vector<Vec2> rf(n), vf(n);
    for (int i = 0; i < n; ++i) {
        if (!objects_[i]->isDynamic()) {
            rf[i] = r0[i];
            vf[i] = v0[i];
            continue;
        }

        rf[i] = r0[i] + (dt_s / 6.0) * (k1r[i] + 2.0 * k2r[i] + 2.0 * k3r[i] + k4r[i]);
        vf[i] = v0[i] + (dt_s / 6.0) * (k1v[i] + 2.0 * k2v[i] + 2.0 * k3v[i] + k4v[i]);
    }

    writeStateBack(rf, vf);
}

Vec2 Universe::barycenterPosition() const {
    Vec2 weighted{0.0, 0.0};
    double totalMass = 0.0;

    for (const auto& obj : objects_) {
        if (obj->massKg() <= 0.0) continue;

        // Use physical bodies for the origin. SpaceCraft/TestParticle should not move the displayed origin.
        if (obj->kind() != SpaceObjectKind::LargeBody &&
            obj->kind() != SpaceObjectKind::SmallBody) {
            continue;
        }

        weighted += obj->massKg() * obj->position();
        totalMass += obj->massKg();
    }

    if (totalMass <= 0.0) return {0.0, 0.0};
    return weighted / totalMass;
}

Vec2 Universe::barycenterVelocity() const {
    Vec2 weighted{0.0, 0.0};
    double totalMass = 0.0;

    for (const auto& obj : objects_) {
        if (obj->massKg() <= 0.0) continue;

        if (obj->kind() != SpaceObjectKind::LargeBody &&
            obj->kind() != SpaceObjectKind::SmallBody) {
            continue;
        }

        weighted += obj->massKg() * obj->velocity();
        totalMass += obj->massKg();
    }

    if (totalMass <= 0.0) return {0.0, 0.0};
    return weighted / totalMass;
}

int Universe::strongestGravitySourceIndexFor(int objectIndex) const {
    if (objectIndex < 0 || objectIndex >= static_cast<int>(objects_.size())) return -1;
    return strongestGravitySourceIndexAt(objects_[objectIndex]->position());
}

int Universe::strongestGravitySourceIndexAt(Vec2 position_m) const {
    int best = -1;
    double bestAccel = -1.0;

    for (int i = 0; i < static_cast<int>(objects_.size()); ++i) {
        const SpaceObject& src = *objects_[i];
        if (!src.isGravityEnabled()) continue;

        const double mu = src.gravitationalParameter();
        if (mu <= 0.0) continue;

        const double r2 = norm2(src.position() - position_m) + SOFTENING_M * SOFTENING_M;
        const double accel = mu / r2;

        if (accel > bestAccel) {
            bestAccel = accel;
            best = i;
        }
    }

    return best;
}

RelativeKinematics2D Universe::relativeToBarycenter(int objectIndex) const {
    if (objectIndex < 0 || objectIndex >= static_cast<int>(objects_.size())) return {};

    const Vec2 originR = barycenterPosition();
    const Vec2 originV = barycenterVelocity();

    const SpaceObject& obj = *objects_[objectIndex];

    const Vec2 relR = obj.position() - originR;
    const Vec2 relV = obj.velocity() - originV;

    RelativeKinematics2D k;
    k.velocityVector_m_s = relV;
    k.speed_m_s = norm(relV);

    const Vec2 rHat = safeUnit(relR);
    k.radialSpeed_m_s = dot(relV, rHat);
    k.tangentialVelocityVector_m_s = relV - k.radialSpeed_m_s * rHat;
    k.tangentialSpeed_m_s = norm(k.tangentialVelocityVector_m_s);

    return k;
}

RelativeKinematics2D Universe::relativeToObject(int objectIndex, int referenceIndex) const {
    if (objectIndex < 0 || objectIndex >= static_cast<int>(objects_.size())) return {};
    if (referenceIndex < 0 || referenceIndex >= static_cast<int>(objects_.size())) return {};

    const SpaceObject& obj = *objects_[objectIndex];
    const SpaceObject& ref = *objects_[referenceIndex];

    const Vec2 relR = obj.position() - ref.position();
    const Vec2 relV = obj.velocity() - ref.velocity();

    RelativeKinematics2D k;
    k.velocityVector_m_s = relV;
    k.speed_m_s = norm(relV);

    const Vec2 rHat = safeUnit(relR);
    k.radialSpeed_m_s = dot(relV, rHat);
    k.tangentialVelocityVector_m_s = relV - k.radialSpeed_m_s * rHat;
    k.tangentialSpeed_m_s = norm(k.tangentialVelocityVector_m_s);

    return k;
}

RelativeKinematics2D Universe::relativeToStrongestGravitySource(int objectIndex) const {
    const int ref = strongestGravitySourceIndexFor(objectIndex);
    return relativeToObject(objectIndex, ref);
}

bool Universe::applyImpulsiveBurnToSelected(double deltaV_m_s, bool prograde) {
    SpaceObject* obj = selected();
    if (!obj) return false;
    if (!canReceiveBurn(*obj)) return false;

    Vec2 vhat = safeUnit(obj->velocity());
    if (!prograde) vhat = -1.0 * vhat;

    obj->setVelocity(obj->velocity() + deltaV_m_s * vhat);
    return true;
}

bool Universe::toggleContinuousBurnForSelected() {
    SpaceObject* obj = selected();
    if (!obj) return false;
    if (!canReceiveBurn(*obj)) return false;

    obj->setContinuousBurnEnabled(!obj->continuousBurnEnabled());

    if (obj->continuousBurnAccelerationMps2() <= 0.0) {
        obj->setContinuousBurnAccelerationMps2(0.001);
    }

    return true;
}

bool Universe::setSelectedContinuousBurnAcceleration(double accel_m_s2) {
    SpaceObject* obj = selected();
    if (!obj) return false;
    if (!canReceiveBurn(*obj)) return false;

    obj->setContinuousBurnAccelerationMps2(accel_m_s2);
    return true;
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
                const double mBig = std::max(1e-9, big->massKg());
                const double mSmall = std::max(1e-9, small->massKg());
                const double mNew = mBig + mSmall;

                const Vec2 vNew = (mBig * big->velocity() + mSmall * small->velocity()) / mNew;

                const double rNew = std::cbrt(
                    big->radiusM() * big->radiusM() * big->radiusM()
                    + small->radiusM() * small->radiusM() * small->radiusM()
                );

                big->setVelocity(vNew);
                big->setMassKg(mNew);
                big->setRadiusM(std::max(big->radiusM(), rNew));

                if (selectedIndex_ == smallIndex) {
                    selectedIndex_ = bigIndex;
                    big->setSelected(true);
                }

                removeIndices.insert(smallIndex);
            } else {
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