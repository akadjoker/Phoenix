#pragma once



// Forward declarations
class Plane3D;
class Triangle;
class BoundingBox;

class Ray
{
public:
    Vec3 origin;
    Vec3 direction; // Sempre normalizado

    // Construtores
    Ray();
    Ray(const Vec3 &origin, const Vec3 &direction);

    // Criar ray de screen coordinates
    static Ray FromScreen(const Vec3 &screenPos, const Mat4 &invViewProjection);
    static Ray FromCamera(const Vec3 &cameraPos, const Vec3 &cameraForward);

    // Ponto ao longo do ray
    Vec3 at(float t) const;
    Vec3 getPoint(float t) const;

    // Intersecções básicas
    bool intersectPlane(const Plane3D &plane, float &outT) const;
    bool intersectTriangle(const Triangle &triangle, float &outT) const;
    bool intersectTriangle(const Triangle &triangle, float &outT, float &outU, float &outV) const;

    // Intersecção com esfera
    bool intersectSphere(const Vec3 &center, float radius, float &outT) const;
    bool intersectSphere(const Vec3 &center, float radius, float &outT1, float &outT2) const;

    // Intersecção com AABB
    bool intersectAABB(const Vec3 &min, const Vec3 &max, float &outTMin, float &outTMax) const;
    bool intersectAABB(const BoundingBox &box, float &outTMin, float &outTMax) const;

    // Ponto mais próximo no ray a um ponto
    Vec3 closestPoint(const Vec3 &point) const;
    float distance(const Vec3 &point) const;

    // Transform ray
    Ray transformed(const Mat4 &matrix) const;
    void transform(const Mat4 &matrix);

    // Debug
    void print() const;
};

// Hit info para ray casting
struct RayHit
{
    bool hit;
    float t;                  // Distância ao longo do ray
    Vec3 point;               // Ponto de intersecção
    Vec3 normal;              // Normal da superfície
    const Triangle *triangle; // Triângulo atingido (pode ser null)
    void *userData;           // Dados customizados

    RayHit() : hit(false), t(0), triangle(nullptr), userData(nullptr) {}
};
