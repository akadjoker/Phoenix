#pragma once

#include "Config.hpp"
#include "Object.hpp"
#include <string>
#include <vector>
#include <algorithm>

/**
 * @brief Base class for all nodes in the scene graph
 *
 * Node provides the fundamental hierarchy system used throughout the engine.
 * All objects in the scene (3D objects, cameras, lights, UI elements) inherit from Node.
 */

enum class RenderType : u8
{
    Solid = 0,
    Terrain,
    Trasparent,
    Special,
    Mirror,
    Water,
    Sky,
    Light
};

class Node : public Object
{
protected:
    std::vector<Node *> children;

    bool insideTree;
    RenderType m_renderType;

public:
    Node(const std::string &name = "Node");
    virtual ~Node();

    virtual ObjectType getType() override { return ObjectType::Node; }
    RenderType getRenderType() const { return m_renderType; };
    void setRenderType(RenderType renderType);

    // ==================== Hierarchy ====================

    void addChild(Node *child);
    void removeChild(Node *child);
    void removeAllChildren();

    const std::vector<Node *> &getChildren() const;
    int getChildCount() const;
    Node *getChild(int index) const;

    Node *findChild(const std::string &name) const;
    Node *findChildRecursive(const std::string &name) const;

    bool isAncestorOf(const Node *node) const;
    bool isDescendantOf(const Node *node) const;

    // ==================== Lifecycle ====================

    virtual void ready() {}
    virtual void update(float deltaTime) {}
    virtual void render() {}

    void propagateUpdate(float deltaTime);
    void propagateRender();

    // ==================== Active State ====================

    bool isInsideTree() const;

    void show();
    void hide();

    // ==================== Name ====================

    std::string getPath() const;

    // ==================== Utilities ====================

    void queueFree();
    Node *getRoot();
    int getDepth() const;

protected:
    virtual void _enterTree();
    virtual void _exitTree();

    void _propagateEnterTree();
    void _propagateExitTree();
};