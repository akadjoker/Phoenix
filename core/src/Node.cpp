#include "pch.h"
#include "Node.hpp"
#include "Utils.hpp"

Node::Node(const std::string &name)
    : Object(name), insideTree(false)
{
}

Node::~Node()
{
    children.clear();
}

// ==================== Hierarchy ====================

void Node::addChild(Node *child)
{
    if (!child || child == this)
        return;

    children.push_back(child);
}

void Node::removeChild(Node *child)
{
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
    }
}

void Node::removeAllChildren()
{

    children.clear();
}

const std::vector<Node *> &Node::getChildren() const
{
    return children;
}

int Node::getChildCount() const
{
    return static_cast<int>(children.size());
}

Node *Node::getChild(int index) const
{
    if (index >= 0 && index < static_cast<int>(children.size()))
        return children[index];
    return nullptr;
}

Node *Node::findChild(const std::string &name) const
{
    for (Node *child : children)
    {
        if (child->m_name == name)
            return child;
    }
    return nullptr;
}

Node *Node::findChildRecursive(const std::string &name) const
{
    for (Node *child : children)
    {
        if (child->m_name == name)
            return child;

        Node *found = child->findChildRecursive(name);
        if (found)
            return found;
    }
    return nullptr;
}

// ==================== Lifecycle ====================

void Node::propagateUpdate(float deltaTime)
{
    if (!m_active)
        return;

    update(deltaTime);

    for (Node *child : children)
    {
        child->propagateUpdate(deltaTime);
    }
}

void Node::propagateRender()
{
    if (!m_active)
        return;

    render();

    for (Node *child : children)
    {
        child->propagateRender();
    }
}

// ==================== Active State ====================

bool Node::isInsideTree() const
{
    return insideTree;
}

void Node::show()
{
    setActive(true);
}

void Node::hide()
{
    setActive(false);
}

// ==================== Name ====================

// ==================== Utilities ====================

void Node::setRenderType(RenderType renderType)
{
    m_renderType = renderType;
}

// ==================== Protected ====================

void Node::_enterTree()
{
    insideTree = true;
    ready();
}

void Node::_exitTree()
{
    insideTree = false;
}

void Node::_propagateEnterTree()
{
    _enterTree();

    for (Node *child : children)
    {
        child->_propagateEnterTree();
    }
}

void Node::_propagateExitTree()
{
    _exitTree();

    for (Node *child : children)
    {
        child->_propagateExitTree();
    }
}