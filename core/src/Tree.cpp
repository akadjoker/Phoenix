#include "pch.h"
#include "Tree.hpp"
#include "Frustum.hpp"

// ==================== QuadtreeNode ====================

QuadtreeNode::QuadtreeNode(const BoundingBox &bounds, int depth)
    : bounds(bounds), depth(depth), isLeaf(true)
{
    for (int i = 0; i < 4; i++)
    {
        children[i] = nullptr;
    }
}

QuadtreeNode::~QuadtreeNode()
{
    for (int i = 0; i < 4; i++)
    {
        delete children[i];
    }
}

void QuadtreeNode::split()
{
    if (!isLeaf)
        return;

    Vec3 center = bounds.center();
    Vec3 size = bounds.size();
    Vec3 halfSize = size * 0.5f;

    // Criar 4 quadrantes (no plano XZ, ignora Y)
    // NW (North-West) = canto superior esquerdo
    // NE (North-East) = canto superior direito
    // SW (South-West) = canto inferior esquerdo
    // SE (South-East) = canto inferior direito

    // NW
    children[0] = new QuadtreeNode(
        BoundingBox(Vec3(bounds.min.x, bounds.min.y, center.z),
                    Vec3(center.x, bounds.max.y, bounds.max.z)),
        depth + 1);

    // NE
    children[1] = new QuadtreeNode(
        BoundingBox(Vec3(center.x, bounds.min.y, center.z),
                    Vec3(bounds.max.x, bounds.max.y, bounds.max.z)),
        depth + 1);

    // SW
    children[2] = new QuadtreeNode(
        BoundingBox(Vec3(bounds.min.x, bounds.min.y, bounds.min.z),
                    Vec3(center.x, bounds.max.y, center.z)),
        depth + 1);

    // SE
    children[3] = new QuadtreeNode(
        BoundingBox(Vec3(center.x, bounds.min.y, bounds.min.z),
                    Vec3(bounds.max.x, bounds.max.y, center.z)),
        depth + 1);

    isLeaf = false;
}

bool QuadtreeNode::containsTriangle(const Triangle &tri) const
{
    return bounds.contains(tri.v0) &&
           bounds.contains(tri.v1) &&
           bounds.contains(tri.v2);
}

bool QuadtreeNode::intersectsTriangle(const Triangle &tri) const
{
    // Simplificado: verificar se algum vértice está dentro
    // ou se BoundingBox do triângulo interseta bounds do node
    Vec3 triMin, triMax;
    tri.getBounds(triMin, triMax);

    BoundingBox triBounds(triMin, triMax);

    // BoundingBox intersection test
    if (bounds.min.x > triBounds.max.x || triBounds.min.x > bounds.max.x)
        return false;
    if (bounds.min.y > triBounds.max.y || triBounds.min.y > bounds.max.y)
        return false;
    if (bounds.min.z > triBounds.max.z || triBounds.min.z > bounds.max.z)
        return false;

    return true;
}

// ==================== Quadtree ====================

Quadtree::Quadtree(const Vec3 &min, const Vec3 &max, int maxDepth, int maxTrisPerNode)
    : maxDepth(maxDepth), maxTrianglesPerNode(maxTrisPerNode), totalTriangles(0)
{
    root = new QuadtreeNode(BoundingBox(min, max), 0);
}

Quadtree::Quadtree(const BoundingBox &bounds, int maxDepth, int maxTrisPerNode)
    : maxDepth(maxDepth), maxTrianglesPerNode(maxTrisPerNode), totalTriangles(0)
{
    root = new QuadtreeNode(bounds, 0);
}

Quadtree::~Quadtree()
{
    delete root;
}

void Quadtree::insertRecursive(QuadtreeNode *node, const Triangle &tri)
{
    if (!node->intersectsTriangle(tri))
    {
        return; // Triângulo não interseta este node
    }

    if (node->isLeaf)
    {
        // Adicionar ao leaf node
        node->triangles.push_back(tri);

        // Verificar se precisa split
        if (static_cast<int>(node->triangles.size()) > maxTrianglesPerNode &&
            node->depth < maxDepth)
        {
            node->split();

            // Redistribuir triângulos para filhos
            std::vector<Triangle> oldTriangles = node->triangles;
            node->triangles.clear();

            for (const auto &oldTri : oldTriangles)
            {
                bool addedToChild = false;
                for (int i = 0; i < 4; i++)
                {
                    if (node->children[i]->containsTriangle(oldTri))
                    {
                        insertRecursive(node->children[i], oldTri);
                        addedToChild = true;
                        break;
                    }
                }

                // Se não cabe em nenhum filho, manter no pai
                if (!addedToChild)
                {
                    node->triangles.push_back(oldTri);
                }
            }
        }
    }
    else
    {
        // Tentar adicionar aos filhos
        bool addedToChild = false;
        for (int i = 0; i < 4; i++)
        {
            if (node->children[i]->containsTriangle(tri))
            {
                insertRecursive(node->children[i], tri);
                addedToChild = true;
                break;
            }
        }

        // Se não cabe em nenhum filho, adicionar a este node
        if (!addedToChild)
        {
            node->triangles.push_back(tri);
        }
    }
}

