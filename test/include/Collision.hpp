#pragma once

#include "Math.hpp"
#include "Triangle.hpp"
#include "Plane3D.hpp"
#include <vector>
#include <cfloat>

// ==================== Collision Info ====================

struct CollisionInfo
{
    bool foundCollision;
    Vec3 intersectionPoint;
    Vec3 intersectionNormal;
    float nearestDistance;
    const Triangle *triangle;

    CollisionInfo()
        : foundCollision(false), nearestDistance(FLT_MAX), triangle(nullptr) {}
};

// ==================== Sphere Sliding Collision ====================
// Baseado em "Improved Collision detection and Response" de Kasper Fauerby

class CollisionPacket
{
public:
    // Ellipsoid radius (para sphere use Vec3(r, r, r))
    Vec3 eRadius;

    // Informação em R3 (world space)
    Vec3 R3Position;
    Vec3 R3Velocity;

    // Informação em eSpace (ellipsoid space)
    Vec3 ePosition;
    Vec3 eVelocity;
    Vec3 eNormalizedVelocity;

    // Informação de colisão
    bool foundCollision;
    float nearestDistance;
    Vec3 intersectionPoint;
    const Triangle *intersectionTriangle;

    // Configurações
    float slidingSpeed;    // Distância "muito próxima" (geralmente 0.001f)
    int maxRecursionDepth; // Máximo de iterações (geralmente 5)

    CollisionPacket()
        : eRadius(1, 1, 1), foundCollision(false), nearestDistance(FLT_MAX), intersectionTriangle(nullptr), slidingSpeed(0.001f), maxRecursionDepth(5) {}
};

// ==================== Collision System ====================

class CollisionSystem
{
private:
    std::vector<Triangle> triangles;

    // Testar colisão de swept sphere com triângulo
    bool checkTriangle(CollisionPacket &packet, const Triangle &triangle);

    // Testar swept sphere com ponto
    bool getLowestRoot(float a, float b, float c, float maxR, float &root);

    // Colisão recursiva com sliding
    Vec3 collideWithWorld(int recursionDepth, CollisionPacket &packet,
                          const Vec3 &pos, const Vec3 &vel);

public:
    CollisionSystem();

    // Adicionar/remover triângulos
    void addTriangle(const Triangle &tri);
    void addTriangles(const std::vector<Triangle> &tris);
    void removeTriangle(int index);
    void clear();

    int getTriangleCount() const;
    const Triangle &getTriangle(int index) const;
    const std::vector<Triangle> &getTriangles() const;

    // ==================== Sphere Sliding ====================

    // Colisão principal com ellipsoid/sphere sliding
    // position: posição inicial
    // velocity: movimento desejado
    // radius: raio do ellipsoid (Vec3(r,r,r) para sphere)
    // gravity: vetor de gravidade (Vec3(0, -9.8, 0) por exemplo)
    // outPosition: posição final após colisões
    // outGrounded: true se está no chão
    Vec3 collideAndSlide(const Vec3 &position, const Vec3 &velocity,
                         const Vec3 &radius, const Vec3 &gravity,
                         bool &outGrounded);

    // Versão sem gravidade
    Vec3 collideAndSlide(const Vec3 &position, const Vec3 &velocity,
                         const Vec3 &radius);

    // Versão para sphere simples (raio uniforme)
    Vec3 sphereSlide(const Vec3 &position, const Vec3 &velocity,
                     float radius, const Vec3 &gravity, bool &outGrounded);

    Vec3 sphereSlide(const Vec3 &position, const Vec3 &velocity, float radius);

    // ==================== Simple Collision Tests ====================

    // Testar se ponto está dentro de geometria
    bool pointInside(const Vec3 &point) const;

    // Ray casting simples (primeiro hit)
    bool rayCast(const Vec3 &origin, const Vec3 &direction,
                 float maxDistance, CollisionInfo &outInfo) const;

    // Ray casting com sorting (closest hit)
    bool rayCastClosest(const Vec3 &origin, const Vec3 &direction,
                        float maxDistance, CollisionInfo &outInfo) const;

    // Sphere cast (swept sphere)
    bool sphereCast(const Vec3 &origin, const Vec3 &direction,
                    float radius, float maxDistance, CollisionInfo &outInfo) const;
};

// ==================== Utility Functions ====================

// Calcular se swept sphere interseta ponto
bool sweptSphereIntersectPoint(const Vec3 &spherePos, const Vec3 &sphereVel,
                               float sphereRadius, const Vec3 &point,
                               float &outT);

// Calcular se swept sphere interseta edge
bool sweptSphereIntersectEdge(const Vec3 &spherePos, const Vec3 &sphereVel,
                              float sphereRadius,
                              const Vec3 &edgeStart, const Vec3 &edgeEnd,
                              float &outT);
 