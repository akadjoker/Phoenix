#include "pch.h"
#include "Math.hpp"
#include "NavMesh.hpp"
#include <limits>
#include <queue>

int NavMesh::addNode(const Vec3& position, float radius)
{
    m_nodes.push_back(NavNode(position, radius));
    return (int)m_nodes.size() - 1;
}

void NavMesh::connectNodes(int nodeA, int nodeB)
{
    if (nodeA >= (int)m_nodes.size() || nodeB >= (int)m_nodes.size())
        return;
    
    // Bidirecional
    m_connections[nodeA].push_back(nodeB);
    m_connections[nodeB].push_back(nodeA);
}

void NavMesh::autoConnect(float maxDistance)
{
    m_connections.clear();
    
    for (int i = 0; i < (int)m_nodes.size(); i++)
    {
        for (int j = i + 1; j < (int)m_nodes.size(); j++)
        {
            float dist = (m_nodes[i].position - m_nodes[j].position).length();
            
            if (dist <= maxDistance)
            {
                connectNodes(i, j);
            }
        }
    }
}

int NavMesh::findNearestNode(const Vec3& position) const
{
    if (m_nodes.empty())
        return -1;
    
    int nearest = 0;
    float minDist = (m_nodes[0].position - position).length();
    
    for (int i = 1; i < (int)m_nodes.size(); i++)
    {
        float dist = (m_nodes[i].position - position).length();
        if (dist < minDist)
        {
            minDist = dist;
            nearest = i;
        }
    }
    
    return nearest;
}

bool NavMesh::isConnected(int nodeA, int nodeB) const
{
    auto it = m_connections.find(nodeA);
    if (it == m_connections.end())
        return false;
    
    const auto& neighbors = it->second;
    return std::find(neighbors.begin(), neighbors.end(), nodeB) != neighbors.end();
}

const std::vector<int>& NavMesh::getConnections(int nodeIndex) const
{
    static std::vector<int> empty;
    
    auto it = m_connections.find(nodeIndex);
    if (it == m_connections.end())
        return empty;
    
    return it->second;
}

const NavNode& NavMesh::getNode(int index) const
{
    return m_nodes[index];
}

// Estrutura para A*
struct PathNode
{
    int index;
    float g; // Custo do início até aqui
    float h; // Heurística até o objetivo
    float f; // g + h
    int parent;
    
    PathNode(int idx, float gCost, float hCost, int p)
        : index(idx), g(gCost), h(hCost), f(gCost + hCost), parent(p) {}
    
    bool operator>(const PathNode& other) const
    {
        return f > other.f;
    }
};

std::vector<int> NavMesh::findPathIndices(int startNode, int goalNode)
{
    std::vector<int> path;
    
    if (startNode < 0 || goalNode < 0 || 
        startNode >= (int)m_nodes.size() || 
        goalNode >= (int)m_nodes.size())
    {
        return path; // Inválido
    }
    
    if (startNode == goalNode)
    {
        path.push_back(startNode);
        return path;
    }
    
    // Priority queue (min-heap)
    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> openSet;
    
    std::vector<bool> closedSet(m_nodes.size(), false);
    std::vector<float> gScore(m_nodes.size(), std::numeric_limits<float>::infinity());
    std::vector<int> cameFrom(m_nodes.size(), -1);
    
    // Heurística: distância euclidiana
    auto heuristic = [this, goalNode](int nodeIdx) -> float
    {
        return (m_nodes[nodeIdx].position - m_nodes[goalNode].position).length();
    };
    
    // Inicializa
    gScore[startNode] = 0.0f;
    openSet.push(PathNode(startNode, 0.0f, heuristic(startNode), -1));
    
    while (!openSet.empty())
    {
        PathNode current = openSet.top();
        openSet.pop();
        
        int currentIdx = current.index;
        
        // Já processado?
        if (closedSet[currentIdx])
            continue;
        
        closedSet[currentIdx] = true;
        
        // Chegou ao objetivo?
        if (currentIdx == goalNode)
        {
            // Reconstrói o caminho
            int node = goalNode;
            while (node != -1)
            {
                path.push_back(node);
                node = cameFrom[node];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        // Explora vizinhos
        const auto& neighbors = getConnections(currentIdx);
        
        for (int neighborIdx : neighbors)
        {
            if (closedSet[neighborIdx])
                continue;
            
            float edgeCost = (m_nodes[currentIdx].position - 
                             m_nodes[neighborIdx].position).length();
            float tentativeG = gScore[currentIdx] + edgeCost;
            
            if (tentativeG < gScore[neighborIdx])
            {
                // Caminho melhor encontrado
                gScore[neighborIdx] = tentativeG;
                cameFrom[neighborIdx] = currentIdx;
                
                float h = heuristic(neighborIdx);
                openSet.push(PathNode(neighborIdx, tentativeG, h, currentIdx));
            }
        }
    }
    
    // Não encontrou caminho
    return path;
}

std::vector<Vec3> NavMesh::findPath(const Vec3& start, const Vec3& goal)
{
    std::vector<Vec3> path;

    int startNode = findNearestNode(start);
    int goalNode  = findNearestNode(goal);
    
    if (startNode < 0 || goalNode < 0)
        return path;

    std::vector<int> nodeIndices = findPathIndices(startNode, goalNode);

    // Se não houver caminho, devolve vazio
    if (nodeIndices.empty())
        return path;

    // Adiciona posição inicial exata
    path.push_back(start);

     
    for (size_t i = 1; i < nodeIndices.size(); ++i)
    {
        path.push_back(m_nodes[nodeIndices[i]].position);
    }

 
    path.push_back(goal);

    return path;
}
