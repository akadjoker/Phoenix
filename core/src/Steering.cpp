#include "pch.h"
#include "Math.hpp"
#include "Steering.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Utils.hpp"

bool SteeringComponent::computeDesiredOrientation(Quat& outRotation, Vec3& outForward) const
{
    // Se não estamos a mover, não há orientação "útil"
    Vec3 vel = m_velocity;
    if (m_lockY)
        vel.y = 0.0f;

    float speed = vel.length();
    if (speed < 0.001f)
        return false;

    Vec3 forward = vel / speed; // normalized

    // Forward final (sem roll) em world space
    outForward = forward;

    // Base "up" global
    Vec3 up(0.0f, 1.0f, 0.0f);

    // Evitar caso degenerado
    if (fabs(Vec3::Dot(up, forward)) > 0.999f)
    {
        // Se estiver quase colado, escolhe outro up qualquer
        up = Vec3(1.0f, 0.0f, 0.0f);
    }

    // Right e up ortogonais
    Vec3 right = Vec3::Cross(up, forward).normalized();
    Vec3 newUp = Vec3::Cross(forward, right).normalized();

    // Aplica roll (banking) à volta da forward
    if (fabs(m_rollAngle) > 0.0001f)
    {
        Quat rollQuat = Quat::FromAxisAngle(forward, m_rollAngle);
        right = rollQuat * right;
        newUp = rollQuat * newUp;
    }

    // Cria matriz de rotação (igual espírito do teu lookAt)
    Mat4 rotMat = Mat4::Identity();
    // Colunas (ou linhas) dependem da tua Mat4; vou seguir o estilo do teu lookAt:
    rotMat[0]  = right.x;
    rotMat[4]  = right.y;
    rotMat[8]  = right.z;

    rotMat[1]  = newUp.x;
    rotMat[5]  = newUp.y;
    rotMat[9]  = newUp.z;

    rotMat[2]  = -forward.x;
    rotMat[6]  = -forward.y;
    rotMat[10] = -forward.z;

    Quat worldRot = Quat::FromMat4(rotMat);

    // Compensar offset do modelo (caso a mesh aponte noutro eixo)
    // Por ex: se o modelo estiver em +Z e a lógica usa -Z, podes definir um offset de 180º em Y
    worldRot = worldRot * m_modelForwardOffset;

    outRotation = worldRot;
    return true;
}


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

void SteeringComponent::stop()
{
    m_velocity = Vec3::Zero;
    m_steeringForce = Vec3::Zero;
    
}

Vec3 SteeringComponent::seek(const Vec3 &target, float maxSpeed)
{
    if (maxSpeed < 0)
        maxSpeed = m_maxSpeed;

    Vec3 position = getOwner()->getPosition(TransformSpace::World);
    Vec3 desired = (target - position);
    float distance = desired.length();

    if (distance < 0.001f)
        return Vec3::Zero;

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
        return Vec3::Zero;

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
        return Vec3::Zero;

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
        return Vec3::Zero;
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
        return Vec3::Zero;
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

   
    m_wanderAngle += (((float)rand() / RAND_MAX) - 0.5f) * wanderJitter;

    Vec3 position = getOwner()->getPosition(TransformSpace::World);

 
    Vec3 forward = getOwner()->getForward(TransformSpace::World);
    forward.y = 0;

 
    if (forward.length() < 0.01f)
    {
        forward = m_velocity;
        forward.y = 0;
    }

 
    if (forward.length() < 0.01f)
    {
        forward = Vec3(0, 0, 1); // Forward padrão
    }

    forward = forward.normalized();

    // Centro do círculo à frente do agente
    Vec3 circleCenter = position + forward * wanderDistance;
    circleCenter.y = position.y; // Mantém no mesmo Y

    // Calcula ponto no círculo usando o ângulo
    // Right vector no plano XZ (perpendicular ao forward)
    Vec3 right(-forward.z, 0, forward.x);

    Vec3 displacement = forward * (std::cos(m_wanderAngle) * wanderRadius) +
                        right * (std::sin(m_wanderAngle) * wanderRadius);

    Vec3 wanderTarget = circleCenter + displacement;

 
    Vec3 desired = (wanderTarget - position).normalized() * maxSpeed;
    desired.y = 0; 

    return desired - m_velocity;
}

