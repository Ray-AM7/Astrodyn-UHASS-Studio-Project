#pragma once

#include "astrodyn/LargeBody.hpp"
#include "astrodyn/SmallBody.hpp"
#include "astrodyn/SpaceCraft.hpp"
#include "astrodyn/TestParticle.hpp"
#include <memory>
#include <string>
#include <vector>

namespace astrodyn {

struct RelativeKinematics2D {
    Vec2 velocityVector_m_s{0.0, 0.0};
    double speed_m_s = 0.0;
    double radialSpeed_m_s = 0.0;
    double tangentialSpeed_m_s = 0.0;
    Vec2 tangentialVelocityVector_m_s{0.0, 0.0};
};

class Universe {
public:
    Universe();

    void clear();

    int addObject(std::unique_ptr<SpaceObject> obj, bool placeRelativeToSelected = true);
    int addLargeBody();
    int addSmallBody();
    int addSpaceCraft();
    int addTestParticle();

    void removeSelected();

    std::vector<std::unique_ptr<SpaceObject>>& objects();
    const std::vector<std::unique_ptr<SpaceObject>>& objects() const;

    int selectedIndex() const;
    void selectNext();
    void selectPrevious();
    void selectIndex(int index);

    SpaceObject* selected();
    const SpaceObject* selected() const;

    void propagate(double dt_s, PropagationMethod method);

    // Legacy/simple force API still kept for object interface compatibility.
    ForceState computeForcesOn(int objectIndex) const;

    // True universe-level integrators.
    void propagateSymplecticEuler(double dt_s);
    void propagateVelocityVerlet(double dt_s);
    void propagateRK4(double dt_s);

    void resolveCollisions();

    double timeS() const;
    void resetTime();

    // Origin/barycenter.
    Vec2 barycenterPosition() const;
    Vec2 barycenterVelocity() const;

    // Strongest gravitational influence on objectIndex.
    int strongestGravitySourceIndexFor(int objectIndex) const;
    int strongestGravitySourceIndexAt(Vec2 position_m) const;

    // Relative velocity helpers.
    RelativeKinematics2D relativeToBarycenter(int objectIndex) const;
    RelativeKinematics2D relativeToObject(int objectIndex, int referenceIndex) const;
    RelativeKinematics2D relativeToStrongestGravitySource(int objectIndex) const;

    // Burns for SpaceCraft and TestParticle.
    bool applyImpulsiveBurnToSelected(double deltaV_m_s, bool prograde);
    bool toggleContinuousBurnForSelected();
    bool setSelectedContinuousBurnAcceleration(double accel_m_s2);

    bool collisionsEnabled = true;
    bool paused = false;

    double spawnDistanceFromSelected_m = 1.0e7;

private:
    std::vector<Vec2> computeAccelerationsForState(
        const std::vector<Vec2>& positions_m,
        const std::vector<Vec2>& velocities_m_s
    ) const;

    Vec2 continuousBurnAccelerationForObject(const SpaceObject& obj) const;

    void writeStateBack(
        const std::vector<Vec2>& positions_m,
        const std::vector<Vec2>& velocities_m_s
    );

    void pushAllTrails();

    int defaultSpawnAnchorIndex() const;
    void placeObjectInCircularOrbitAroundAnchor(SpaceObject& obj, int anchorIndex, double radius_m);

private:
    std::vector<std::unique_ptr<SpaceObject>> objects_;
    int selectedIndex_ = -1;
    double time_s_ = 0.0;
};

} // namespace astrodyn