#pragma once
#include "Core.hpp"

#pragma once
#include "Core.hpp"

class GodRays
{
private:
    GLuint sceneFBO;
    GLuint sceneTexture;
    GLuint sceneDepth;
    
    GLuint occlusionFBO;
    GLuint occlusionTexture;
    GLuint occlusionDepth;   
    
    GLuint godRaysFBO;
    GLuint godRaysTexture;
    
    Shader* occlusionShader;
    Shader* godRaysShader;
    Shader* compositeShader;
    

    
    int screenWidth;
    int screenHeight;
    
public:
    float exposure = 0.3f;
    float decay = 0.95f;
    float density = 0.8f;
    float weight = 0.6f;
    int numSamples = 100;
    
    bool Init(int width, int height);
    void Resize(int width, int height);
    void BeginScenePass();
    void RenderOcclusion(Scene* scene, const Vec3& lightWorldPos, const Vec3& cameraPos,
                         const Mat4& view, const Mat4& proj);
    void ApplyGodRays(const Vec3& lightWorldPos, const Mat4& view, const Mat4& proj);
    void Composite();
    void Release();
    
private:
    void CreateFramebuffers();
    void DeleteFramebuffers();
};