void Quadtree::insert(const Triangle &tri)
{
    insertRecursive(root, tri);
    totalTriangles++;
}

void Quadtree::insert(const std::vector<Triangle> &tris)
{
    for (const auto &tri : tris)
    {
        insert(tri);
    }
}

void Quadtree::build(const std::vector<Triangle> &tris)
{
    clear();
    insert(tris);
}

void Quadtree::clear()
{
    delete root;
    root = new QuadtreeNode(root->bounds, 0);
    totalTriangles = 0;
}

void Quadtree::rebuild()
{
    // Coletar todos os triângulos
    std::vector<Triangle> allTriangles;
    std::vector<const Triangle *> queryResult;
    query(root->bounds, queryResult);

    for (auto *tri : queryResult)
    {
        allTriangles.push_back(*tri);
    }

    // Rebuild
    build(allTriangles);
}

void Quadtree::queryRecursive(QuadtreeNode *node, const BoundingBox &queryBounds,
                              std::vector<const Triangle *> &outTriangles) const
{
    if (!node)
        return;

    // BoundingBox intersection test
    if (node->bounds.min.x > queryBounds.max.x || queryBounds.min.x > node->bounds.max.x)
        return;
    if (node->bounds.min.y > queryBounds.max.y || queryBounds.min.y > node->bounds.max.y)
        return;
    if (node->bounds.min.z > queryBounds.max.z || queryBounds.min.z > node->bounds.max.z)
        return;

    // Adicionar triângulos deste node
    for (const auto &tri : node->triangles)
    {
        outTriangles.push_back(&tri);
    }

    // Recursão nos filhos
    if (!node->isLeaf)
    {
        for (int i = 0; i < 4; i++)
        {
            queryRecursive(node->children[i], queryBounds, outTriangles);
        }
    }
}

void Quadtree::queryRecursive(QuadtreeNode *node, const Vec3 &point,
                              std::vector<const Triangle *> &outTriangles) const
{
    if (!node || !node->bounds.contains(point))
        return;

    for (const auto &tri : node->triangles)
    {
        outTriangles.push_back(&tri);
    }

    if (!node->isLeaf)
    {
        for (int i = 0; i < 4; i++)
        {
            queryRecursive(node->children[i], point, outTriangles);
        }
    }
}

void Quadtree::queryRecursive(QuadtreeNode *node, const Vec3 &sphereCenter,
                              float sphereRadius,
                              std::vector<const Triangle *> &outTriangles) const
{
    if (!node)
        return;

    // Sphere-BoundingBox intersection
    Vec3 closestPoint = Vec3(
        std::fmax(node->bounds.min.x, std::fmin(sphereCenter.x, node->bounds.max.x)),
        std::fmax(node->bounds.min.y, std::fmin(sphereCenter.y, node->bounds.max.y)),
        std::fmax(node->bounds.min.z, std::fmin(sphereCenter.z, node->bounds.max.z)));

    float distSq = Vec3::DistanceSquared(closestPoint, sphereCenter);
    if (distSq > sphereRadius * sphereRadius)
    {
        return; // Sphere não interseta este node
    }

    for (const auto &tri : node->triangles)
    {
        outTriangles.push_back(&tri);
    }

    if (!node->isLeaf)
    {
        for (int i = 0; i < 4; i++)
        {
            queryRecursive(node->children[i], sphereCenter, sphereRadius, outTriangles);
        }
    }
}

void Quadtree::query(const BoundingBox &queryBounds, std::vector<const Triangle *> &outTriangles) const
{
    outTriangles.clear();
    queryRecursive(root, queryBounds, outTriangles);
}

