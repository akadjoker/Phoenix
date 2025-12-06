#include "pch.h"
#include "Math.hpp"
#include "Steering.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Mesh.hpp"

SteeringComponent::SteeringComponent()
    : m_velocity(0, 0, 0), m_steeringForce(0, 0, 0)
{
}

const char *SteeringComponent::getTypeName() const
{
    return "SteeringComponent";
}

// ==================== Properties ====================

void SteeringComponent::setMaxSpeed(float speed)
{
    m_maxSpeed = speed;
}

float SteeringComponent::getMaxSpeed() const
{
    return m_maxSpeed;
}

void SteeringComponent::setMaxForce(float force)
{
    m_maxForce = force;
}

float SteeringComponent::getMaxForce() const
{
    return m_maxForce;
}

void SteeringComponent::setMass(float mass)
{
    m_mass = mass;
}

float SteeringComponent::getMass() const
{
    return m_mass;
}

void SteeringComponent::setAutoRotate(bool enabled)
{
    m_autoRotate = enabled;
}

bool SteeringComponent::isAutoRotate() const
{
    return m_autoRotate;
}

void SteeringComponent::setLockY(bool lock)
{
    m_lockY = lock;
}

bool SteeringComponent::isLockY() const
{
    return m_lockY;
}

Vec3 SteeringComponent::getVelocity() const
{
    return m_velocity;
}

void SteeringComponent::setVelocity(const Vec3 &vel)
{
    m_velocity = vel;
}

Vec3 SteeringComponent::getSteeringForce() const
{
    return m_steeringForce;
}

// ==================== Force Accumulation ====================

void SteeringComponent::clearForces()
{
    m_steeringForce = Vec3::Zero;
}

void SteeringComponent::addForce(const Vec3 &force, float weight)
{
    m_steeringForce += force * weight;
}

// ==================== Basic Behaviors ====================

Vec3 SteeringComponent::seek(const Vec3 &target, float maxSpeed)
{
    if (maxSpeed < 0)
        maxSpeed = m_maxSpeed;

    Vec3 position = getOwner()->getPosition(TransformSpace::World);
    Vec3 desired = (target - position);
    float distance = desired.length();

    if (distance < 0.001f)
        return Vec3(0, 0, 0);

    desired = desired.normalized() * maxSpeed;
    return desired - m_velocity; // Steering = desired - current
}

Vec3 SteeringComponent::flee(const Vec3 &threat, float panicDistance, float maxSpeed)
{
    if (maxSpeed < 0)
        maxSpeed = m_maxSpeed;

    Vec3 position = getOwner()->getPosition(TransformSpace::World);
    Vec3 toThreat = threat - position;
    float distance = toThreat.length();

    if (distance > panicDistance)
        return Vec3(0, 0, 0);

    Vec3 desired = (position - threat).normalized() * maxSpeed;

    // Stronger flee when closer
    float panicFactor = 1.0f - (distance / panicDistance);
    desired = desired * (0.5f + panicFactor * 0.5f);

    return desired - m_velocity;
}

Vec3 SteeringComponent::arrive(const Vec3 &target, float slowingRadius, float maxSpeed)
{
    if (maxSpeed < 0)
        maxSpeed = m_maxSpeed;

    Vec3 position = getOwner()->getPosition(TransformSpace::World);
    Vec3 toTarget = target - position;
    float distance = toTarget.length();

    if (distance < 0.001f)
        return Vec3(0, 0, 0);

    float speed = maxSpeed;
    if (distance < slowingRadius)
    {
        speed = maxSpeed * (distance / slowingRadius);
    }

    Vec3 desired = toTarget.normalized() * speed;
    return desired - m_velocity;
}

// ==================== Advanced Behaviors ====================

Vec3 SteeringComponent::pursue(GameObject *target, float predictionTime, float maxSpeed)
{
    if (!target)
        return Vec3(0, 0, 0);
    if (maxSpeed < 0)
        maxSpeed = m_maxSpeed;

    Vec3 targetPos = target->getPosition(TransformSpace::World);

    // Estimate target velocity from its forward direction
    Vec3 targetVelocity = target->getForward(TransformSpace::World) * (maxSpeed * 0.7f);

    // Check if target has steering component
    auto *targetSteering = target->getComponent<SteeringComponent>();
    if (targetSteering)
    {
        targetVelocity = targetSteering->getVelocity();
    }

    Vec3 futurePos = targetPos + targetVelocity * predictionTime;
    return seek(futurePos, maxSpeed);
}

