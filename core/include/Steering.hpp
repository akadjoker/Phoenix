#pragma once

#include "Component.hpp"
#include "Math.hpp"
#include <vector>

class SteeringComponent : public Component
{
private:
    Vec3 m_velocity;
    Vec3 m_steeringForce;
    
    float m_maxSpeed = 10.0f;
    float m_maxForce = 20.0f;
    float m_mass = 1.0f;
    
    bool m_autoRotate = true;
    bool m_lockY = true; // Lock vertical movement
    
    // Wander state
    float m_wanderAngle = 0.0f;

public:
    SteeringComponent();

    const char* getTypeName() const override;

    // ==================== Properties ====================

    void setMaxSpeed(float speed);
    float getMaxSpeed() const;

    void setMaxForce(float force);
    float getMaxForce() const;

    void setMass(float mass);
    float getMass() const;

    void setAutoRotate(bool enabled);
    bool isAutoRotate() const;

    void setLockY(bool lock);
    bool isLockY() const;

    Vec3 getVelocity() const;
    void setVelocity(const Vec3& vel);

    Vec3 getSteeringForce() const;

    // ==================== Force Accumulation ====================

    void clearForces();
    void addForce(const Vec3& force, float weight = 1.0f);

    // ==================== Basic Behaviors ====================

    Vec3 seek(const Vec3& target, float maxSpeed = -1.0f);
    Vec3 flee(const Vec3& threat, float panicDistance = 20.0f, float maxSpeed = -1.0f);
    Vec3 arrive(const Vec3& target, float slowingRadius = 5.0f, float maxSpeed = -1.0f);

    // ==================== Advanced Behaviors ====================

    Vec3 pursue(GameObject* target, float predictionTime = 1.0f, float maxSpeed = -1.0f);
    Vec3 evade(GameObject* threat, float predictionTime = 1.0f,
               float panicDistance = 25.0f, float maxSpeed = -1.0f);

    Vec3 wander(float wanderRadius = 2.0f, float wanderDistance = 5.0f,
                float wanderJitter = 0.5f, float maxSpeed = -1.0f);

    Vec3 circleStrafe(const Vec3& target, float orbitRadius = 15.0f,
                      float orbitSpeed = 5.0f, bool clockwise = true);

    // ==================== Obstacle Avoidance ====================

    Vec3 avoidObstacles(const std::vector<GameObject*>& obstacles,
                        float lookAheadDistance = 10.0f,
                        float avoidanceForce = 15.0f);

    // ==================== Group Behaviors ====================

    Vec3 separate(const std::vector<GameObject*>& neighbors,
                  float separationRadius = 5.0f, float separationForce = 10.0f);

    Vec3 cohesion(const std::vector<GameObject*>& neighbors,
                  float neighborRadius = 15.0f, float maxSpeed = -1.0f);

    // ==================== Update ====================

    void update(float deltaTime) override;
};

// ==================== Example Usage ====================

/*

// ===== SETUP =====

GameObject* tank = new GameObject("Tank");
auto* steering = tank->addComponent<SteeringComponent>();
steering->setMaxSpeed(10.0f);
steering->setMaxForce(20.0f);

// ===== SIMPLE SEEK =====

void update(float dt)
{
    auto* steering = tank->getComponent<SteeringComponent>();
    
    Vec3 seekForce = steering->seek(targetPosition);
    steering->addForce(seekForce);
}

// ===== COMBINE MULTIPLE BEHAVIORS =====

void update(float dt)
{
    auto* steering = tank->getComponent<SteeringComponent>();
    
    // Seek player with high priority
    Vec3 seekForce = steering->seek(playerPos);
    steering->addForce(seekForce, 1.0f);
    
    // Avoid obstacles with higher priority
    Vec3 avoidForce = steering->avoidObstacles(obstacles, 15.0f, 20.0f);
    steering->addForce(avoidForce, 2.5f);
    
    // Separate from other tanks
    Vec3 separateForce = steering->separate(otherTanks, 5.0f, 10.0f);
    steering->addForce(separateForce, 1.5f);
}

// ===== PATROL BEHAVIOR =====

void patrolUpdate(float dt)
{
    auto* steering = tank->getComponent<SteeringComponent>();
    
    Vec3 wanderForce = steering->wander(3.0f, 5.0f, 0.5f);
    steering->addForce(wanderForce);
    
    Vec3 avoidForce = steering->avoidObstacles(obstacles);
    steering->addForce(avoidForce, 2.0f);
}

// ===== ATTACK BEHAVIOR =====

void attackUpdate(float dt)
{
    auto* steering = tank->getComponent<SteeringComponent>();
    
    // Circle strafe around player
    Vec3 strafeForce = steering->circleStrafe(playerPos, 20.0f, 6.0f, true);
    steering->addForce(strafeForce, 1.0f);
    
    // Avoid obstacles
    Vec3 avoidForce = steering->avoidObstacles(obstacles);
    steering->addForce(avoidForce, 2.5f);
}

// ===== RETREAT BEHAVIOR =====

void retreatUpdate(float dt)
{
    auto* steering = tank->getComponent<SteeringComponent>();
    
    // Flee from player
    Vec3 fleeForce = steering->flee(playerPos, 30.0f);
    steering->addForce(fleeForce, 1.5f);
    
    // Avoid obstacles while fleeing
    Vec3 avoidForce = steering->avoidObstacles(obstacles);
    steering->addForce(avoidForce, 2.0f);
}

*/
