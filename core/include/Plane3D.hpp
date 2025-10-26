#pragma once

#include "Math.hpp"

// Plano 3D: ax + by + cz + d = 0
// Ou: dot(normal, point) + d = 0
struct Plane3D
{

    Vec3 normal; // Normal do plano (normalizado)
    float d;     // Distância da origem

    Plane3D();
    Plane3D(const Vec3 &normal, float d);
    Plane3D(const Vec3 &normal, const Vec3 &point);
    Plane3D(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3); // De 3 pontos
    Plane3D(float a, float b, float c, float d);             // Forma geral ax+by+cz+d=0

    // Getters
    Vec3 getNormal() const;
    float getD() const;
    void getEquation(float &outA, float &outB, float &outC, float &outD) const;

    // Setters
    void set(const Vec3 &normal, float d);
    void set(const Vec3 &normal, const Vec3 &point);
    void setFromPoints(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3);

    // Normalizar o plano
    void normalize();
    Plane3D normalized() const;

    // Distância de um ponto ao plano (signed)
    // Positivo = lado da normal, Negativo = lado oposto
    float distance(const Vec3 &point) const;

    // Classificar ponto em relação ao plano
    enum Side
    {
        FRONT,   // Lado da normal
        BACK,    // Lado oposto à normal
        ON_PLANE // No plano (com threshold)
    };
    Side classifyPoint(const Vec3 &point, float threshold = 1e-5f) const;

    // Projetar ponto no plano
    Vec3 project(const Vec3 &point) const;

    // Espelhar ponto através do plano
    Vec3 reflect(const Vec3 &point) const;

    // Espelhar vetor através do plano (útil para reflexões)
    Vec3 reflectVector(const Vec3 &direction) const;

    // Intersecção ray-plane
    // Retorna true se intersetar, e t é o parâmetro ao longo do ray
    bool intersectRay(const Vec3 &rayOrigin, const Vec3 &rayDirection, float &outT) const;

    // Intersecção line segment-plane
    bool intersectSegment(const Vec3 &p1, const Vec3 &p2, Vec3 &outIntersection) const;

    // Intersecção de 3 planos (resolve sistema linear)
    static bool Intersect3Planes(const Plane3D &p1, const Plane3D &p2, const Plane3D &p3, Vec3 &outPoint);

    // Criar planos comuns
    static Plane3D FromPointNormal(const Vec3 &point, const Vec3 &normal);
    static Plane3D XY(float z = 0.0f); // Plano XY em altura z
    static Plane3D XZ(float y = 0.0f); // Plano XZ em altura y
    static Plane3D YZ(float x = 0.0f); // Plano YZ em posição x

    // Debug
    void print() const;
};