void Quadtree::query(const Vec3 &point, float radius, std::vector<const Triangle *> &outTriangles) const
{
    outTriangles.clear();
    BoundingBox queryBounds(point - Vec3(radius, radius, radius),
                            point + Vec3(radius, radius, radius));
    queryRecursive(root, queryBounds, outTriangles);
}

void Quadtree::querySphere(const Vec3 &center, float radius,
                           std::vector<const Triangle *> &outTriangles) const
{
    outTriangles.clear();
    queryRecursive(root, center, radius, outTriangles);
}

void Quadtree::queryRay(const Vec3 &origin, const Vec3 &direction, float maxDistance,
                        std::vector<const Triangle *> &outTriangles) const
{
    outTriangles.clear();

    // Criar BoundingBox ao longo do ray
    Vec3 end = origin + direction * maxDistance;
    Vec3 min = Vec3(
        std::fmin(origin.x, end.x),
        std::fmin(origin.y, end.y),
        std::fmin(origin.z, end.z));
    Vec3 max = Vec3(
        std::fmax(origin.x, end.x),
        std::fmax(origin.y, end.y),
        std::fmax(origin.z, end.z));

    BoundingBox rayBounds(min, max);
    queryRecursive(root, rayBounds, outTriangles);
}

int Quadtree::getTotalTriangles() const
{
    return totalTriangles;
}

void Quadtree::getStatsRecursive(QuadtreeNode *node, int &outNodes, int &outLeaves,
                                 int &outMaxDepth) const
{
    if (!node)
        return;

    outNodes++;
    if (node->depth > outMaxDepth)
    {
        outMaxDepth = node->depth;
    }

    if (node->isLeaf)
    {
        outLeaves++;
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            getStatsRecursive(node->children[i], outNodes, outLeaves, outMaxDepth);
        }
    }
}

void Quadtree::getStats(int &outNodes, int &outLeaves, int &outMaxDepth) const
{
    outNodes = 0;
    outLeaves = 0;
    outMaxDepth = 0;
    getStatsRecursive(root, outNodes, outLeaves, outMaxDepth);
}

int Quadtree::getNodeCount() const
{
    int nodes, leaves, depth;
    getStats(nodes, leaves, depth);
    return nodes;
}

int Quadtree::getMaxDepthReached() const
{
    int nodes, leaves, depth;
    getStats(nodes, leaves, depth);
    return depth;
}

 // ==================== OctreeNode ====================

OctreeNode::OctreeNode(const BoundingBox &bounds, int depth)
    : bounds(bounds), depth(depth), isLeaf(true)
{
    for (int i = 0; i < 8; i++)
    {
        children[i] = nullptr;
    }
}

OctreeNode::~OctreeNode()
{
    for (int i = 0; i < 8; i++)
    {
        delete children[i];
    }
}

void OctreeNode::split()
{
    if (!isLeaf)
        return;

    Vec3 center = bounds.center();

    // Criar 8 octantes
    // Ordem: [x][y][z] onde 0=min, 1=max
    // 0: min,min,min | 1: max,min,min
    // 2: min,max,min | 3: max,max,min
    // 4: min,min,max | 5: max,min,max
    // 6: min,max,max | 7: max,max,max

    for (int i = 0; i < 8; i++)
    {
        Vec3 childMin, childMax;

        childMin.x = (i & 1) ? center.x : bounds.min.x;
        childMax.x = (i & 1) ? bounds.max.x : center.x;

        childMin.y = (i & 2) ? center.y : bounds.min.y;
        childMax.y = (i & 2) ? bounds.max.y : center.y;

        childMin.z = (i & 4) ? center.z : bounds.min.z;
        childMax.z = (i & 4) ? bounds.max.z : center.z;

        children[i] = new OctreeNode(BoundingBox(childMin, childMax), depth + 1);
    }

    isLeaf = false;
}

bool OctreeNode::containsTriangle(const Triangle &tri) const
{
    return bounds.contains(tri.v0) &&
           bounds.contains(tri.v1) &&
           bounds.contains(tri.v2);
}

bool OctreeNode::intersectsTriangle(const Triangle &tri) const
{
    Vec3 triMin, triMax;
    tri.getBounds(triMin, triMax);

    BoundingBox triBounds(triMin, triMax);

    // BoundingBox intersection test
    if (bounds.min.x > triBounds.max.x || triBounds.min.x > bounds.max.x)
        return false;
    if (bounds.min.y > triBounds.max.y || triBounds.min.y > bounds.max.y)
        return false;
    if (bounds.min.z > triBounds.max.z || triBounds.min.z > bounds.max.z)
        return false;

    return true;
}

