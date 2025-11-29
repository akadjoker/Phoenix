#pragma once
#include "Config.hpp"
#include "GraphicsTypes.hpp"
#include "LoadTypes.hpp"
#include "Node.hpp"
#include <string>
#include <map>
#include <vector>

class VertexBuffer;
class IndexBuffer;
class VertexArray;
class Mesh;
class MeshBuffer;
class MeshRenderer;
class MeshManager;
class Stream;
class Texture;
class Spatial;
class Object;
class Driver;
class RenderBatch;
class MeshWriter;
class MeshLoader;
class Animator;
class Component;
 
 

struct AnimationKeyframe
{
    float time;
    Vec3 position;
    Quat rotation;

    AnimationKeyframe() : time(0.0f), position(), rotation() {}
};

struct AnimationChannel
{
    std::string boneName;
    s32 boneIndex; // Index no mesh
    std::vector<AnimationKeyframe> keyframes;
};

class Animation
{
public:
    bool Load(const std::string &filename);
   
 
    void Bind(Node3D *parent);

    float GetDuration() const { return m_duration; }
    float GetTicksPerSecond() const { return m_ticksPerSecond; }
    const std::string &GetName() const { return m_name; }

    AnimationChannel *GetChannel(u32 index) { return &m_channels[index]; }
    AnimationChannel *FindChannel(const std::string &name);

    Vec3 InterpolatePosition(const AnimationChannel &channel, float time);
    Quat InterpolateRotation(const AnimationChannel &channel, float time);

    u32 GetChannelCount() const { return m_channels.size(); }

    bool operator==(const Animation &other) const { return m_name == other.m_name; }
    bool operator!=(const Animation &other) const { return !(*this == other); }

private:
    std::string m_name;
    float m_duration;
    float m_ticksPerSecond;
 
    float m_currentTime;
    friend class Animator;
    friend class AnimationLayer;


 

    std::vector<AnimationChannel> m_channels;
    std::unordered_map<std::string, AnimationChannel> m_boneMap;
};


class AnimReader
{
public:
    struct Channel
    {
        std::string boneName;                     // Nome do bone (ex: "mixamorig:LeftArm")
        std::vector<AnimationKeyframe> keyframes; // Keyframes ao longo do tempo
    };

    struct FrameAnimation
    {
        std::string name;              // Nome da animação (ex: "idle", "walk", "run")
        float duration;                // Duração total em segundos (ex: 2.5s)
        float ticksPerSecond;          // FPS/ticks (ex: 25.0, 30.0, 60.0)
        std::vector<Channel> channels; // 1 channel por bone animado

        FrameAnimation()
            : name("unnamed"), duration(0.0f), ticksPerSecond(25.0f)
        {
        }
    };

    FrameAnimation *Load(const std::string &filename);

private:
    Stream *m_stream;
    bool ReadInfoChunk(FrameAnimation &info);
    bool ReadChannelChunk(Channel &channel);
};


enum class PlayMode
{
    Once,
    Loop,
    PingPong
};

class AnimationLayer
{
public:
    AnimationLayer( );
    ~AnimationLayer();

    void AddAnimation(const std::string &name, Animation *anim);
    Animation *GetAnimation(const std::string &name);
    Animation *LoadAnimation(const std::string &name, const std::string &filename);

 
    void Play(const std::string &animName, PlayMode mode = PlayMode::Loop,
              float blendTime = 0.3f);
    void PlayOneShot(const std::string &animName, const std::string &returnTo, float blendTime = 0.3f, PlayMode toMode = PlayMode::Loop);
    void CrossFade(const std::string &toAnim, float duration);
    void Stop(float blendOutTime = 0.3f);
    void Pause();
    void Resume();

    // Update
    void Update(float deltaTime);
    void Update(float deltaTime,const std::vector<Joint3D*> &bones);

    void Bind(Node3D *parent);
 

    // Getters
    bool IsPlaying(const std::string &animName) const;
    float GetCurrentTime() const { return m_currentTime; }
    const std::string &GetCurrentAnimation() const { return m_currentAnimName; }

    // Settings
    void SetSpeed(float speed) { m_globalSpeed = speed; }
    void SetDefaultBlendTime(float time) { m_defaultBlendTime = time; }

private:
 
    std::map<std::string, Animation *> m_animations;

    // Animação única atual
    std::string m_currentAnimName;
    std::string m_previousAnimName;
    Animation *m_currentAnim;
    Animation *m_previousAnim;
    Animation *m_playTo;

    float m_currentTime;
    float m_currentTimeBlend;
    float m_globalSpeed;
    bool m_isPaused;

    // Blending
    bool m_isBlending;
    float m_blendTime;
    float m_blendDuration;

    // OneShot
    std::string m_returnToAnim;
    bool m_shouldReturn;

    PlayMode m_currentMode;
    PlayMode m_toReturnMode;
    float m_defaultBlendTime;
    bool m_isPingPongReverse;

    void UpdateBlending(float deltaTime);
    void UpdateLayers(float deltaTime);
    bool CheckAnimationEnd();
};

class Animator : public Component
{

    std::vector<AnimationLayer *> layers;
    Mesh *m_mesh;
    bool m_active;
    bool m_firstStarted;
    MeshRenderer *meshRenderer;

public:
    Animator();
    ~Animator();

    void update(float deltaTime) override;
    const char *getTypeName() const override { return "Animator"; }

    void setActive(bool active) { m_active = active; }
    bool isActive() const { return m_active; }

    void attach() override;

    void render() override;


    AnimationLayer *AddLayer();
    AnimationLayer *GetLayer(u32 index);


};