Vec3 SteeringComponent::evade(GameObject *threat, float predictionTime,
                              float panicDistance, float maxSpeed)
{
    if (!threat)
        return Vec3(0, 0, 0);
    if (maxSpeed < 0)
        maxSpeed = m_maxSpeed;

    Vec3 threatPos = threat->getPosition(TransformSpace::World);
    Vec3 threatVelocity = threat->getForward(TransformSpace::World) * (maxSpeed * 0.7f);

    auto *threatSteering = threat->getComponent<SteeringComponent>();
    if (threatSteering)
    {
        threatVelocity = threatSteering->getVelocity();
    }

    Vec3 futurePos = threatPos + threatVelocity * predictionTime;
    return flee(futurePos, panicDistance, maxSpeed);
}

Vec3 SteeringComponent::wander(float wanderRadius, float wanderDistance,
                               float wanderJitter, float maxSpeed)
{
    if (maxSpeed < 0)
        maxSpeed = m_maxSpeed;

    // Update wander angle with jitter
    m_wanderAngle += (((float)rand() / RAND_MAX) - 0.5f) * wanderJitter;

    Vec3 forward = getOwner()->getForward(TransformSpace::World);
    Vec3 circleCenter = forward * wanderDistance;

    // Calculate displacement on wander circle
    Vec3 right = getOwner()->getRight(TransformSpace::World);
    Vec3 up = getOwner()->getUp(TransformSpace::World);

    Vec3 displacement = right * (std::cos(m_wanderAngle) * wanderRadius) +
                        up * (std::sin(m_wanderAngle) * wanderRadius);

    if (m_lockY)
        displacement.y = 0;

    Vec3 wanderTarget = circleCenter + displacement;
    Vec3 desired = wanderTarget.normalized() * maxSpeed;

    return desired - m_velocity;
}

Vec3 SteeringComponent::circleStrafe(const Vec3 &target, float orbitRadius,
                                     float orbitSpeed, bool clockwise)
{
    Vec3 position = getOwner()->getPosition(TransformSpace::World);
    Vec3 toTarget = target - position;
    float distance = toTarget.length();

    // Tangent for circular motion (perpendicular to radius)
    Vec3 tangent(-toTarget.z, 0, toTarget.x);
    if (tangent.length() > 0.001f)
    {
        tangent = tangent.normalized();
        if (!clockwise)
            tangent = tangent * -1.0f;
    }

    // Radial correction to maintain orbit distance
    Vec3 radial(0, 0, 0);
    if (distance > orbitRadius + 1.0f)
        radial = toTarget.normalized() * 2.0f;
    else if (distance < orbitRadius - 1.0f)
        radial = toTarget.normalized() * -2.0f;

    Vec3 desired = tangent * orbitSpeed + radial;
    return desired - m_velocity;
}

// ==================== Obstacle Avoidance ====================

Vec3 SteeringComponent::avoidObstacles(const std::vector<GameObject *> &obstacles,
                                       float lookAheadDistance,
                                       float avoidanceForce)
{
    Vec3 position = getOwner()->getPosition(TransformSpace::World);
    Vec3 forward = getOwner()->getForward(TransformSpace::World);
    Vec3 right = getOwner()->getRight(TransformSpace::World);

    Vec3 avoidance(0, 0, 0);

    for (auto *obstacle : obstacles)
    {
        if (!obstacle || obstacle == getOwner())
            continue;

        Vec3 obstaclePos = obstacle->getPosition(TransformSpace::World);
        Vec3 toObstacle = obstaclePos - position;

        // Check if obstacle is ahead
        float dot = forward.dot(toObstacle.normalized());
        if (dot < 0.3f)
            continue;

        float distance = toObstacle.length();
        if (distance > lookAheadDistance)
            continue;

        // Get obstacle size
        BoundingBox bbox = obstacle->getTransformedBoundingBox();
        float obstacleRadius = (bbox.max - bbox.min).length() * 0.5f;
        float minDistance = obstacleRadius + 2.0f;

        if (distance < minDistance)
        {
            // Determine which side to avoid
            float side = right.dot(toObstacle);

            // Stronger avoidance when closer
            float urgency = 1.0f - (distance / minDistance);
            float force = avoidanceForce * urgency;

            avoidance += right * (side > 0 ? -force : force);
        }
    }

    return avoidance;
}