Vec3 SteeringComponent::circleStrafe(const Vec3 &target, float orbitRadius,
                                     float orbitSpeed, bool clockwise)
{
    Vec3 position = getOwner()->getPosition(TransformSpace::World);
    Vec3 toTarget = target - position;
    float distance = toTarget.length();

    Vec3 up(0, 1, 0);
    Vec3 tangent = up.cross(toTarget).normalized();
    if (!clockwise)
        tangent = tangent * -1.0f;

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

    return Vec3::Zero;
}

Vec3 SteeringComponent::align(const std::vector<GameObject *> &neighbors, float neighborRadius, float maxSpeed)
{
    if (maxSpeed < 0)
        maxSpeed = m_maxSpeed;

    Vec3 avgVelocity(0, 0, 0);
    int count = 0;

    for (auto *neighbor : neighbors)
    {
        if (!neighbor || neighbor == getOwner())
            continue;

        auto *neighborSteering = neighbor->getComponent<SteeringComponent>();
        if (!neighborSteering)
            continue;

        Vec3 neighborPos = neighbor->getPosition(TransformSpace::World);
        Vec3 position = getOwner()->getPosition(TransformSpace::World);
        float distance = (position - neighborPos).length();

        if (distance < neighborRadius)
        {
            avgVelocity += neighborSteering->getVelocity();
            count++;
        }
    }

    if (count > 0)
    {
        avgVelocity = avgVelocity / (float)count;
        Vec3 desired = avgVelocity.normalized() * maxSpeed;
        return desired - m_velocity;
    }

    return Vec3::Zero;
}

// ==================== Update ====================
// Na implementação (.cpp)

void SteeringComponent::setSmoothingEnabled(bool enabled)
{
    m_smoothingEnabled = enabled;
}

bool SteeringComponent::isSmoothingEnabled() const
{
    return m_smoothingEnabled;
}

void SteeringComponent::setSmoothingFactor(float factor)
{
    m_smoothingFactor = std::clamp(factor, 0.01f, 1.0f);
}

float SteeringComponent::getSmoothingFactor() const
{
    return m_smoothingFactor;
}

float SteeringComponent::getWanderAngle() const
{
    return m_wanderAngle;
}

Vec3 SteeringComponent::getForwardDirection() const
{
    Vec3 vel = m_velocity;
    if (m_lockY)
        vel.y = 0.0f;

    float speed = vel.length();
    if (speed < 0.0001f)
        return Vec3(0,0,0);

    return vel / speed;
}


// void SteeringComponent::update(float deltaTime)
// {
//     if (!isEnabled())
//         return;

//     if (deltaTime <= 0.0f)
//     {
//         clearForces();
//         return;
//     }

//     // 1) Limitar força de steering
//     Vec3 steering = m_steeringForce;
//     float steeringMag = steering.length();
//     if (steeringMag > m_maxForce && steeringMag > 0.0001f)
//     {
//         steering = steering * (m_maxForce / steeringMag);
//     }

//     // 2) F = m a  →  a = F / m
//     Vec3 acceleration = steering / m_mass;

//     // 3) Calcular velocidade alvo
//     Vec3 targetVelocity = m_velocity + acceleration * deltaTime;

//     // Bloquear eixo Y, se necessário
//     if (m_lockY)
//         targetVelocity.y = 0.0f;

//     // 4) Limitar velocidade máxima
//     float targetSpeed = targetVelocity.length();
//     if (targetSpeed > m_maxSpeed && targetSpeed > 0.0001f)
//     {
//         targetVelocity = targetVelocity * (m_maxSpeed / targetSpeed);
//     }

//     // 5) Suavização (LERP)
//     if (m_smoothingEnabled)
//     {
//         float t = m_smoothingFactor;
//         if (t < 0.0f) t = 0.0f;
//         if (t > 1.0f) t = 1.0f;

//         // v = v + (target - v) * t
//         m_velocity = m_velocity + (targetVelocity - m_velocity) * t;
//     }
//     else
//     {
//         m_velocity = targetVelocity;
//     }

//     // Garantir lockY também depois da suavização
//     if (m_lockY)
//         m_velocity.y = 0.0f;

//     // 6) Se velocidade for muito baixa, não fazemos nada
//     float speed = m_velocity.length();
//     const float EPS = 0.0001f;
//     if (speed < EPS)
//     {
//         m_velocity = Vec3(0.0f);
//         clearForces();
//         return;
//     }