// ==================== Octree ====================

Octree::Octree(const Vec3 &min, const Vec3 &max, int maxDepth, int maxTrisPerNode)
    : maxDepth(maxDepth), maxTrianglesPerNode(maxTrisPerNode), totalTriangles(0)
{
    root = new OctreeNode(BoundingBox(min, max), 0);
}

Octree::Octree(const BoundingBox &bounds, int maxDepth, int maxTrisPerNode)
    : maxDepth(maxDepth), maxTrianglesPerNode(maxTrisPerNode), totalTriangles(0)
{
    root = new OctreeNode(bounds, 0);
}

Octree::~Octree()
{
    delete root;
}

void Octree::insertRecursive(OctreeNode *node, const Triangle &tri)
{
    if (!node->intersectsTriangle(tri))
    {
        return;
    }

    if (node->isLeaf)
    {
        node->triangles.push_back(tri);

        if (static_cast<int>(node->triangles.size()) > maxTrianglesPerNode &&
            node->depth < maxDepth)
        {
            node->split();

            std::vector<Triangle> oldTriangles = node->triangles;
            node->triangles.clear();

            for (const auto &oldTri : oldTriangles)
            {
                bool addedToChild = false;
                for (int i = 0; i < 8; i++)
                {
                    if (node->children[i]->containsTriangle(oldTri))
                    {
                        insertRecursive(node->children[i], oldTri);
                        addedToChild = true;
                        break;
                    }
                }

                if (!addedToChild)
                {
                    node->triangles.push_back(oldTri);
                }
            }
        }
    }
    else
    {
        bool addedToChild = false;
        for (int i = 0; i < 8; i++)
        {
            if (node->children[i]->containsTriangle(tri))
            {
                insertRecursive(node->children[i], tri);
                addedToChild = true;
                break;
            }
        }

        if (!addedToChild)
        {
            node->triangles.push_back(tri);
        }
    }
}

void Octree::insert(const Triangle &tri)
{
    insertRecursive(root, tri);
    totalTriangles++;
}

void Octree::insert(const std::vector<Triangle> &tris)
{
    for (const auto &tri : tris)
    {
        insert(tri);
    }
}

void Octree::build(const std::vector<Triangle> &tris)
{
    clear();
    insert(tris);
}

void Octree::clear()
{
    delete root;
    root = new OctreeNode(root->bounds, 0);
    totalTriangles = 0;
}

void Octree::rebuild()
{
    std::vector<Triangle> allTriangles;
    std::vector<const Triangle *> queryResult;
    query(root->bounds, queryResult);

    for (auto *tri : queryResult)
    {
        allTriangles.push_back(*tri);
    }

    build(allTriangles);
}

void Octree::queryRecursive(OctreeNode *node, const BoundingBox &queryBounds,
                            std::vector<const Triangle *> &outTriangles) const
{
    if (!node)
        return;

    if (node->bounds.min.x > queryBounds.max.x || queryBounds.min.x > node->bounds.max.x)
        return;
    if (node->bounds.min.y > queryBounds.max.y || queryBounds.min.y > node->bounds.max.y)
        return;
    if (node->bounds.min.z > queryBounds.max.z || queryBounds.min.z > node->bounds.max.z)
        return;

    for (const auto &tri : node->triangles)
    {
        outTriangles.push_back(&tri);
    }

    if (!node->isLeaf)
    {
        for (int i = 0; i < 8; i++)
        {
            queryRecursive(node->children[i], queryBounds, outTriangles);
        }
    }
}

void Octree::queryRecursive(OctreeNode *node, const Vec3 &point,
                            std::vector<const Triangle *> &outTriangles) const
{
    if (!node || !node->bounds.contains(point))
        return;

    for (const auto &tri : node->triangles)
    {
        outTriangles.push_back(&tri);
    }

    if (!node->isLeaf)
    {
        for (int i = 0; i < 8; i++)
        {
            queryRecursive(node->children[i], point, outTriangles);
        }
    }
}

