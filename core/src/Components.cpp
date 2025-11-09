#include "pch.h"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Math.hpp"
#include "Node.hpp"
#include "Node3D.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Mesh.hpp"

void MeshRenderer::render()
{
    if (!visible || !isEnabled())
        return;
    if (mesh)
    {
        // mesh->render();
    }
}

MeshRenderer::MeshRenderer(Mesh *m) : mesh(m), visible(true)
{
}

void MeshRenderer::attach()
{
    // m_owner->getBoundingBox().merge(mesh->getBoundingBox());
}

void Rotator::update(float deltaTime)
{
    if (!isEnabled())
        return;

    // Rotate GameObject
    GameObject *owner = getOwner();
    if (owner)
    {
        owner->rotateLocalX(rotationSpeed.x * deltaTime);
        owner->rotateLocalY(rotationSpeed.y * deltaTime);
        owner->rotateLocalZ(rotationSpeed.z * deltaTime);
    }
}

void Oscillator::start()

{
    // Store initial position
    GameObject *owner = getOwner();
    if (owner)
    {
        startPosition = owner->getLocalPosition();
    }
}
void Oscillator::update(float deltaTime)
{
    if (!isEnabled())
        return;

    time += deltaTime;

    GameObject *owner = getOwner();
    if (owner)
    {
        // Sine wave oscillation
        float wave = std::sin(time * frequency * 2.0f * 3.14159f);
        Vec3 offset = amplitude * wave;
        owner->setLocalPosition(startPosition + offset);
    }
}

void LookAtCamera::lateUpdate(float deltaTime)
{
    if (!isEnabled() || !targetCamera)
        return;

    GameObject *owner = getOwner();
    if (owner)
    {
        Vec3 cameraPos = targetCamera->getPosition();
        owner->lookAt(cameraPos);
    }
}