//     Node3D* owner = getOwner();
//     if (owner)
//     {
//         // 7) Movimento
//         owner->translate(m_velocity * deltaTime, TransformSpace::World);

//         // 8) Auto-rotate para a direção do movimento
//         if (m_autoRotate)
//         {
//             Vec3 dir = m_velocity;

//             if (m_lockY)
//                 dir.y = 0.0f;

//             if (dir.lengthSquared() > EPS * EPS)
//             {
//                 dir = dir.normalized();

//                 Vec3 pos = owner->getPosition(TransformSpace::World);
//                 owner->lookAt(pos + dir, TransformSpace::World, Vec3(0, 1, 0));
//             }
//         }
//     }

 
//     clearForces();
// }

void SteeringComponent::update(float deltaTime)
{
    if (!isEnabled())
        return;

    if (deltaTime <= 0.0f)
    {
        clearForces();
        return;
    }

    // --- 1) Steering force ---
    Vec3 steering = m_steeringForce;
    float mag = steering.length();
    if (mag > m_maxForce)
        steering *= (m_maxForce / mag);

    // --- 2) Acceleration ---
    Vec3 acceleration = steering / m_mass;

    // --- 3) Target velocity ---
    Vec3 targetVelocity = m_velocity + acceleration * deltaTime;
    if (m_lockY) targetVelocity.y = 0;

    float ts = targetVelocity.length();
    if (ts > m_maxSpeed)
        targetVelocity *= (m_maxSpeed / ts);

    // --- 4) Smoothing velocity ---
    if (m_smoothingEnabled)
    {
        float t = m_smoothingFactor;
        m_velocity = m_velocity + (targetVelocity - m_velocity) * t;
    }
    else
        m_velocity = targetVelocity;

    if (m_lockY) m_velocity.y = 0;

    float speed = m_velocity.length();
    if (speed < 0.0001f)
    {
        m_velocity = Vec3(0);
        clearForces();
        return;
    }

    // --- 5) Move object ---
    Node3D* owner = getOwner();
    owner->translate(m_velocity * deltaTime, TransformSpace::World);

    // --- 6) Auto-rotate with SLERP ---
    if (m_autoRotate)
    {
        Vec3 dir = m_velocity;
        if (m_lockY) dir.y = 0;

        if (dir.lengthSquared() > 0.000001f)
        {
            dir.normalize();

       
            Mat3 rot3 = Mat3::LookAtDirection(dir, Vec3(0,1,0));
            Quat desiredRot = Quat::FromMat3(rot3);

            Quat currentRot = owner->getRotation(TransformSpace::World);

           
          
            Quat smoothRot = Quat::Slerp(currentRot, desiredRot, m_rotationSmoothFactor);

            owner->setRotation(smoothRot, TransformSpace::World);
        }
    }

 
    clearForces();
}

Vec3 SteeringComponent::pursue(const Vec3& targetPos,
                               const Vec3& targetVelocity,
                               float weight)
{
    Node3D* owner = getOwner();
    if (!owner)
        return Vec3(0);

    Vec3 ownerPos = owner->getPosition(TransformSpace::World);

    // Direção atual até ao alvo
    Vec3 toTarget = targetPos - ownerPos;
    float distance = toTarget.length();

    // Velocidade do nosso agente
    float speed = m_velocity.length();
    if (speed < 0.0001f)
    {
        // Se quase parado, faz basicamente um seek normal
        return seek(targetPos, weight);
    }

    // Tempo de previsão (quanto mais longe e mais rápido, mais à frente prevemos)
    float predictionTime = distance / speed;

    // Posição prevista do alvo
    Vec3 predictedPos = targetPos + targetVelocity * predictionTime;

    return seek(predictedPos, weight);
}

Vec3 SteeringComponent::evade(const Vec3& targetPos,
                              const Vec3& targetVelocity,
                              float weight)
{
    Node3D* owner = getOwner();
    if (!owner)
        return Vec3(0);

    Vec3 ownerPos = owner->getPosition(TransformSpace::World);

    Vec3 toTarget = targetPos - ownerPos;
    float distance = toTarget.length();

    float speed = m_velocity.length();
    if (speed < 0.0001f)
    {
       
        return flee(targetPos, weight);
    }

    float predictionTime = distance / speed;
    Vec3 predictedPos = targetPos + targetVelocity * predictionTime;

    return flee(predictedPos, weight);
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
