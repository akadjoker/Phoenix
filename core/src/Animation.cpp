#include "pch.h"
#include "Driver.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "Stream.hpp"


Animator::Animator(Mesh *mesh)
{
    this->m_mesh = mesh;
}

Animator::~Animator()
{
    for (AnimationLayer *layer : layers)
        delete layer;

    layers.clear();
    m_mesh = nullptr;
}

void Animator::Update(float deltaTime)
{
    for (size_t i = 0; i < layers.size(); i++)
    {
        layers[i]->Update(deltaTime);
    }
}

AnimationLayer *Animator::AddLayer()
{
    
    AnimationLayer *layer =  new AnimationLayer(m_mesh);
    layers.push_back(layer);
    return layer;
}

AnimationLayer *Animator::GetLayer(u32 index)
{
    if (index >= layers.size())
    {
        LogWarning("[Animator] Invalid layer index: %d", index);
        return nullptr;
    }
    return layers[index];
}

AnimationLayer::AnimationLayer(Mesh *mesh)
    : m_mesh(mesh), m_currentAnim(nullptr), m_previousAnim(nullptr), m_playTo(nullptr), m_currentTime(0.0f),m_currentTimeBlend(0.0f), m_globalSpeed(1.0f), m_isPaused(false), m_isBlending(false), m_blendTime(0.0f), m_blendDuration(0.9f), m_shouldReturn(false), m_currentMode(PlayMode::Loop), m_defaultBlendTime(0.3f), m_isPingPongReverse(false)
{
}

AnimationLayer::~AnimationLayer()
{
    // Deleta as animações que foram criadas pelo AnimationLayer
    for (auto &pair : m_animations)
    {
        delete pair.second;
    }
    m_animations.clear();
}

// ============================================================================
// GERENCIAMENTO DE ANIMAÇÕES
// ============================================================================

void AnimationLayer::AddAnimation(const std::string &name, Animation *anim)
{
    m_animations[name] = anim;
    if (anim)
        anim->BindToMesh(m_mesh);
}

Animation *AnimationLayer::GetAnimation(const std::string &name)
{
    auto it = m_animations.find(name);
    if (it != m_animations.end())
        return it->second;
    return nullptr;
}

Animation *AnimationLayer::LoadAnimation(const std::string &name, const std::string &filename)
{
    if (GetAnimation(name))
        return nullptr;

    Animation *anim = new Animation();
    if (!anim->Load(filename))
    {
        delete anim;
        return nullptr;
    }
    anim->m_name = name;

    AddAnimation(name, anim);
    return anim;
}

// ============================================================================
// CONTROLE DE PLAYBACK (ANIMAÇÃO ÚNICA)
// ============================================================================

void AnimationLayer::Play(const std::string &animName, PlayMode mode, float blendTime)
{
    Animation *anim = GetAnimation(animName);
    if (!anim)
        return;

     if (m_currentAnimName == animName && !m_isBlending)
        return;

    if (m_currentAnim && blendTime > 0.0f)
    {
        m_isBlending = true;
        m_blendTime = 0.0f;
        m_blendDuration = blendTime;
    }
    else
    {
        m_isBlending = false;
        m_blendTime = 0.0f;
        m_blendDuration = 0.0f;
    }

    if (!m_playTo)
    {
        m_previousAnim = anim;
        m_previousAnimName = animName;
    }

    m_playTo = anim;
    m_currentMode = mode;
    m_shouldReturn = false;
    m_isPingPongReverse = false;
}

void AnimationLayer::PlayOneShot(const std::string &animName, const std::string &returnTo, float blendTime, PlayMode toMode)
{
    m_returnToAnim = returnTo;
    m_toReturnMode = toMode;
    Play(animName, PlayMode::Once, blendTime);
    m_shouldReturn = true;
}

void AnimationLayer::CrossFade(const std::string &toAnim, float duration)
{
    Play(toAnim, m_currentMode, duration);
}

void AnimationLayer::Stop(float blendOutTime)
{
    if (blendOutTime > 0.0f && m_currentAnim)
    {
        // Blend out para bind pose
        m_previousAnim = m_currentAnim;
        m_previousAnimName = m_currentAnimName;
        m_currentAnim = nullptr;
        m_currentAnimName = "";

        m_isBlending = true;
        m_blendTime = 0.0f;
        m_blendDuration = blendOutTime;
    }
    else
    {
        // Stop imediato
        if (!m_isPaused)
        {
            m_currentAnim = nullptr;
            m_currentAnimName = "";
            m_currentTime = 0.0f;
            m_isPaused = false;
            m_isBlending = false;

            // Volta para bind pose?? em multi layer temos problemas , logo s eve se vale apena
            // if (m_mesh)
            // {
            //     for (Bone *bone : m_mesh->GetBones())
            //     {
            //         bone->hasAnimation = false;
            //     }
            //     m_mesh->UpdateSkinning();
            // }
        }
    }
}

void AnimationLayer::Pause()
{
    m_isPaused = true;
}

void AnimationLayer::Resume()
{
    m_isPaused = false;
}

bool AnimationLayer::IsPlaying(const std::string &animName) const
{
    if (m_currentAnimName == animName && m_currentAnim && !m_isPaused)
        return true;

    return false;
}

