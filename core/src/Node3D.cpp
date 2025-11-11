#include "pch.h"
#include "Node3D.hpp"

Node3D::Node3D(const std::string &name)
    : Node(name),

      transformDirty(true)
{
    Parent = nullptr;
    m_boundingBox.min = Vec3(-0.5f, -0.5f, -0.5f);
    m_boundingBox.max = Vec3(0.5f, 0.5f, 0.5f);
    RelativeRotation = Vec3(0.0f, 0.0f, 0.0f);
    RelativeScale = Vec3(1.0f, 1.0f, 1.0f);
    RelativeTranslation = Vec3(0.0f, 0.0f, 0.0f);
}

Mat4 Node3D::getRelativeTransformation() 
{
    Mat4 mat;
    mat.setRotationDegrees(RelativeRotation);
    mat.setTranslation(RelativeTranslation);

    if (RelativeScale != Vec3(1.f, 1.f, 1.f))
    {
        Mat4 smat;
        smat.setScale(RelativeScale);
        mat *= smat;
    }

    return mat;
}

const Mat4 &Node3D::getAbsoluteTransformation() 
{
  //  if (transformDirty)
       updateAbsolutePosition();
    return AbsoluteTransformation;
}

Mat4 &Node3D::getWorldMatrix()
{

    updateAbsolutePosition();
    return AbsoluteTransformation;
}

void Node3D::updateAbsolutePosition() 
{
   //  if (!transformDirty) return; // Early out

 
    if (Parent)
    {
        AbsoluteTransformation = Parent->getAbsoluteTransformation() * getRelativeTransformation();
    }
    else
    {
        AbsoluteTransformation = getRelativeTransformation();
    }

    for (auto *child : children)
    {
        Node3D *child3D = dynamic_cast<Node3D *>(child);
        if (child3D)
        {
            child3D->updateAbsolutePosition();
        }
         
    }
}

void Node3D::markTransformDirty()
{
    transformDirty = true;

    for (Node *child : children)
    {
        Node3D *child3D = dynamic_cast<Node3D *>(child);
        if (child3D)
        {
            child3D->markTransformDirty();
        }
    }
}

Node3D *Node3D::getParent() const
{
    return Parent;
}

void Node3D::setParent(Node3D *parent) 
{
    Parent = parent;
}

BoundingBox &Node3D::getBoundingBox()
{
    return m_boundingBox;
}

const BoundingBox Node3D::getBoundingBox() const
{
    return m_boundingBox;
}

const BoundingBox Node3D::getTransformedBoundingBox() const
{
    return BoundingBox::Transform(getBoundingBox(), AbsoluteTransformation);
}