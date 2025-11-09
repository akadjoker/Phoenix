#include "pch.h"
#include "Collision.hpp"

// ==================== CollisionSystem ====================

CollisionSystem::CollisionSystem() {}

void CollisionSystem::addTriangle(const Triangle &tri)
{
    triangles.push_back(tri);
}

void CollisionSystem::addTriangles(const std::vector<Triangle> &tris)
{
    triangles.insert(triangles.end(), tris.begin(), tris.end());
}

void CollisionSystem::removeTriangle(int index)
{
    if (index >= 0 && index < static_cast<int>(triangles.size()))
    {
        triangles.erase(triangles.begin() + index);
    }
}

void CollisionSystem::clear()
{
    triangles.clear();
}

int CollisionSystem::getTriangleCount() const
{
    return static_cast<int>(triangles.size());
}

const Triangle &CollisionSystem::getTriangle(int index) const
{
    return triangles[index];
}

const std::vector<Triangle> &CollisionSystem::getTriangles() const
{
    return triangles;
}

// ==================== Sphere Sliding Algorithm ====================

bool CollisionSystem::getLowestRoot(float a, float b, float c, float maxR, float &root)
{
    // Resolver equação quadrática
    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f)
    {
        return false;
    }

    float sqrtD = std::sqrt(discriminant);
    float r1 = (-b - sqrtD) / (2.0f * a);
    float r2 = (-b + sqrtD) / (2.0f * a);

    // Ordenar
    if (r1 > r2)
    {
        std::swap(r1, r2);
    }

    // Obter menor root positivo
    if (r1 > 0 && r1 < maxR)
    {
        root = r1;
        return true;
    }

    if (r2 > 0 && r2 < maxR)
    {
        root = r2;
        return true;
    }

    return false;
}

