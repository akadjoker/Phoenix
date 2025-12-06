#pragma once
#include "Config.hpp"
#include "Component.hpp"



class Mesh;
class Terrain;
class Animator;
class M3dMesh;
class Shader;

// ============================================================================
// MeshRenderer - Renders a mesh
// ============================================================================


class MeshRenderer : public Component
{
public:
    MeshRenderer(Mesh *m);
    ~MeshRenderer();

    const char *getTypeName() const override { return "MeshRenderer"; }

    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }

    void setMesh(Mesh *m) { mesh = m; }

    Mesh *getMesh() const { return mesh; }

    void attach() override;

    void render(Shader *shader, bool useMaterial) override;

    

private:
    friend class Animator;
    Mesh *mesh{nullptr};
    bool visible;
    std::vector<Mat4> boneMatrices;
 
     void UpdateSkinning();
};



class MeshM3DRenderer : public Component
{
public:
    MeshM3DRenderer(MeshM3D *m);
    ~MeshM3DRenderer();

    const char *getTypeName() const override { return "MeshM3DRenderer"; }

    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }

    void setMesh(MeshM3D *m) { mesh = m; }
    MeshM3D *getMesh() const { return mesh; }

    void attach() override;

    void render( Shader *shader, bool useMaterial) override;

    

private:
    
    MeshM3D *mesh{nullptr};
    bool visible;
    
};


// ============================================================================
// Vertex Animator - Animation component
// ============================================================================

struct VertexAnimation
{
    std::string name;
    int startFrame;
    int endFrame;
    float fps;
    bool loop;

    VertexAnimation(const std::string& n, int start, int end, float f, bool l = true)
        : name(n), startFrame(start), endFrame(end), fps(f), loop(l) {}
};




class VertexAnimator : public Component
{
public:
    VertexAnimator();
    ~VertexAnimator();


    const char *getTypeName() const override { return "VertexAnimator"; }

    void AddAnimation(const std::string& name, int startFrame, int endFrame, float fps, bool loop = true);
    void PlayAnimation(const std::string& name, bool forceRestart = false);
    void PlayAnimationThen(const std::string& name, const std::string& nextAnim, bool forceRestart = false);
    void PlayAnimationSequence(const std::vector<std::string>& animationNames);
    void TransitionToAnimation(const std::string& name, float duration = 0.3f);
    void GetCurrentFrames(int& currentFrame, int& nextFrame, float& interpolation);

    bool IsPlaying() const;
    bool IsPlaying(const std::string& name) const;
    std::string GetCurrentAnimation() const;
    std::string GetQueuedAnimation() const;
    u32 GetAnimationCount() const;
    u32 GetFrame() const;
    
    void Stop();
    void Pause();
    void Resume();


     void attach() override;
     void update(float deltaTime) override;

    private:
    std::map<std::string, VertexAnimation*> animations;
    std::string currentAnimation;
    std::string nextAnimation;
    std::string trasitionName;
    std::string queuedAnimation;

    float currentTime;
    float transitionTime;
    float transitionDuration;

    u32 lastFrame {0};
    u32 m_current_frame {0};
    MeshM3DRenderer* meshRenderer;
    MeshM3D *m_mesh;
    bool m_active;


    enum State 
    {
        STOPPED,
        PLAYING,
        PAUSED,
        TRANSITIONING
    };

    State state;
    State GetState() const;

    void SetTransitionDuration(float duration);
    void ClearCallback();
    bool HasQueuedAnimation() const;
    void UpdateAnimation(float deltaTime);
    void UpdateTransition(float deltaTime);
    void GetTransitionFrames(int& currentFrame, int& nextFrame, float& interpolation);

    
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


class Controller : public Component
{
public:
    Controller();
    virtual ~Controller() = default;

    const char* getTypeName() const override { return "Controller"; }

    void onDestroy() override;
    void start() override;
    void update(float deltaTime) override;

protected:
    virtual void OnUpdate(float deltaTime) = 0;
    virtual void OnStart() {}
    virtual void OnDestroy() {}

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
    Camera *targetCamera;

public:
    LookAtCamera() : targetCamera(nullptr) {}

    const char *getTypeName() const override { return "LookAtCamera"; }

    void setCamera(Camera *camera) { targetCamera = camera; }
    Camera *getCamera() const { return targetCamera; }

    void lateUpdate(float deltaTime) override;

};



class FreeCameraComponent : public Component
{
private:
    Camera* m_camera;
    
    // Movement
    float m_moveSpeed;
    float m_sprintMultiplier;
    float m_slowMultiplier;
    
    // Rotation
    float m_mouseSensitivity;
    float m_pitch;
    float m_yaw;
    bool m_invertY;
    
    // Constraints
    float m_minPitch;
    float m_maxPitch;
    
    // Input state
    Vec3 m_moveInput;
    Vec2 m_rotationInput;
    bool m_isSprinting;
    bool m_isSlowMode;
    
    void updateRotation(float dt);
    void updateMovement(float dt);
    
public:
    FreeCameraComponent();


     const char *getTypeName() const override { return "FreeCamera"; }
    
    void attach() override;
    void update(float dt) override;
    
    // Movement config
    void setMoveSpeed(float speed) { m_moveSpeed = speed; }
    float getMoveSpeed() const { return m_moveSpeed; }
    
    void setSprintMultiplier(float mult) { m_sprintMultiplier = mult; }
    float getSprintMultiplier() const { return m_sprintMultiplier; }
    
    void setSlowMultiplier(float mult) { m_slowMultiplier = mult; }
    float getSlowMultiplier() const { return m_slowMultiplier; }
    
    // Rotation config
    void setMouseSensitivity(float sens) { m_mouseSensitivity = sens; }
    float getMouseSensitivity() const { return m_mouseSensitivity; }
    
    void setInvertY(bool invert) { m_invertY = invert; }
    bool isYInverted() const { return m_invertY; }
    
    void setPitchLimits(float minDeg, float maxDeg);
    

    void setMoveInput(const Vec3& input);      // x=right, y=up, z=forward
    void setRotationInput(const Vec2& delta);  // x=yaw, y=pitch
    void setSprinting(bool sprint) { m_isSprinting = sprint; }
    void setSlowMode(bool slow) { m_isSlowMode = slow; }
    
    // Direct control
    void setPitch(float pitchDeg);
    void setYaw(float yawDeg);
    float getPitch() const { return m_pitch; }
    float getYaw() const { return m_yaw; }
};