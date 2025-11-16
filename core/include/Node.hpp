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
    RenderType m_renderType;

public:
    Node(const std::string &name = "Node");
    virtual ~Node();

    virtual ObjectType getType() override { return ObjectType::Node; }
    RenderType getRenderType() const { return m_renderType; };
    void setRenderType(RenderType renderType);

    virtual void ready() {}
    virtual void update(float deltaTime) {}
    virtual void render() {}

    void show();
    void hide();

    virtual void serialize(Serialize &serialize);
    virtual void deserialize(const Serialize &in);

protected:
};