void Octree::queryRecursive(OctreeNode *node, const Vec3 &sphereCenter,
                            float sphereRadius,
                            std::vector<const Triangle *> &outTriangles) const
{
    if (!node)
        return;

    Vec3 closestPoint = Vec3(
        std::fmax(node->bounds.min.x, std::fmin(sphereCenter.x, node->bounds.max.x)),
        std::fmax(node->bounds.min.y, std::fmin(sphereCenter.y, node->bounds.max.y)),
        std::fmax(node->bounds.min.z, std::fmin(sphereCenter.z, node->bounds.max.z)));

    float distSq = Vec3::DistanceSquared(closestPoint, sphereCenter);
    if (distSq > sphereRadius * sphereRadius)
    {
        return;
    }

    for (const auto &tri : node->triangles)
    {
        outTriangles.push_back(&tri);
    }

    if (!node->isLeaf)
    {
        for (int i = 0; i < 8; i++)
        {
            queryRecursive(node->children[i], sphereCenter, sphereRadius, outTriangles);
        }
    }
}

void Octree::query(const BoundingBox &queryBounds, std::vector<const Triangle *> &outTriangles) const
{
    outTriangles.clear();
    queryRecursive(root, queryBounds, outTriangles);
}

void Octree::query(const Vec3 &point, float radius, std::vector<const Triangle *> &outTriangles) const
{
    outTriangles.clear();
    BoundingBox queryBounds(point - Vec3(radius, radius, radius),
                            point + Vec3(radius, radius, radius));
    queryRecursive(root, queryBounds, outTriangles);
}

void Octree::querySphere(const Vec3 &center, float radius,
                         std::vector<const Triangle *> &outTriangles) const
{
    outTriangles.clear();
    queryRecursive(root, center, radius, outTriangles);
}

void Octree::queryRay(const Vec3 &origin, const Vec3 &direction, float maxDistance,
                      std::vector<const Triangle *> &outTriangles) const
{
    outTriangles.clear();

    Vec3 end = origin + direction * maxDistance;
    Vec3 min = Vec3(
        std::fmin(origin.x, end.x),
        std::fmin(origin.y, end.y),
        std::fmin(origin.z, end.z));
    Vec3 max = Vec3(
        std::fmax(origin.x, end.x),
        std::fmax(origin.y, end.y),
        std::fmax(origin.z, end.z));

    BoundingBox rayBounds(min, max);
    queryRecursive(root, rayBounds, outTriangles);
}

void Octree::queryFrustumRecursive(OctreeNode *node, const Frustum &frustum,
                                   std::vector<const Triangle *> &outTriangles) const
{
    if (!node)
        return;

    // Testar se o bounds do node está no frustum
    if (!frustum.intersectsAABB(node->bounds))
        return;

    for (const auto &tri : node->triangles)
    {
        outTriangles.push_back(&tri);
    }

    if (!node->isLeaf)
    {
        for (int i = 0; i < 8; i++)
        {
            queryFrustumRecursive(node->children[i], frustum, outTriangles);
        }
    }
}

void Octree::queryFrustum(const Frustum &frustum, std::vector<const Triangle *> &outTriangles) const
{
    outTriangles.clear();
    queryFrustumRecursive(root, frustum, outTriangles);
}

int Octree::getTotalTriangles() const
{
    return totalTriangles;
}

void Octree::getStatsRecursive(OctreeNode *node, int &outNodes, int &outLeaves,
                               int &outMaxDepth) const
{
    if (!node)
        return;

    outNodes++;
    if (node->depth > outMaxDepth)
    {
        outMaxDepth = node->depth;
    }

    if (node->isLeaf)
    {
        outLeaves++;
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            getStatsRecursive(node->children[i], outNodes, outLeaves, outMaxDepth);
        }
    }
}

void Octree::getStats(int &outNodes, int &outLeaves, int &outMaxDepth) const
{
    outNodes = 0;
    outLeaves = 0;
    outMaxDepth = 0;
    getStatsRecursive(root, outNodes, outLeaves, outMaxDepth);
}

int Octree::getNodeCount() const
{
    int nodes, leaves, depth;
    getStats(nodes, leaves, depth);
    return nodes;
}

int Octree::getMaxDepthReached() const
{
    int nodes, leaves, depth;
    getStats(nodes, leaves, depth);
    return depth;
}

float Octree::getMemoryUsage() const
{
    int nodes, leaves, depth;
    getStats(nodes, leaves, depth);
    
    // Aproximação: cada node tem um overhead + triangles
    float nodeSize = sizeof(OctreeNode);
    float triangleSize = sizeof(Triangle);
    
    float totalMemory = nodes * nodeSize + totalTriangles * triangleSize;
    return totalMemory / 1024.0f; // KB
}

 
 