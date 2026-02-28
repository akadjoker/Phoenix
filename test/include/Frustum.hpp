#pragma once

#include "Math.hpp"
#include "Plane3D.hpp"
 
class Frustum
{
public:
    enum PlaneIndex
    {
        PLANE_LEFT = 0,
        PLANE_RIGHT,
        PLANE_BOTTOM,
        PLANE_TOP,
        PLANE_NEAR,
        PLANE_FAR,
        PLANE_COUNT = 6
    };

private:
    Plane3D planes[PLANE_COUNT];

public:
 
    Frustum();

 
    void extractFromMatrix(const Mat4 &viewProjection);

 
    void extractFromCamera(const Mat4 &view, const Mat4 &projection);

 
    const Plane3D &getPlane(PlaneIndex index) const;
    Plane3D &getPlane(PlaneIndex index);

 
    bool containsPoint(const Vec3 &point) const;
 
    bool intersectsSphere(const Vec3 &center, float radius) const;

 
    bool intersectsAABB(const Vec3 &min, const Vec3 &max) const;
    bool intersectsAABB(const BoundingBox &box) const;

 
    bool intersectsOBB(const Vec3 corners[8]) const;

    // Resultado detalhado
    enum IntersectionResult
    {
        OUTSIDE,      // Completamente fora
        INTERSECTING, // Parcialmente dentro
        INSIDE        // Completamente dentro
    };

    IntersectionResult testSphere(const Vec3 &center, float radius) const;
    IntersectionResult testAABB(const Vec3 &min, const Vec3 &max) const;
    IntersectionResult testAABB(const BoundingBox &box) const;

   
    void getCorners(Vec3 outCorners[8]) const;

 
};
