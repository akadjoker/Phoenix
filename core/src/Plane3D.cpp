#include "pch.h"
#include "Plane3D.hpp"


Plane3D::Plane3D() : normal(0, 1, 0), d(0) {}

Plane3D::Plane3D(const Vec3 &normal, float d)
    : normal(normal.normalized()), d(d) {}

Plane3D::Plane3D(const Vec3 &normal, const Vec3 &point)
{
    this->normal = normal.normalized();
    this->d = -Vec3::Dot(this->normal, point);
}

Plane3D::Plane3D(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3)
{
    setFromPoints(p1, p2, p3);
}

Plane3D::Plane3D(float a, float b, float c, float d)
    : normal(a, b, c), d(d)
{
    normalize();
}

Vec3 Plane3D::getNormal() const
{
    return normal;
}

float Plane3D::getD() const
{
    return d;
}

void Plane3D::getEquation(float &outA, float &outB, float &outC, float &outD) const
{
    outA = normal.x;
    outB = normal.y;
    outC = normal.z;
    outD = d;
}

void Plane3D::set(const Vec3 &normal, float d)
{
    this->normal = normal.normalized();
    this->d = d;
}

void Plane3D::set(const Vec3 &normal, const Vec3 &point)
{
    this->normal = normal.normalized();
    this->d = -Vec3::Dot(this->normal, point);
}

void Plane3D::setFromPoints(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3)
{
    Vec3 v1 = p2 - p1;
    Vec3 v2 = p3 - p1;
    normal = Vec3::Cross(v1, v2).normalized();
    d = -Vec3::Dot(normal, p1);
}

void Plane3D::normalize()
{
    float len = normal.length();
    if (len > 0.0f)
    {
        normal = normal / len;
        d = d / len;
    }
}

Plane3D Plane3D::normalized() const
{
    Plane3D result = *this;
    result.normalize();
    return result;
}

float Plane3D::distance(const Vec3 &point) const
{
    return Vec3::Dot(normal, point) + d;
}

Plane3D::Side Plane3D::classifyPoint(const Vec3 &point, float threshold) const
{
    float dist = distance(point);
    if (dist > threshold)
        return FRONT;
    if (dist < -threshold)
        return BACK;
    return ON_PLANE;
}

Vec3 Plane3D::project(const Vec3 &point) const
{
    float dist = distance(point);
    return point - normal * dist;
}

Vec3 Plane3D::reflect(const Vec3 &point) const
{
    float dist = distance(point);
    return point - normal * (2.0f * dist);
}

Vec3 Plane3D::reflectVector(const Vec3 &direction) const
{
    float dot = Vec3::Dot(direction, normal);
    return direction - normal * (2.0f * dot);
}

bool Plane3D::intersectRay(const Vec3 &rayOrigin, const Vec3 &rayDirection, float &outT) const
{
    float denom = Vec3::Dot(normal, rayDirection);

    // Ray paralelo ao plano
    if (std::fabs(denom) < 1e-6f)
    {
        return false;
    }

    float numer = -(Vec3::Dot(normal, rayOrigin) + d);
    outT = numer / denom;

    return outT >= 0.0f; // Só intersecções à frente
}

bool Plane3D::intersectSegment(const Vec3 &p1, const Vec3 &p2, Vec3 &outIntersection) const
{
    Vec3 direction = p2 - p1;
    float t;

    if (!intersectRay(p1, direction.normalized(), t))
    {
        return false;
    }

    float segmentLength = direction.length();
    if (t > segmentLength)
    {
        return false; // Intersecção além do segmento
    }

    outIntersection = p1 + direction.normalized() * t;
    return true;
}

bool Plane3D::Intersect3Planes(const Plane3D &p1, const Plane3D &p2, const Plane3D &p3, Vec3 &outPoint)
{
    // Resolver sistema 3x3:
    // n1.x * x + n1.y * y + n1.z * z = -d1
    // n2.x * x + n2.y * y + n2.z * z = -d2
    // n3.x * x + n3.y * y + n3.z * z = -d3

    Vec3 n1 = p1.getNormal();
    Vec3 n2 = p2.getNormal();
    Vec3 n3 = p3.getNormal();

    // Determinante
    float det = Vec3::Dot(n1, Vec3::Cross(n2, n3));

    if (std::fabs(det) < 1e-6f)
    {
        return false; // Planos paralelos ou colineares
    }

    // Regra de Cramer
    outPoint = (Vec3::Cross(n2, n3) * (-p1.d) +
                Vec3::Cross(n3, n1) * (-p2.d) +
                Vec3::Cross(n1, n2) * (-p3.d)) /
               det;

    return true;
}

Plane3D Plane3D::FromPointNormal(const Vec3 &point, const Vec3 &normal)
{
    return Plane3D(normal, point);
}

Plane3D Plane3D::XY(float z)
{
    return Plane3D(Vec3(0, 0, 1), Vec3(0, 0, z));
}

Plane3D Plane3D::XZ(float y)
{
    return Plane3D(Vec3(0, 1, 0), Vec3(0, y, 0));
}

Plane3D Plane3D::YZ(float x)
{
    return Plane3D(Vec3(1, 0, 0), Vec3(x, 0, 0));
}

void Plane3D::print() const
{
    std::cout << "Plane3D: " << normal.x << "x + " << normal.y << "y + "
              << normal.z << "z + " << d << " = 0" << std::endl;
}

 