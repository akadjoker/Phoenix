#include "pch.h"
#include "Driver.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "Stream.hpp"



Animator::Animator(Mesh *mesh)
    : m_mesh(mesh), m_currentAnim(nullptr), m_previousAnim(nullptr), m_currentTime(0.0f), m_globalSpeed(1.0f), m_isPaused(false), m_isBlending(false), m_blendTime(0.0f), m_blendDuration(0.3f), m_shouldReturn(false), m_currentMode(PlayMode::Loop), m_defaultBlendTime(0.3f)
{
}

Animator::~Animator()
{
    for (auto it = m_animations.begin(); it != m_animations.end(); ++it)
    {
           delete it->second;
    }
}

void Animator::AddAnimation(const std::string &name, Animation *anim)
{
    m_animations[name] = anim;
    if (anim)
        anim->BindToMesh(m_mesh);
}

Animation *Animator::GetAnimation(const std::string &name)
{
    auto it = m_animations.find(name);
    if (it != m_animations.end())
        return it->second;
    return nullptr;
}

Animation *Animator::LoadAnimation(const std::string &name, const std::string &filename)
{
    if (GetAnimation(name))
        return nullptr;

    Animation *anim = new Animation();
    if (!anim->Load(filename))
    {
        delete anim;
        return nullptr;
    }
    AddAnimation(name, anim);
    return anim;
    
}

void Animator::Play(const std::string &animName, PlayMode mode, float blendTime)
{
    Animation *anim = GetAnimation(animName);
    if (!anim)
        return;

    if (m_currentAnimName == animName && !m_isBlending)
        return;

    m_previousAnim = m_currentAnim;
    m_previousAnimName = m_currentAnimName;
    m_currentAnim = anim;
    m_currentAnimName = animName;
    m_currentMode = mode;
    m_shouldReturn = false;

    if (blendTime > 0.0f && m_previousAnim)
    {
        m_isBlending = true;
        m_blendTime = 0.0f;
        m_blendDuration = blendTime;
    }
    else
    {
        m_isBlending = false;
        m_currentTime = 0.0f;
    }
}

void Animator::PlayOneShot(const std::string &animName, const std::string &returnTo, float blendTime)
{
    m_returnToAnim = returnTo;
    m_shouldReturn = true;
    Play(animName, PlayMode::Once, blendTime);
}

void Animator::CrossFade(const std::string &toAnim, float duration)
{
    Play(toAnim, m_currentMode, duration);
}

void Animator::Update(float deltaTime)
{
    if (m_isPaused)
        return;

    float dt = deltaTime * m_globalSpeed;


 
    bool useLayers = false;
    for (AnimationLayer &layer : m_layers)
    {
        if (!layer.isActive || !layer.animation)
            continue;

        useLayers = true;

        float duration = layer.animation->GetDuration();
        layer.currentTime += dt;

        switch (m_currentMode)
        {
        case PlayMode::Once:
            if (layer.currentTime >= duration)
            {
                layer.currentTime = duration;
                m_isPaused = true;
                layer.isActive = false;
            }
            break;

        case PlayMode::Loop:
            if (layer.currentTime >= duration)
            {
                layer.currentTime = fmod(layer.currentTime, duration);
                layer.isActive = true;
            }
            break;

        case PlayMode::OnceAndReturn:
            if (layer.currentTime >= duration)
            {
                layer.isActive = false;
            }
            break;

        case PlayMode::PingPong:
            // TODO: Implementar ping pong
            break;
        }

        for (const auto &channel : layer.animation->m_channels)
        {
            if (channel.boneIndex == (u32)-1)
                continue;

            Vec3 pos = layer.animation->InterpolatePosition(channel, layer.currentTime);
            Quat rot = layer.animation->InterpolateRotation(channel, layer.currentTime);
            m_mesh->SetBoneTransform(channel.boneIndex, pos, rot);
        }
        m_mesh->UpdateSkinning();
         
    }

    if (!useLayers)
    {
        if (m_currentAnim)
        {
            if (m_isBlending)
            {
                UpdateBlending(dt);
            }
            else
            {
                m_currentTime += dt;

                if (CheckAnimationEnd())
                {
                    if (m_shouldReturn && !m_returnToAnim.empty())
                    {
                        Play(m_returnToAnim, PlayMode::Loop, m_defaultBlendTime);
                        m_shouldReturn = false;
                        return; // Vai entrar em blending no próximo frame
                    }
                }

                for (const auto &channel : m_currentAnim->m_channels)
                {
                    if (channel.boneIndex == (u32)-1)
                        continue;

                    Vec3 pos = m_currentAnim->InterpolatePosition(channel, m_currentTime);
                    Quat rot = m_currentAnim->InterpolateRotation(channel, m_currentTime);
                    m_mesh->SetBoneTransform(channel.boneIndex, pos, rot);
                }
                m_mesh->UpdateSkinning();
            }
        }
    }
}

