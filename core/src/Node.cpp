#include "pch.h"
#include "Node.hpp"
#include "Utils.hpp"

Node::Node(const std::string& name)
    :Object(name),  parent(nullptr),   insideTree(false)
{
}

Node::~Node()
{
    if (parent)
    {
        parent->removeChild(this);
    }
    
    for (Node* child : children)
    {
        child->parent = nullptr;
    }
}

// ==================== Hierarchy ====================

void Node::setParent(Node* newParent)
{
    if (parent == newParent)
        return;
    
    if (newParent && newParent->isDescendantOf(this))
    {
        LogError("[Node] Cannot set parent to descendant (would create cycle)");
        
        return;
    }
    
    bool wasInTree = insideTree;
    
    if (wasInTree)
        _propagateExitTree();
    
    if (parent)
    {
        parent->removeChild(this);
    }
    
    parent = newParent;
    
    if (parent)
    {
        parent->addChild(this);
        
        if (parent->isInsideTree() && !insideTree)
            _propagateEnterTree();
    }
}

Node* Node::getParent() const
{
    return parent;
}

void Node::removeParent()
{
    setParent(nullptr);
}

void Node::addChild(Node* child)
{
    if (!child || child == this)
        return;
    
    auto it = std::find(children.begin(), children.end(), child);
    if (it == children.end())
    {
        children.push_back(child);
    }
}

void Node::removeChild(Node* child)
{
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
    }
}

void Node::removeAllChildren()
{
    for (Node* child : children)
    {
        child->parent = nullptr;
    }
    children.clear();
}

const std::vector<Node*>& Node::getChildren() const
{
    return children;
}

int Node::getChildCount() const
{
    return static_cast<int>(children.size());
}

Node* Node::getChild(int index) const
{
    if (index >= 0 && index < static_cast<int>(children.size()))
        return children[index];
    return nullptr;
}

Node* Node::findChild(const std::string& name) const
{
    for (Node* child : children)
    {
        if (child->m_name == name)
            return child;
    }
    return nullptr;
}

Node* Node::findChildRecursive(const std::string& name) const
{
    for (Node* child : children)
    {
        if (child->m_name == name)
            return child;
        
        Node* found = child->findChildRecursive(name);
        if (found)
            return found;
    }
    return nullptr;
}

bool Node::isAncestorOf(const Node* node) const
{
    if (!node)
        return false;
    
    const Node* current = node->parent;
    while (current)
    {
        if (current == this)
            return true;
        current = current->parent;
    }
    return false;
}

bool Node::isDescendantOf(const Node* node) const
{
    if (!node)
        return false;
    return node->isAncestorOf(this);
}

// ==================== Lifecycle ====================

void Node::propagateUpdate(float deltaTime)
{
    if (!m_active)
        return;
    
    update(deltaTime);
    
    for (Node* child : children)
    {
        child->propagateUpdate(deltaTime);
    }
}

void Node::propagateRender()
{
    if (!m_active)
        return;
    
    render();
    
    for (Node* child : children)
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

 

std::string Node::getPath() const
{
    if (!parent)
        return "/" + m_name;
    
    return parent->getPath() + "/" + m_name;
}

// ==================== Utilities ====================

void Node::setRenderType(RenderType renderType)
{
    m_renderType=renderType;
}


void Node::queueFree()
{
    if (parent)
    {
        parent->removeChild(this);
    }
    delete this;
}

Node* Node::getRoot()
{
    Node* current = this;
    while (current->parent)
    {
        current = current->parent;
    }
    return current;
}

int Node::getDepth() const
{
    int depth = 0;
    const Node* current = parent;
    while (current)
    {
        depth++;
        current = current->parent;
    }
    return depth;
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
    
    for (Node* child : children)
    {
        child->_propagateEnterTree();
    }
}

void Node::_propagateExitTree()
{
    _exitTree();
    
    for (Node* child : children)
    {
        child->_propagateExitTree();
    }
}