bool CollisionSystem::checkTriangle(CollisionPacket &packet, const Triangle &triangle)
{
    // Todos os cálculos em ellipsoid space

    const Vec3 &p1 = triangle.v0;
    const Vec3 &p2 = triangle.v1;
    const Vec3 &p3 = triangle.v2;

    // Plano do triângulo
    Plane3D trianglePlane = triangle.getPlane();
    Vec3 planeNormal = trianglePlane.getNormal();
    float planeD = trianglePlane.getD();

    // Verificar se sphere está viajando na direção do plano
    float normalDotVelocity = Vec3::Dot(planeNormal, packet.eVelocity);

    // Se viaja para longe ou paralelo, sem colisão
    if (std::fabs(normalDotVelocity) < 1e-6f)
    {
        return false;
    }

    // Calcular intervalo de tempo durante o qual sphere interseta plano
    float t0, t1;
    bool embeddedInPlane = false;

    float signedDistToPlane = trianglePlane.distance(packet.ePosition);
    float normalDotNormVel = Vec3::Dot(planeNormal, packet.eNormalizedVelocity);

    if (std::fabs(normalDotNormVel) < 1e-6f)
    {
        // Viajando paralelo ao plano
        if (std::fabs(signedDistToPlane) >= 1.0f)
        {
            return false; // Sphere não está perto do plano
        }
        else
        {
            embeddedInPlane = true;
            t0 = 0.0f;
            t1 = 1.0f;
        }
    }
    else
    {
        // Calcular intervalo de intersecção
        float nvi = 1.0f / normalDotNormVel;
        t0 = (-1.0f - signedDistToPlane) * nvi;
        t1 = (1.0f - signedDistToPlane) * nvi;

        // Ordenar
        if (t0 > t1)
        {
            std::swap(t0, t1);
        }

        // Verificar se intervalo contém [0,1]
        if (t0 > 1.0f || t1 < 0.0f)
        {
            return false;
        }

        // Clamp para [0,1]
        if (t0 < 0.0f)
            t0 = 0.0f;
        if (t1 > 1.0f)
            t1 = 1.0f;
    }

    // OK, sphere pode intersectar o plano do triângulo
    // Verificar se interseta o triângulo em si

    Vec3 collisionPoint;
    bool foundCollision = false;
    float t = 1.0f;

    if (!embeddedInPlane)
    {
        // Verificar contra o interior do triângulo
        Vec3 planeIntersectPoint = (packet.ePosition - planeNormal) +
                                   packet.eVelocity * t0;

        if (triangle.contains(planeIntersectPoint))
        {
            foundCollision = true;
            t = t0;
            collisionPoint = planeIntersectPoint;
        }
    }

    // Se não colidiu com interior, verificar edges e vértices
    if (!foundCollision)
    {
        Vec3 velocity = packet.eVelocity;
        Vec3 base = packet.ePosition;
        float velocitySquaredLength = velocity.lengthSquared();
        float a, b, c;
        float newT;

        // P1
        a = velocitySquaredLength;
        b = 2.0f * Vec3::Dot(velocity, base - p1);
        c = (p1 - base).lengthSquared() - 1.0f;
        if (getLowestRoot(a, b, c, t, newT))
        {
            t = newT;
            foundCollision = true;
            collisionPoint = p1;
        }

        // P2
        b = 2.0f * Vec3::Dot(velocity, base - p2);
        c = (p2 - base).lengthSquared() - 1.0f;
        if (getLowestRoot(a, b, c, t, newT))
        {
            t = newT;
            foundCollision = true;
            collisionPoint = p2;
        }

        // P3
        b = 2.0f * Vec3::Dot(velocity, base - p3);
        c = (p3 - base).lengthSquared() - 1.0f;
        if (getLowestRoot(a, b, c, t, newT))
        {
            t = newT;
            foundCollision = true;
            collisionPoint = p3;
        }

        // Edges
        // Edge P1-P2
        Vec3 edge = p2 - p1;
        Vec3 baseToVertex = p1 - base;
        float edgeSquaredLength = edge.lengthSquared();
        float edgeDotVelocity = Vec3::Dot(edge, velocity);
        float edgeDotBaseToVertex = Vec3::Dot(edge, baseToVertex);

        a = edgeSquaredLength * -velocitySquaredLength + edgeDotVelocity * edgeDotVelocity;
        b = edgeSquaredLength * (2.0f * Vec3::Dot(velocity, baseToVertex)) -
            2.0f * edgeDotVelocity * edgeDotBaseToVertex;
        c = edgeSquaredLength * (1.0f - baseToVertex.lengthSquared()) +
            edgeDotBaseToVertex * edgeDotBaseToVertex;

        if (getLowestRoot(a, b, c, t, newT))
        {
            float f = (edgeDotVelocity * newT - edgeDotBaseToVertex) / edgeSquaredLength;
            if (f >= 0.0f && f <= 1.0f)
            {
                t = newT;
                foundCollision = true;
                collisionPoint = p1 + edge * f;
            }
        }

        // Edge P2-P3
        edge = p3 - p2;
        baseToVertex = p2 - base;
        edgeSquaredLength = edge.lengthSquared();
        edgeDotVelocity = Vec3::Dot(edge, velocity);
        edgeDotBaseToVertex = Vec3::Dot(edge, baseToVertex);

        a = edgeSquaredLength * -velocitySquaredLength + edgeDotVelocity * edgeDotVelocity;
        b = edgeSquaredLength * (2.0f * Vec3::Dot(velocity, baseToVertex)) -
            2.0f * edgeDotVelocity * edgeDotBaseToVertex;
        c = edgeSquaredLength * (1.0f - baseToVertex.lengthSquared()) +
            edgeDotBaseToVertex * edgeDotBaseToVertex;

        if (getLowestRoot(a, b, c, t, newT))
        {
            float f = (edgeDotVelocity * newT - edgeDotBaseToVertex) / edgeSquaredLength;
            if (f >= 0.0f && f <= 1.0f)
            {
                t = newT;
                foundCollision = true;
                collisionPoint = p2 + edge * f;
            }
        }

        // Edge P3-P1
        edge = p1 - p3;
        baseToVertex = p3 - base;
        edgeSquaredLength = edge.lengthSquared();
        edgeDotVelocity = Vec3::Dot(edge, velocity);
        edgeDotBaseToVertex = Vec3::Dot(edge, baseToVertex);

        a = edgeSquaredLength * -velocitySquaredLength + edgeDotVelocity * edgeDotVelocity;
        b = edgeSquaredLength * (2.0f * Vec3::Dot(velocity, baseToVertex)) -
            2.0f * edgeDotVelocity * edgeDotBaseToVertex;
        c = edgeSquaredLength * (1.0f - baseToVertex.lengthSquared()) +
            edgeDotBaseToVertex * edgeDotBaseToVertex;

        if (getLowestRoot(a, b, c, t, newT))
        {
            float f = (edgeDotVelocity * newT - edgeDotBaseToVertex) / edgeSquaredLength;
            if (f >= 0.0f && f <= 1.0f)
            {
                t = newT;
                foundCollision = true;
                collisionPoint = p3 + edge * f;
            }
        }
    }

    // Atualizar packet se encontrou colisão mais próxima
    if (foundCollision && t >= 0.0f && t <= packet.nearestDistance)
    {
        packet.nearestDistance = t;
        packet.intersectionPoint = collisionPoint;
        packet.foundCollision = true;
        packet.intersectionTriangle = &triangle;
        return true;
    }

    return false;
}

