#include "pch.h"
#include "Triangle.hpp"
#include "Plane3D.hpp"

Triangle::Triangle() : v0(0, 0, 0), v1(1, 0, 0), v2(0, 1, 0) {}

Triangle::Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2)
    : v0(v0), v1(v1), v2(v2) {}

Vec3 Triangle::getNormal() const
{
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    return Vec3::Cross(edge1, edge2).normalized();
}

Vec3 Triangle::getCenter() const
{
    return (v0 + v1 + v2) / 3.0f;
}

float Triangle::getArea() const
{
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    return Vec3::Cross(edge1, edge2).length() * 0.5f;
}

Plane3D Triangle::getPlane() const
{
    return Plane3D(v0, v1, v2);
}

void Triangle::getBounds(Vec3 &outMin, Vec3 &outMax) const
{
    outMin = Vec3(
        std::fmin(v0.x, std::fmin(v1.x, v2.x)),
        std::fmin(v0.y, std::fmin(v1.y, v2.y)),
        std::fmin(v0.z, std::fmin(v1.z, v2.z)));

    outMax = Vec3(
        std::fmax(v0.x, std::fmax(v1.x, v2.x)),
        std::fmax(v0.y, std::fmax(v1.y, v2.y)),
        std::fmax(v0.z, std::fmax(v1.z, v2.z)));
}

bool Triangle::contains(const Vec3 &point) const
{
    Vec3 bary = getBarycentricCoords(point);
    return isInsideBarycentric(bary);
}

Vec3 Triangle::getBarycentricCoords(const Vec3 &point) const
{
    Vec3 v0v1 = v1 - v0;
    Vec3 v0v2 = v2 - v0;
    Vec3 v0p = point - v0;

    float d00 = Vec3::Dot(v0v1, v0v1);
    float d01 = Vec3::Dot(v0v1, v0v2);
    float d11 = Vec3::Dot(v0v2, v0v2);
    float d20 = Vec3::Dot(v0p, v0v1);
    float d21 = Vec3::Dot(v0p, v0v2);

    float denom = d00 * d11 - d01 * d01;

    if (std::fabs(denom) < 1e-6f)
    {
        return Vec3(1, 0, 0); // Degenerate triangle
    }

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return Vec3(u, v, w);
}

bool Triangle::isInsideBarycentric(const Vec3 &baryCoords) const
{
    return baryCoords.x >= 0.0f && baryCoords.y >= 0.0f && baryCoords.z >= 0.0f;
}

Vec3 Triangle::fromBarycentric(const Vec3 &baryCoords) const
{
    return v0 * baryCoords.x + v1 * baryCoords.y + v2 * baryCoords.z;
}

bool Triangle::intersectRay(const Vec3 &rayOrigin, const Vec3 &rayDirection,
                            float &outT, float &outU, float &outV) const
{
    // Möller-Trumbore algorithm
    const float EPSILON = 1e-6f;

    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;

    Vec3 h = Vec3::Cross(rayDirection, edge2);
    float a = Vec3::Dot(edge1, h);

    // Ray paralelo ao triângulo
    if (a > -EPSILON && a < EPSILON)
    {
        return false;
    }

    float f = 1.0f / a;
    Vec3 s = rayOrigin - v0;
    outU = f * Vec3::Dot(s, h);

    if (outU < 0.0f || outU > 1.0f)
    {
        return false;
    }

    Vec3 q = Vec3::Cross(s, edge1);
    outV = f * Vec3::Dot(rayDirection, q);

    if (outV < 0.0f || outU + outV > 1.0f)
    {
        return false;
    }

    // Calcular t
    outT = f * Vec3::Dot(edge2, q);

    return outT > EPSILON; // Intersecção válida à frente do ray
}

bool Triangle::intersectRay(const Vec3 &rayOrigin, const Vec3 &rayDirection) const
{
    float t, u, v;
    return intersectRay(rayOrigin, rayDirection, t, u, v);
}

Vec3 Triangle::closestPoint(const Vec3 &point) const
{
    // Projetar ponto no plano do triângulo
    Plane3D plane = getPlane();
    Vec3 projected = plane.project(point);

    // Se dentro do triângulo, retornar projeção
    if (contains(projected))
    {
        return projected;
    }

    // Senão, encontrar ponto mais próximo nas arestas
    auto closestPointOnSegment = [](const Vec3 &p, const Vec3 &a, const Vec3 &b)
    {
        Vec3 ab = b - a;
        float t = Vec3::Dot(p - a, ab) / Vec3::Dot(ab, ab);
        t = std::fmax(0.0f, std::fmin(1.0f, t));
        return a + ab * t;
    };

    Vec3 p1 = closestPointOnSegment(point, v0, v1);
    Vec3 p2 = closestPointOnSegment(point, v1, v2);
    Vec3 p3 = closestPointOnSegment(point, v2, v0);

    float d1 = Vec3::Distance(point, p1);
    float d2 = Vec3::Distance(point, p2);
    float d3 = Vec3::Distance(point, p3);

    if (d1 <= d2 && d1 <= d3)
        return p1;
    if (d2 <= d3)
        return p2;
    return p3;
}

float Triangle::distance(const Vec3 &point) const
{
    return Vec3::Distance(point, closestPoint(point));
}

Triangle Triangle::transformed(const Mat4 &matrix) const
{
    return Triangle(
        matrix * v0,
        matrix * v1,
        matrix * v2);
}

void Triangle::transform(const Mat4 &matrix)
{
    v0 = matrix * v0;
    v1 = matrix * v1;
    v2 = matrix * v2;
}

void Triangle::flip()
{
    Vec3 temp = v1;
    v1 = v2;
    v2 = temp;
}

Triangle Triangle::flipped() const
{
    return Triangle(v0, v2, v1);
}

void Triangle::print() const
{
    // std::cout << "Triangle:" << std::endl;
    // std::cout << "  v0: " << v0 << std::endl;
    // std::cout << "  v1: " << v1 << std::endl;
    // std::cout << "  v2: " << v2 << std::endl;
    // std::cout << "  Normal: " << getNormal() << std::endl;
    // std::cout << "  Area: " << getArea() << std::endl;
}

 