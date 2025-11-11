#pragma once
#include "Config.hpp"
#include "Node.hpp"
#include "Math.hpp"

/**
 * @brief 3D spatial node with transformation capabilities
 *
 * Node3D extends Node with 3D position, rotation, and scale.
 * All 3D objects (cameras, meshes, lights) inherit from Node3D.
 */
class Node3D : public Node
{
protected:
	bool transformDirty;

	BoundingBox m_boundingBox;

	void updateAbsolutePosition() ;
	void markTransformDirty();

	Mat4 AbsoluteTransformation;
	Vec3 RelativeTranslation;
	Vec3 RelativeRotation;
	Vec3 RelativeScale;

	Node3D *Parent;

public:
	Node3D(const std::string &name = "Node3D");

	virtual ~Node3D() {}

	virtual ObjectType getType() override { return ObjectType::Node3D; }

	BoundingBox &getBoundingBox();
	const BoundingBox getBoundingBox() const;
	const BoundingBox getTransformedBoundingBox() const;

	virtual Mat4 getRelativeTransformation() ;
	virtual const Mat4 &getAbsoluteTransformation() ;
 

	Mat4 &getWorldMatrix();

	virtual const Vec3 &getScale() const
	{
		return RelativeScale;
	}

	virtual void setScale(const Vec3 &scale)
	{
		RelativeScale = scale;
	}

	virtual const Vec3 &getRotation() const
	{
		return RelativeRotation;
	}

	virtual void setRotation(const Vec3 &rotation)
	{
		RelativeRotation = rotation;
	}

	virtual const Vec3 &getPosition() const
	{
		return RelativeTranslation;
	}

	virtual void setPosition(float x, float y, float z)
	{
		RelativeTranslation.x = x;
		RelativeTranslation.y = y;
		RelativeTranslation.z = z;
		updateAbsolutePosition();
	}

	virtual void setPosition(const Vec3 &newpos)
	{
		RelativeTranslation = newpos;
		updateAbsolutePosition();
	}

	virtual Vec3 getAbsolutePosition() const
	{
		return AbsoluteTransformation.getTranslation();
	}

	Node3D *getParent() const;
	void setParent(Node3D *parent);
};