Vec3 CollisionSystem::collideWithWorld(int recursionDepth, CollisionPacket &packet,
                                       const Vec3 &pos, const Vec3 &vel)
{
    if (recursionDepth > packet.maxRecursionDepth)
    {
        return pos;
    }

    packet.eVelocity = vel;
    packet.eNormalizedVelocity = vel.normalized();
    packet.ePosition = pos;
    packet.foundCollision = false;
    packet.nearestDistance = std::numeric_limits<float>::max();

    // Testar contra todos os triângulos
    for (const auto &tri : triangles)
    {
        checkTriangle(packet, tri);
    }

    // Se não há colisão, retornar destino
    if (!packet.foundCollision)
    {
        return pos + vel;
    }

    // Colisão encontrada, calcular nova posição e velocidade
    Vec3 destinationPoint = pos + vel;
    Vec3 newPosition = pos;

    // Só mover se não estamos muito próximos
    float veryCloseDistance = packet.slidingSpeed;
    if (packet.nearestDistance >= veryCloseDistance)
    {
        Vec3 v = vel;
        v = v.normalized() * (packet.nearestDistance - veryCloseDistance);
        newPosition = pos + v;

        Vec3 normalized = v.normalized();
        packet.intersectionPoint = packet.intersectionPoint - (normalized * veryCloseDistance);
    }

    // Calcular sliding plane
    Vec3 slidePlaneOrigin = packet.intersectionPoint;
    Vec3 slidePlaneNormal = (newPosition - packet.intersectionPoint).normalized();

    // Project destination onto sliding plane
    float distToPlane = Vec3::Dot(destinationPoint - slidePlaneOrigin, slidePlaneNormal);
    Vec3 newDestinationPoint = destinationPoint - (slidePlaneNormal * distToPlane);

    // Gerar novo vetor de velocidade (sliding)
    Vec3 newVelocityVector = newDestinationPoint - packet.intersectionPoint;

    if (newVelocityVector.length() < veryCloseDistance)
    {
        return newPosition;
    }

    // Recursão para continuar sliding
    return collideWithWorld(recursionDepth + 1, packet, newPosition, newVelocityVector);
}

Vec3 CollisionSystem::collideAndSlide(const Vec3 &position, const Vec3 &velocity,
                                      const Vec3 &radius, const Vec3 &gravity,
                                      bool &outGrounded)
{
    CollisionPacket packet;
    packet.eRadius = radius;
    packet.R3Position = position;
    packet.R3Velocity = velocity;

    // Converter para ellipsoid space
    Vec3 eSpacePosition = position / radius;
    Vec3 eSpaceVelocity = velocity / radius;

    // Primeira passagem: movimento
    Vec3 finalPosition = collideWithWorld(0, packet, eSpacePosition, eSpaceVelocity);

    // Segunda passagem: gravidade
    outGrounded = false;
    if (gravity.lengthSquared() > 0.0f)
    {
        packet.R3Position = finalPosition * radius;
        packet.R3Velocity = gravity;

        Vec3 eSpaceGravity = gravity / radius;

        Vec3 gravityPosition = collideWithWorld(0, packet, finalPosition, eSpaceGravity);

        // Se não se moveu na direção da gravidade, está no chão
        float gravityDistance = (gravityPosition - finalPosition).length();
        outGrounded = (gravityDistance < packet.slidingSpeed * 2.0f);

        finalPosition = gravityPosition;
    }

    // Converter de volta para R3
    return finalPosition * radius;
}

Vec3 CollisionSystem::collideAndSlide(const Vec3 &position, const Vec3 &velocity,
                                      const Vec3 &radius)
{
    bool grounded;
    return collideAndSlide(position, velocity, radius, Vec3(0, 0, 0), grounded);
}

Vec3 CollisionSystem::sphereSlide(const Vec3 &position, const Vec3 &velocity,
                                  float radius, const Vec3 &gravity, bool &outGrounded)
{
    return collideAndSlide(position, velocity, Vec3(radius, radius, radius),
                           gravity, outGrounded);
}

Vec3 CollisionSystem::sphereSlide(const Vec3 &position, const Vec3 &velocity, float radius)
{
    bool grounded;
    return sphereSlide(position, velocity, radius, Vec3(0, 0, 0), grounded);
}

// ==================== Simple Collision Tests ====================

bool CollisionSystem::pointInside(const Vec3 &point) const
{
    // Ray casting simples
    Vec3 direction(1, 0.707f, 0.707f); // Direção arbitrária
    int intersections = 0;

    for (const auto &tri : triangles)
    {
        float t;
        if (tri.intersectRay(point, direction, t))
        {
            if (t > 0)
            {
                intersections++;
            }
        }
    }

    return (intersections % 2) == 1;
}

bool CollisionSystem::rayCast(const Vec3 &origin, const Vec3 &direction,
                              float maxDistance, CollisionInfo &outInfo) const
{
    outInfo.foundCollision = false;
    outInfo.nearestDistance = maxDistance;

    for (const auto &tri : triangles)
    {
        float t, u, v;
        if (tri.intersectRay(origin, direction, t, u, v))
        {
            if (t > 0 && t < outInfo.nearestDistance)
            {
                outInfo.foundCollision = true;
                outInfo.nearestDistance = t;
                outInfo.intersectionPoint = origin + direction * t;
                outInfo.intersectionNormal = tri.getNormal();
                outInfo.triangle = &tri;
            }
        }
    }

    return outInfo.foundCollision;
}

bool CollisionSystem::rayCastClosest(const Vec3 &origin, const Vec3 &direction,
                                     float maxDistance, CollisionInfo &outInfo) const
{
    return rayCast(origin, direction, maxDistance, outInfo);
}

bool CollisionSystem::sphereCast(const Vec3 &origin, const Vec3 &direction,
                                 float radius, float maxDistance,
                                 CollisionInfo &outInfo) const
{
    // Implementação simplificada
    // TODO: Implementar swept sphere completo
    return rayCast(origin, direction, maxDistance, outInfo);
}
