#include "pch.h"

#include "Math.hpp"
#include "Ray.hpp"
#include "Plane3D.hpp"
#include "Triangle.hpp"
 

Ray::Ray() : origin(0, 0, 0), direction(0, 0, -1) {}

Ray::Ray(const Vec3 &origin, const Vec3 &direction)
    : origin(origin), direction(direction.normalized()) {}

Ray Ray::FromScreen(const Vec3 &screenPos, const Mat4 &invViewProjection)
{
    // screenPos: (x, y) em NDC [-1, 1], z = -1 (near) ou 1 (far)
    Vec4 nearPoint(screenPos.x, screenPos.y, -1.0f, 1.0f);
    Vec4 farPoint(screenPos.x, screenPos.y, 1.0f, 1.0f);

    Vec4 worldNear = invViewProjection * nearPoint;
    Vec4 worldFar = invViewProjection * farPoint;

    worldNear = worldNear / worldNear.w;
    worldFar = worldFar / worldFar.w;

    Vec3 origin(worldNear.x, worldNear.y, worldNear.z);
    Vec3 end(worldFar.x, worldFar.y, worldFar.z);
    Vec3 direction = (end - origin).normalized();

    return Ray(origin, direction);
}

Ray Ray::FromCamera(const Vec3 &cameraPos, const Vec3 &cameraForward)
{
    return Ray(cameraPos, cameraForward);
}

Vec3 Ray::at(float t) const
{
    return origin + direction * t;
}

Vec3 Ray::getPoint(float t) const
{
    return at(t);
}

bool Ray::intersectPlane(const Plane3D &plane, float &outT) const
{
    return plane.intersectRay(origin, direction, outT);
}

bool Ray::intersectTriangle(const Triangle &triangle, float &outT) const
{
    float u, v;
    return triangle.intersectRay(origin, direction, outT, u, v);
}

bool Ray::intersectTriangle(const Triangle &triangle, float &outT, float &outU, float &outV) const
{
    return triangle.intersectRay(origin, direction, outT, outU, outV);
}

bool Ray::intersectSphere(const Vec3 &center, float radius, float &outT) const
{
    Vec3 oc = origin - center;
    float a = Vec3::Dot(direction, direction);
    float b = 2.0f * Vec3::Dot(oc, direction);
    float c = Vec3::Dot(oc, oc) - radius * radius;

    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
    {
        return false;
    }

    float sqrtDisc = std::sqrt(discriminant);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);

    // Retornar o t mais próximo que seja positivo
    if (t1 > 0)
    {
        outT = t1;
        return true;
    }

    if (t2 > 0)
    {
        outT = t2;
        return true;
    }

    return false;
}

bool Ray::intersectSphere(const Vec3 &center, float radius, float &outT1, float &outT2) const
{
    Vec3 oc = origin - center;
    float a = Vec3::Dot(direction, direction);
    float b = 2.0f * Vec3::Dot(oc, direction);
    float c = Vec3::Dot(oc, oc) - radius * radius;

    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
    {
        return false;
    }

    float sqrtDisc = std::sqrt(discriminant);
    outT1 = (-b - sqrtDisc) / (2.0f * a);
    outT2 = (-b + sqrtDisc) / (2.0f * a);

    return true;
}

bool Ray::intersectAABB(const Vec3 &min, const Vec3 &max, float &outTMin, float &outTMax) const
{
    float tmin = (min.x - origin.x) / direction.x;
    float tmax = (max.x - origin.x) / direction.x;

    if (tmin > tmax)
        std::swap(tmin, tmax);

    float tymin = (min.y - origin.y) / direction.y;
    float tymax = (max.y - origin.y) / direction.y;

    if (tymin > tymax)
        std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
    {
        return false;
    }

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (min.z - origin.z) / direction.z;
    float tzmax = (max.z - origin.z) / direction.z;

    if (tzmin > tzmax)
        std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
    {
        return false;
    }

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    outTMin = tmin;
    outTMax = tmax;

    return tmax > 0; // Ray aponta para a box
}

bool Ray::intersectAABB(const BoundingBox &box, float &outTMin, float &outTMax) const
{
    return intersectAABB(box.min, box.max, outTMin, outTMax);
}

Vec3 Ray::closestPoint(const Vec3 &point) const
{
    Vec3 toPoint = point - origin;
    float t = Vec3::Dot(toPoint, direction);
    t = std::fmax(0.0f, t); // Clamp to ray start
    return at(t);
}

float Ray::distance(const Vec3 &point) const
{
    return Vec3::Distance(point, closestPoint(point));
}

Ray Ray::transformed(const Mat4 &matrix) const
{
    Vec3 newOrigin = matrix * origin;

    // Transform direction (w=0 para vetor)
    Vec4 dir4(direction.x, direction.y, direction.z, 0.0f);
    Vec4 newDir4 = matrix * dir4;
    Vec3 newDirection(newDir4.x, newDir4.y, newDir4.z);

    return Ray(newOrigin, newDirection);
}

void Ray::transform(const Mat4 &matrix)
{
    *this = transformed(matrix);
}

 