// ============================================================================
// UPDATE PRINCIPAL
// ============================================================================

void AnimationLayer::Update(float deltaTime)
{
    if (m_isPaused)
        return;

    float dt = deltaTime * m_globalSpeed;
    bool isOnBlend = false;

    if (m_playTo)
    {

       

            if (m_currentAnim)
            {
                m_previousAnim = m_currentAnim;
                m_previousAnimName = m_currentAnim->GetName();
            }

            if (m_isBlending)
            {
                // LogInfo("[ANIMATOR] Blending from %s to %s", m_currentAnimName.c_str(), m_playTo->GetName().c_str());
                m_blendTime += dt ;
                float blend = Min(m_blendTime / m_blendDuration, 1.0f);

                if (blend >= 1.0f || !m_currentAnim)
                {  
                    m_currentTime = m_currentTimeBlend;
                    m_currentTimeBlend = 0.0f;
                    m_isBlending = false;
                    m_blendTime = 0.0f;
                    m_blendDuration = 0.0f;
                    isOnBlend = false;
                    m_currentAnimName = m_playTo->GetName(); 
                    m_currentAnim = m_playTo;
                    m_playTo = nullptr;
                }
                else
                {
                    
                  //  LogInfo("[ANIMATOR]  Blending %f - %f - Blend %f ", m_blendTime, m_blendDuration,blend);

                       m_currentAnimName= m_playTo->GetName(); 
                       m_currentTimeBlend+= dt  * m_playTo->GetTicksPerSecond();

                       while (m_currentTimeBlend >= m_playTo->GetDuration())
                            m_currentTimeBlend -= m_playTo->GetDuration();


                  
                        isOnBlend = true;

                        for (const auto &channel : m_currentAnim->m_channels)
                        {
                            if (channel.boneIndex == (u32)-1)
                                continue;

                            
                            Vec3 pos1, pos2;
                            Quat rot1, rot2;

                            pos1 = m_currentAnim->InterpolatePosition(channel, m_currentTime);
                            rot1 = m_currentAnim->InterpolateRotation(channel, m_currentTime);

                            AnimationChannel* newCh = m_playTo->FindChannel(channel.boneName);
                            if (newCh)
                            {
                                pos2 = m_playTo->InterpolatePosition(*newCh,m_currentTimeBlend);
                                rot2 = m_playTo->InterpolateRotation(*newCh,m_currentTimeBlend);

                                Vec3 finalPos = Vec3::Lerp(pos1 , pos2, blend);
                                Quat finalRot = Quat::Slerp(rot1, rot2, blend);
                    

                            m_mesh->SetBoneTransform(channel.boneIndex, finalPos, finalRot);
                            }
                            else
                            {
                                //LogWarning("Channel not found: %s", channel.boneName.c_str());
                                m_mesh->SetBoneTransform(channel.boneIndex, pos1, rot1);
                            }



                       
                        }
                        m_mesh->UpdateSkinning();
                }


            }
            else
            {
                //   LogInfo("[ANIMATOR] no Blending from %s to %s", m_currentAnimName.c_str(), m_playTo->GetName().c_str());

                m_currentTime = 0.0f;
                m_isBlending = false;
                m_blendTime = 0.0f;
                m_blendDuration = 0.0f;
                m_currentAnimName = m_playTo->GetName();
                m_currentAnim = m_playTo;
                m_playTo = nullptr;
            }
        
 
    }



    if (isOnBlend)        return;
 
    if (m_currentAnim)
    {
       float duration = m_currentAnim->GetDuration();

        if (m_currentMode == PlayMode::PingPong)
        {
            if (!m_isPingPongReverse)
            {
                m_currentTime += dt * m_currentAnim->GetTicksPerSecond();
                if (m_currentTime >= duration)
                {
                    m_currentTime = duration;
                    m_isPingPongReverse = true;
                }
            }
            else
            {
                m_currentTime -= dt * m_currentAnim->GetTicksPerSecond();
                if (m_currentTime <= 0.0f)
                {
                    m_currentTime = 0.0f;
                    m_isPingPongReverse = false;
                }
            }
        }
        else
        {
            m_currentTime += dt * m_currentAnim->GetTicksPerSecond();

            if (CheckAnimationEnd())
            {
                if (m_shouldReturn && !m_returnToAnim.empty())
                {
                   // LogInfo("Returning to previous animation: %s from %s", m_returnToAnim.c_str(), m_currentAnimName.c_str());
                    m_currentAnimName = "";
                    m_isBlending = false;
                    m_currentTime = 0.0f;
                    m_isPaused = false;
                    Play(m_returnToAnim, m_toReturnMode, m_defaultBlendTime);
                    m_shouldReturn = false;
                    return;
                }
            }
        }

        // Sample da animação
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



// ============================================================================
// CHECK ANIMATION END
// ============================================================================

bool AnimationLayer::CheckAnimationEnd()
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

 

    case PlayMode::PingPong:
        if (m_currentTime >= duration)
        {
            m_currentTime = duration;
        }
        else if (m_currentTime < 0.0f)
        {
            m_currentTime = 0.0f;
        }
        break;
    }

    return false;
}