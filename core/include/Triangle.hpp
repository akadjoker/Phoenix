#pragma once

#include "Math.hpp"
class Plane3D;

class Triangle
{
public:
    Vec3 v0, v1, v2; // Vértices

    // Construtores
    Triangle();
    Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2);

    // Propriedades geométricas
    Vec3 getNormal() const;
    Vec3 getCenter() const;
    float getArea() const;
    Plane3D getPlane() const;

    // Bounding box
    void getBounds(Vec3 &outMin, Vec3 &outMax) const;

    // Verificar se ponto está dentro do triângulo (2D projetado)
    bool contains(const Vec3 &point) const;

    // Coordenadas baricêntricas
    // u, v, w onde ponto = u*v0 + v*v1 + w*v2 (u+v+w=1)
    Vec3 getBarycentricCoords(const Vec3 &point) const;
    bool isInsideBarycentric(const Vec3 &baryCoords) const;
    Vec3 fromBarycentric(const Vec3 &baryCoords) const;

    // Intersecção ray-triangle (Möller-Trumbore algorithm)
    // Retorna true se intersetar
    // outT = distância ao longo do ray
    // outU, outV = coordenadas baricêntricas da intersecção
    bool intersectRay(const Vec3 &rayOrigin, const Vec3 &rayDirection,
                      float &outT, float &outU, float &outV) const;

    // Versão simples que só retorna se interseta
    bool intersectRay(const Vec3 &rayOrigin, const Vec3 &rayDirection) const;

    // Ponto mais próximo no triângulo
    Vec3 closestPoint(const Vec3 &point) const;

    // Distância de ponto ao triângulo
    float distance(const Vec3 &point) const;

    // Transformar triângulo
    Triangle transformed(const Mat4 &matrix) const;
    void transform(const Mat4 &matrix);

    // Flip normal (inverter winding)
    void flip();
    Triangle flipped() const;

    // Debug
    void print() const;
};