void Animator::UpdateBlending(float deltaTime)
{
    m_blendTime += deltaTime;
    float blend = std::min(m_blendTime / m_blendDuration, 1.0f);

    if (blend >= 1.0f)
    {
        // Blend completo
        m_isBlending = false;
        m_currentTime = 0.0f;
        m_previousAnim = nullptr;
        return;
    }

    // Blend em progresso
    m_currentTime += deltaTime;

    // Blend entre previous e current
    for (Bone *bone : m_mesh->GetBones())
    {
        AnimationChannel *ch1 = m_previousAnim->FindChannel(bone->name);
        AnimationChannel *ch2 = m_currentAnim->FindChannel(bone->name);

        Vec3 pos1(0, 0, 0), pos2(0, 0, 0);
        Quat rot1(0, 0, 0, 1), rot2(0, 0, 0, 1);

        // Sample from previous
        if (ch1)
        {
            pos1 = m_previousAnim->InterpolatePosition(*ch1, m_currentTime);
            rot1 = m_previousAnim->InterpolateRotation(*ch1, m_currentTime);
        }
        else
        {
            continue; // Bone não tem animação anterior
        }

        // Sample from current
        if (ch2)
        {
            pos2 = m_currentAnim->InterpolatePosition(*ch2, m_currentTime);
            rot2 = m_currentAnim->InterpolateRotation(*ch2, m_currentTime);
        }
        else
        {
            // Current não tem, usar previous
            pos2 = pos1;
            rot2 = rot1;
        }

        // Blend (interpolação suave)
        Vec3 finalPos = Vec3::Lerp(pos1, pos2, blend);
        Quat finalRot = Quat::Slerp(rot1, rot2, blend);

        bone->transform = (Mat4::Translation(finalPos) * finalRot.toMat4());
    }
}
void Animator::BlendAnimations(Animation *from, Animation *to, float blend)
{
    if (!from || !to || !m_mesh)
        return;

    std::vector<Bone *> &bones = m_mesh->GetBones();
    for (Bone *bone : bones)
    {
        bone->hasAnimation = false;
        bone->transform = bone->localPose;
    }

    // Para cada bone
    for (Bone *bone : bones)
    {
        AnimationChannel *ch1 = from->FindChannel(bone->name);
        AnimationChannel *ch2 = to->FindChannel(bone->name);

        if (!ch1 && !ch2)
            continue;

        Vec3 pos1(0, 0, 0), pos2(0, 0, 0);
        Quat rot1(0, 0, 0, 1), rot2(0, 0, 0, 1);
        Vec3 scale1(1, 1, 1), scale2(1, 1, 1);

        // Sample from
        if (ch1)
        {
            pos1 = from->InterpolatePosition(*ch1, m_currentTime);
            rot1 = from->InterpolateRotation(*ch1, m_currentTime);
        }
        else
        {
            // Usar localPose
            continue;
        }

        // Sample to
        if (ch2)
        {
            pos2 = to->InterpolatePosition(*ch2, m_currentTime);
            rot2 = to->InterpolateRotation(*ch2, m_currentTime);
        }
        else
        {
            pos2 = pos1;
            rot2 = rot1;
        }

        // Blend
        Vec3 finalPos = Vec3::Lerp(pos1, pos2, blend);
        Quat finalRot = Quat::Slerp(rot1, rot2, blend);
        Mat4 local = Mat4::Translation(finalPos) * finalRot.toMat4();

        bone->hasAnimation = true;
        bone->transform = local;
        // m_mesh->SetBoneTransform(channelboneIndex, finalPos, finalRot);
    }

    m_mesh->UpdateSkinning();
}

bool Animator::CheckAnimationEnd()
{
    if (!m_currentAnim)
        return false;

    float duration = m_currentAnim->GetDuration();

    switch (m_currentMode)
    {
    case PlayMode::Once:
        if (m_currentTime >= duration)
        {
            m_currentTime = duration;
            m_isPaused = true;
            return true;
        }
        break;

    case PlayMode::Loop:
        if (m_currentTime >= duration)
        {
            m_currentTime = fmod(m_currentTime, duration);
        }
        break;

    case PlayMode::OnceAndReturn:
        if (m_currentTime >= duration)
        {
            return true;
        }
        break;

    case PlayMode::PingPong:
        // TODO: Implementar ping pong
        break;
    }

    return false;
}

// LAYERS

u32 Animator::AddLayer(const std::string &layerName, float weight)
{
    AnimationLayer layer;
    layer.name = layerName;
    layer.animation = nullptr;
    layer.weight = weight;
    layer.speed = 1.0f;
    layer.currentTime = 0.0f;
    layer.mode = PlayMode::Loop;
    layer.isActive = false;

    m_layers.push_back(layer);
    return m_layers.size() - 1;
}

void Animator::PlayOnLayer(u32 layerIndex, const std::string &animName, PlayMode mode)
{
    if (layerIndex >= m_layers.size())
    {
        LogWarning("Layer index out of range: %d", layerIndex);
        return;
    }

    if (animName.empty())
        return;

    Animation *anim = GetAnimation(animName);
    if (!anim)
    {
        LogWarning("Animation not found: %s", animName.c_str());
        return;
    }

    AnimationLayer &layer = m_layers[layerIndex];
    layer.animation = anim;
    layer.mode = mode;
    layer.currentTime = 0.0f;
    layer.isActive = true;
}
