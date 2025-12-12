#pragma once
#include <vector>
#include <map>
#include "Math.hpp"

struct NavNode
{
    Vec3 position;
    float radius = 1.0f; // Área navegável ao redor

    NavNode(const Vec3 &pos, float r = 1.0f)
        : position(pos), radius(r) {}
};

class NavMesh
{
private:
    std::vector<NavNode> m_nodes;
    std::map<int, std::vector<int>> m_connections;  

public:
 
    int addNode(const Vec3 &position, float radius = 1.0f);
    void connectNodes(int nodeA, int nodeB);
    void autoConnect(float maxDistance = 15.0f); 

 
    int findNearestNode(const Vec3 &position) const;
    bool isConnected(int nodeA, int nodeB) const;
    const std::vector<int> &getConnections(int nodeIndex) const;
    const NavNode &getNode(int index) const;
    int getNodeCount() const { return (int)m_nodes.size(); }

    // Pathfinding
    std::vector<Vec3> findPath(const Vec3 &start, const Vec3 &goal);
    std::vector<int> findPathIndices(int startNode, int goalNode);



 
    void save(const char *filename);
    void load(const char *filename);
};
