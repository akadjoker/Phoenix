#include "pch.h"
#include "Node.hpp"
#include "Utils.hpp"

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
