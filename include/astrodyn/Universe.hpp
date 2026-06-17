#pragma once

#include "astrodyn/LargeBody.hpp"
#include "astrodyn/SmallBody.hpp"
#include "astrodyn/SpaceCraft.hpp"
#include "astrodyn/TestParticle.hpp"
#include <memory>
#include <string>
#include <vector>

namespace astrodyn {

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
    ForceState computeForcesOn(int objectIndex) const;

    void resolveCollisions();

    double timeS() const;
    void resetTime();

    bool collisionsEnabled = true;
    bool paused = false;

    double spawnDistanceFromSelected_m = 1.0e7;

private:
    std::vector<std::unique_ptr<SpaceObject>> objects_;
    int selectedIndex_ = -1;
    double time_s_ = 0.0;
};

} // namespace astrodyn