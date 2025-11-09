#include "pch.h"
#include "Frustum.hpp"

Frustum::Frustum()
{
}

void Frustum::extractFromMatrix(const Mat4 &viewProjection)
{
    const float *m = viewProjection.m;

    // Left plane
    planes[PLANE_LEFT] = Plane3D(
        m[3] + m[0],
        m[7] + m[4],
        m[11] + m[8],
        m[15] + m[12]);

    // Right plane
    planes[PLANE_RIGHT] = Plane3D(
        m[3] - m[0],
        m[7] - m[4],
        m[11] - m[8],
        m[15] - m[12]);

    // Bottom plane
    planes[PLANE_BOTTOM] = Plane3D(
        m[3] + m[1],
        m[7] + m[5],
        m[11] + m[9],
        m[15] + m[13]);

    // Top plane
    planes[PLANE_TOP] = Plane3D(
        m[3] - m[1],
        m[7] - m[5],
        m[11] - m[9],
        m[15] - m[13]);

    // Near plane
    planes[PLANE_NEAR] = Plane3D(
        m[3] + m[2],
        m[7] + m[6],
        m[11] + m[10],
        m[15] + m[14]);

    // Far plane
    planes[PLANE_FAR] = Plane3D(
        m[3] - m[2],
        m[7] - m[6],
        m[11] - m[10],
        m[15] - m[14]);

    // Normalizar todos os planos
    for (int i = 0; i < PLANE_COUNT; i++)
    {
        planes[i].normalize();
    }
}

void Frustum::extractFromCamera(const Mat4 &view, const Mat4 &projection)
{
    Mat4 vp = projection * view;
    extractFromMatrix(vp);
}

const Plane3D &Frustum::getPlane(PlaneIndex index) const
{
    return planes[index];
}

Plane3D &Frustum::getPlane(PlaneIndex index)
{
    return planes[index];
}

bool Frustum::containsPoint(const Vec3 &point) const
{
    for (int i = 0; i < PLANE_COUNT; i++)
    {
        if (planes[i].distance(point) < 0)
        {
            return false; // Ponto está atrás de algum plano
        }
    }
    return true;
}

bool Frustum::intersectsSphere(const Vec3 &center, float radius) const
{
    for (int i = 0; i < PLANE_COUNT; i++)
    {
        float distance = planes[i].distance(center);
        if (distance < -radius)
        {
            return false; // Esfera completamente fora
        }
    }
    return true;
}

bool Frustum::intersectsAABB(const Vec3 &min, const Vec3 &max) const
{
    for (int i = 0; i < PLANE_COUNT; i++)
    {
        const Plane3D &plane = planes[i];
        Vec3 normal = plane.getNormal();

        // Encontrar o vértice positivo (mais distante na direção da normal)
        Vec3 pVertex(
            normal.x >= 0 ? max.x : min.x,
            normal.y >= 0 ? max.y : min.y,
            normal.z >= 0 ? max.z : min.z);

        // Se o vértice positivo está atrás do plano, BoundingBox está fora
        if (plane.distance(pVertex) < 0)
        {
            return false;
        }
    }
    return true;
}

bool Frustum::intersectsAABB(const BoundingBox &box) const
{
    return intersectsAABB(box.min, box.max);
}

bool Frustum::intersectsOBB(const Vec3 corners[8]) const
{
    for (int i = 0; i < PLANE_COUNT; i++)
    {
        int out = 0;
        for (int j = 0; j < 8; j++)
        {
            if (planes[i].distance(corners[j]) < 0)
            {
                out++;
            }
        }

        // Se todos os 8 cantos estão fora deste plano, OBB está fora
        if (out == 8)
        {
            return false;
        }
    }
    return true;
}

Frustum::IntersectionResult Frustum::testSphere(const Vec3 &center, float radius) const
{
    bool intersecting = false;

    for (int i = 0; i < PLANE_COUNT; i++)
    {
        float distance = planes[i].distance(center);

        if (distance < -radius)
        {
            return OUTSIDE; // Completamente fora
        }

        if (std::fabs(distance) < radius)
        {
            intersecting = true; // Intersectando pelo menos um plano
        }
    }

    return intersecting ? INTERSECTING : INSIDE;
}

Frustum::IntersectionResult Frustum::testAABB(const Vec3 &min, const Vec3 &max) const
{
    bool intersecting = false;

    for (int i = 0; i < PLANE_COUNT; i++)
    {
        const Plane3D &plane = planes[i];
        Vec3 normal = plane.getNormal();

        // P-vertex (vértice mais distante na direção da normal)
        Vec3 pVertex(
            normal.x >= 0 ? max.x : min.x,
            normal.y >= 0 ? max.y : min.y,
            normal.z >= 0 ? max.z : min.z);

        // N-vertex (vértice mais próximo na direção da normal)
        Vec3 nVertex(
            normal.x >= 0 ? min.x : max.x,
            normal.y >= 0 ? min.y : max.y,
            normal.z >= 0 ? min.z : max.z);

        float pDist = plane.distance(pVertex);
        float nDist = plane.distance(nVertex);

        if (pDist < 0)
        {
            return OUTSIDE; // P-vertex atrás = completamente fora
        }

        if (nDist < 0)
        {
            intersecting = true; // N-vertex atrás = intersectando
        }
    }

    return intersecting ? INTERSECTING : INSIDE;
}

Frustum::IntersectionResult Frustum::testAABB(const BoundingBox &box) const
{
    return testAABB(box.min, box.max);
}

void Frustum::getCorners(Vec3 outCorners[8]) const
{
    // Intersectar planos para obter os 8 cantos
    // Near plane corners
    Plane3D::Intersect3Planes(planes[PLANE_NEAR], planes[PLANE_LEFT], planes[PLANE_BOTTOM], outCorners[0]);
    Plane3D::Intersect3Planes(planes[PLANE_NEAR], planes[PLANE_RIGHT], planes[PLANE_BOTTOM], outCorners[1]);
    Plane3D::Intersect3Planes(planes[PLANE_NEAR], planes[PLANE_RIGHT], planes[PLANE_TOP], outCorners[2]);
    Plane3D::Intersect3Planes(planes[PLANE_NEAR], planes[PLANE_LEFT], planes[PLANE_TOP], outCorners[3]);

    // Far plane corners
    Plane3D::Intersect3Planes(planes[PLANE_FAR], planes[PLANE_LEFT], planes[PLANE_BOTTOM], outCorners[4]);
    Plane3D::Intersect3Planes(planes[PLANE_FAR], planes[PLANE_RIGHT], planes[PLANE_BOTTOM], outCorners[5]);
    Plane3D::Intersect3Planes(planes[PLANE_FAR], planes[PLANE_RIGHT], planes[PLANE_TOP], outCorners[6]);
    Plane3D::Intersect3Planes(planes[PLANE_FAR], planes[PLANE_LEFT], planes[PLANE_TOP], outCorners[7]);
}

 