// ==================== Group Behaviors ====================

Vec3 SteeringComponent::separate(const std::vector<GameObject *> &neighbors,
                                 float separationRadius, float separationForce)
{
    Vec3 position = getOwner()->getPosition(TransformSpace::World);
    Vec3 steer(0, 0, 0);
    int count = 0;

    for (auto *neighbor : neighbors)
    {
        if (!neighbor || neighbor == getOwner())
            continue;

        Vec3 neighborPos = neighbor->getPosition(TransformSpace::World);
        float distance = (position - neighborPos).length();

        if (distance > 0.001f && distance < separationRadius)
        {
            Vec3 diff = (position - neighborPos).normalized();
            diff = diff / distance; // Weight by distance
            steer += diff;
            count++;
        }
    }

    if (count > 0)
    {
        steer = (steer / (float)count).normalized() * separationForce;
    }

    return steer;
}

Vec3 SteeringComponent::cohesion(const std::vector<GameObject *> &neighbors,
                                 float neighborRadius, float maxSpeed)
{
    if (maxSpeed < 0)
        maxSpeed = m_maxSpeed;

    Vec3 position = getOwner()->getPosition(TransformSpace::World);
    Vec3 center(0, 0, 0);
    int count = 0;

    for (auto *neighbor : neighbors)
    {
        if (!neighbor || neighbor == getOwner())
            continue;

        Vec3 neighborPos = neighbor->getPosition(TransformSpace::World);
        float distance = (position - neighborPos).length();

        if (distance < neighborRadius)
        {
            center += neighborPos;
            count++;
        }
    }

    if (count > 0)
    {
        center = center / (float)count;
        return seek(center, maxSpeed);
    }

    return Vec3(0, 0, 0);
}

// ==================== Update ====================

void SteeringComponent::update(float deltaTime)
{
    if (!isEnabled())
        return;

    // Limit steering force
    float forceMagnitude = m_steeringForce.length();
    if (forceMagnitude > m_maxForce)
    {
        m_steeringForce = (m_steeringForce / forceMagnitude) * m_maxForce;
    }

    // Apply force: F = ma, a = F/m
    Vec3 acceleration = m_steeringForce / m_mass;
    m_velocity += acceleration * deltaTime;

    // Lock Y if needed
    if (m_lockY)
    {
        m_velocity.y = 0;
    }

    // Limit speed
    float speed = m_velocity.length();
    if (speed > m_maxSpeed)
    {
        m_velocity = (m_velocity / speed) * m_maxSpeed;
    }

    // Apply movement
    if (speed > 0.01f)
    {
        getOwner()->translate(m_velocity * deltaTime, TransformSpace::World);

        // Auto-rotate to face movement direction
        if (m_autoRotate && speed > 0.1f)
        {
            Vec3 forward = m_velocity.normalized();
            if (m_lockY)
                forward.y = 0;

            if (forward.length() > 0.01f)
            {
                Vec3 currentPos = getOwner()->getPosition(TransformSpace::World);
                getOwner()->lookAt(currentPos + forward, TransformSpace::World, Vec3(0, 1, 0));
            }
        }
    }

    // Clear forces for next frame
    clearForces();
}

// void SteeringComponent::debugDraw()
// {
//     GameObject* owner = getOwner();
//     Vec3 pos = owner->getPosition(TransformSpace::World);

//     // Velocidade
//     if (m_velocity.length() > 0.001f)
//         DebugDraw::Line(pos, pos + m_velocity, Color::Green);

//     // Steering force
//     if (m_steeringForce.length() > 0.001f)
//         DebugDraw::Line(pos, pos + m_steeringForce, Color::Yellow);

//     // Wander circle (opcional)
//     Vec3 forward = owner->getForward(TransformSpace::World);
//     Vec3 circleCenter = pos + forward * 5.0f; // usa wanderDistance
//     DebugDraw::Sphere(circleCenter, 2.0f, Color::Cyan); // wanderRadius

//     // Velocidade normalizada (direção)
//     Vec3 dir = m_velocity.normalize();
//     DebugDraw::Line(pos, pos + dir * 2.0f, Color::Blue);
// }
