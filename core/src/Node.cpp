#include "pch.h"
#include "Node.hpp"
#include "Utils.hpp"

void Node::serialize(Serialize &serialize)
{
    Object::serialize(serialize);
    serialize.SetInt("renderType", (int)getRenderType());
}

void Node::deserialize(const Serialize &in)
{
    Object::deserialize(in);
    setRenderType((RenderType)in.GetInt("renderType"));
}

Node::Node(const std::string &name)
    : Object(name)
{
}

Node::~Node()
{
}

void Node::show()
{
    setActive(true);
}

void Node::hide()
{
    setActive(false);
}

void Node::setRenderType(RenderType renderType)
{
    m_renderType = renderType;
}
