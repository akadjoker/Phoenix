#pragma once

#include "Math.hpp"
#include "Triangle.hpp"
#include <vector>

// ==================== Quadtree Node ====================

class QuadtreeNode
{
public:
    BoundingBox bounds;              // Bounding box do node
    std::vector<Triangle> triangles; // Triângulos neste node
    QuadtreeNode *children[4];       // 4 filhos (NW, NE, SW, SE)
    int depth;                       // Profundidade na árvore
    bool isLeaf;                     // É leaf node?

    QuadtreeNode(const BoundingBox &bounds, int depth);
    ~QuadtreeNode();

    // Split node em 4 quadrantes
    void split();

    // Verificar se triângulo cabe completamente no node
    bool containsTriangle(const Triangle &tri) const;

    // Verificar se triângulo interseta o node
    bool intersectsTriangle(const Triangle &tri) const;
};

// ==================== Quadtree ====================

class Quadtree
{
private:
    QuadtreeNode *root;
    int maxDepth;            // Profundidade máxima da árvore
    int maxTrianglesPerNode; // Máximo de triângulos antes de split
    int totalTriangles;

    // Inserir triângulo recursivamente
    void insertRecursive(QuadtreeNode *node, const Triangle &tri);

    // Query recursivo
    void queryRecursive(QuadtreeNode *node, const BoundingBox &queryBounds,
                        std::vector<const Triangle *> &outTriangles) const;

    void queryRecursive(QuadtreeNode *node, const Vec3 &point,
                        std::vector<const Triangle *> &outTriangles) const;

    void queryRecursive(QuadtreeNode *node, const Vec3 &sphereCenter, float sphereRadius,
                        std::vector<const Triangle *> &outTriangles) const;

public:
    Quadtree(const Vec3 &min, const Vec3 &max, int maxDepth = 8, int maxTrisPerNode = 10);
    Quadtree(const BoundingBox &bounds, int maxDepth = 8, int maxTrisPerNode = 10);
    ~Quadtree();

    // Inserir triângulos
    void insert(const Triangle &tri);
    void insert(const std::vector<Triangle> &tris);

    // Build tree de uma vez (mais eficiente que insert individual)
    void build(const std::vector<Triangle> &tris);

    // Limpar árvore
    void clear();

    // Rebuild árvore
    void rebuild();

    // Query methods

    // Obter triângulos numa região AABB
    void query(const BoundingBox &queryBounds, std::vector<const Triangle *> &outTriangles) const;

    // Obter triângulos próximos de um ponto
    void query(const Vec3 &point, float radius, std::vector<const Triangle *> &outTriangles) const;

    // Obter triângulos numa esfera
    void querySphere(const Vec3 &center, float radius, std::vector<const Triangle *> &outTriangles) const;

    // Ray query (obter triângulos ao longo do ray)
    void queryRay(const Vec3 &origin, const Vec3 &direction, float maxDistance,
                  std::vector<const Triangle *> &outTriangles) const;

    // Estatísticas
    int getTotalTriangles() const;
    int getNodeCount() const;
    int getMaxDepthReached() const;
    void getStats(int &outNodes, int &outLeaves, int &outMaxDepth) const;

 

private:
    void getStatsRecursive(QuadtreeNode *node, int &outNodes, int &outLeaves, int &outMaxDepth) const;
 
};

// ==================== Octree Node ====================

class OctreeNode
{
public:
    BoundingBox bounds;              // Bounding box do node
    std::vector<Triangle> triangles; // Triângulos neste node
    OctreeNode *children[8];         // 8 filhos (octantes)
    int depth;                       // Profundidade na árvore
    bool isLeaf;                     // É leaf node?

    OctreeNode(const BoundingBox &bounds, int depth);
    ~OctreeNode();

    // Split node em 8 octantes
    void split();

    // Verificar se triângulo cabe completamente no node
    bool containsTriangle(const Triangle &tri) const;

    // Verificar se triângulo interseta o node
    bool intersectsTriangle(const Triangle &tri) const;
};

// ==================== Octree ====================

class Octree
{
private:
    OctreeNode *root;
    int maxDepth;            // Profundidade máxima da árvore
    int maxTrianglesPerNode; // Máximo de triângulos antes de split
    int totalTriangles;

    // Inserir triângulo recursivamente
    void insertRecursive(OctreeNode *node, const Triangle &tri);

    // Query recursivo
    void queryRecursive(OctreeNode *node, const BoundingBox &queryBounds,
                        std::vector<const Triangle *> &outTriangles) const;

    void queryRecursive(OctreeNode *node, const Vec3 &point,
                        std::vector<const Triangle *> &outTriangles) const;

    void queryRecursive(OctreeNode *node, const Vec3 &sphereCenter, float sphereRadius,
                        std::vector<const Triangle *> &outTriangles) const;

public:
    Octree(const Vec3 &min, const Vec3 &max, int maxDepth = 8, int maxTrisPerNode = 10);
    Octree(const BoundingBox &bounds, int maxDepth = 8, int maxTrisPerNode = 10);
    ~Octree();

    // Inserir triângulos
    void insert(const Triangle &tri);
    void insert(const std::vector<Triangle> &tris);

    // Build tree de uma vez (mais eficiente que insert individual)
    void build(const std::vector<Triangle> &tris);

    // Limpar árvore
    void clear();

    // Rebuild árvore
    void rebuild();

    // Query methods

    // Obter triângulos numa região AABB
    void query(const BoundingBox &queryBounds, std::vector<const Triangle *> &outTriangles) const;

    // Obter triângulos próximos de um ponto
    void query(const Vec3 &point, float radius, std::vector<const Triangle *> &outTriangles) const;

    // Obter triângulos numa esfera
    void querySphere(const Vec3 &center, float radius, std::vector<const Triangle *> &outTriangles) const;

    // Ray query (obter triângulos ao longo do ray)
    void queryRay(const Vec3 &origin, const Vec3 &direction, float maxDistance,
                  std::vector<const Triangle *> &outTriangles) const;

    // Frustum query (obter triângulos visíveis no frustum)
    void queryFrustum(const class Frustum &frustum, std::vector<const Triangle *> &outTriangles) const;

    // Estatísticas
    int getTotalTriangles() const;
    int getNodeCount() const;
    int getMaxDepthReached() const;
    void getStats(int &outNodes, int &outLeaves, int &outMaxDepth) const;
    float getMemoryUsage() const; // Em KB
 

private:
    void getStatsRecursive(OctreeNode *node, int &outNodes, int &outLeaves, int &outMaxDepth) const;
 
    void queryFrustumRecursive(OctreeNode *node, const class Frustum &frustum,
                               std::vector<const Triangle *> &outTriangles) const;
};
