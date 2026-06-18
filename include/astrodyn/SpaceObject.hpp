#pragma once

#include "astrodyn/StartPhysics.hpp"
#include <memory>
#include <string>
#include <vector>

namespace astrodyn {

class SpaceObject : public StartPhysics {
public:
    SpaceObject(
        std::string name,
        SpaceObjectKind kind,
        Vec2 position_m = {0.0, 0.0},
        Vec2 velocity_m_s = {0.0, 0.0}
    );

    virtual ~SpaceObject() = default;

    virtual std::unique_ptr<SpaceObject> clone() const = 0;

    const std::string& name() const;
    void setName(const std::string& name);

    SpaceObjectKind kind() const;

    Vec2 position() const;
    Vec2 velocity() const;
    Vec2 momentum() const;

    void setPosition(Vec2 r_m);
    void setVelocity(Vec2 v_m_s);
    void setMomentum(Vec2 p_kg_m_s);

    virtual double massKg() const;
    virtual double radiusM() const;
    virtual double gravitationalParameter() const;

    virtual void setMassKg(double mass_kg);
    virtual void setRadiusM(double radius_m);

    // Spin angular momentum placeholder for 2D.
    // In real 3D this becomes Vec3.
    Vec2 spinAngularMomentum() const;
    void setSpinAngularMomentum(Vec2 h_spin);

    // Orbital angular momentum about a chosen origin.
    // In 2D this is the z-component of r x p.
    double orbitalAngularMomentumZ(Vec2 origin_m = {0.0, 0.0}) const;

    bool isSelected() const;
    void setSelected(bool selected);

    bool isDynamic() const;
    void setDynamic(bool dynamic);

    bool isGravityEnabled() const;
    void setGravityEnabled(bool enabled);

    bool isAffectedByGravity() const;
    void setAffectedByGravity(bool affected);

    bool isCollisionEnabled() const;
    void setCollisionEnabled(bool enabled);

    bool isTrailEnabled() const;
    void setTrailEnabled(bool enabled);

    float drawRadiusPx() const;
    void setDrawRadiusPx(float r);

    int colorR() const;
    int colorG() const;
    int colorB() const;
    void setColor(int r, int g, int b);

    const std::vector<Vec2>& trail() const;
    void clearTrail();
    void pushTrailPoint();

    // Creation/mission metadata.
    int originBodyIndex() const;
    void setOriginBodyIndex(int index);

    int targetBodyIndex() const;
    void setTargetBodyIndex(int index);

    // Continuous burn support for SpaceCraft and TestParticle.
    bool continuousBurnEnabled() const;
    void setContinuousBurnEnabled(bool enabled);

    double continuousBurnAccelerationMps2() const;
    void setContinuousBurnAccelerationMps2(double accel_m_s2);

    // Interface requirement.
    bool editObject(const std::string& variableName, double newValue) override;

    // Still implemented for interface compliance, but Universe now does real N-body propagation.
    void propagateObj(
        double timestep_s,
        PropagationMethod method,
        const ForceState& forces
    ) override;

protected:
    void propagateSymplecticEuler(double dt_s, const ForceState& forces);
    void propagateVelocityVerletLike(double dt_s, const ForceState& forces);
    void propagateRK4Simple(double dt_s, const ForceState& forces);

protected:
    std::string name_;
    SpaceObjectKind kind_;

    Vec2 r_m_;
    Vec2 v_m_s_;

    double mass_kg_ = 0.0;
    double radius_m_ = 0.0;

    Vec2 spinAngularMomentum_kg_m2_s_{0.0, 0.0};

    bool selected_ = false;
    bool dynamic_ = true;
    bool gravityEnabled_ = false;
    bool affectedByGravity_ = true;
    bool collisionEnabled_ = true;
    bool trailEnabled_ = true;

    int originBodyIndex_ = -1;
    int targetBodyIndex_ = -1;

    bool continuousBurnEnabled_ = false;
    double continuousBurnAcceleration_m_s2_ = 0.0;

    float drawRadiusPx_ = 5.0f;
    int colorR_ = 255;
    int colorG_ = 255;
    int colorB_ = 255;

    std::vector<Vec2> trail_m_;
};

} // namespace astrodyn