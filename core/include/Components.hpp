#pragma once
#include "Config.hpp"
#include "Component.hpp"



class Mesh;

// ============================================================================
// MeshRenderer - Renders a mesh
// ============================================================================


class MeshRenderer : public Component
{
public:
    MeshRenderer(Mesh *m);

    const char *getTypeName() const override { return "MeshRenderer"; }

    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }

    void setMesh(Mesh *m) { mesh = m; }

    Mesh *getMesh() const { return mesh; }

    void attach() override;

    void render() override;

private:
    Mesh *mesh{nullptr};
    bool visible;
};

// ============================================================================
// Rotator - Simple rotation component (example behavior)
// ============================================================================

class Rotator : public Component
{
private:
    Vec3 rotationSpeed; // Degrees per second

public:
    Rotator() : rotationSpeed(0, 45, 0) {} // Default: 45°/s on Y axis

    Rotator(const Vec3 &speed) : rotationSpeed(speed) {}

    const char *getTypeName() const override { return "Rotator"; }

    void setRotationSpeed(const Vec3 &speed) { rotationSpeed = speed; }
    Vec3 getRotationSpeed() const { return rotationSpeed; }

    void update(float deltaTime) override;
 
};

// ============================================================================
// Oscillator - Moves object back and forth (example behavior)
// ============================================================================

class Oscillator : public Component
{
private:
    Vec3 amplitude;
    float frequency;
    float time;
    Vec3 startPosition;

public:
    Oscillator()
        : amplitude(0, 1, 0), // Default: 1m vertical oscillation
          frequency(1.0f),
          time(0.0f)
    {
    }

    Oscillator(const Vec3 &amp, float freq)
        : amplitude(amp),
          frequency(freq),
          time(0.0f)
    {
    }

    const char *getTypeName() const override { return "Oscillator"; }

    void setAmplitude(const Vec3 &amp) { amplitude = amp; }
    Vec3 getAmplitude() const { return amplitude; }

    void setFrequency(float freq) { frequency = freq; }
    float getFrequency() const { return frequency; }

    void start() override;
 

    void update(float deltaTime) override;
   
};

// ============================================================================
// LookAtCamera - Always faces the camera (billboard)
// ============================================================================

class LookAtCamera : public Component
{
private:
    Camera3D *targetCamera;

public:
    LookAtCamera() : targetCamera(nullptr) {}

    const char *getTypeName() const override { return "LookAtCamera"; }

    void setCamera(Camera3D *camera) { targetCamera = camera; }
    Camera3D *getCamera() const { return targetCamera; }

    void lateUpdate(float deltaTime) override;

};