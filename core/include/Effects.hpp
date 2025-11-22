#pragma once
#include "Config.hpp"
#include "GraphicsTypes.hpp"
#include "Math.hpp"
#include "Texture.hpp"
#include <string>
#include <vector>



class RenderBatch;
class Node3D;
 

class LensFlare
{
private:
    Texture *texture;
    
    Vec3 lightPosWorld;
    Vec3 cameraPosition;
    Vec3 cameraForward;
    
    float viewAngle;
    float borderLimit;
    bool occluded;
    
    FloatRect burnClip;
    std::vector<FloatRect> clips;
    std::vector<float> offsets;
    std::vector<float> scales;
    std::vector<int> indexes;
    std::vector<Vec3> colors;
    
    int flareCount;
    int textureWidth;
    int textureHeight;
    
 
    float angle_between_camera_and_point(const Vec3 &point,
                                         const Vec3 &cameraPos,
                                         const Vec3 &cameraForwardDir);
    
    bool IsLightVisible();
    
    float CalculateBurnByAngle(float angleDeg, float minAngle, float maxAngle);
    
    Vec3 CalculateScreenPosition(const Mat4 &view, const Mat4 &projection,
                                 int screenWidth, int screenHeight);
    
    float CalculateFade(const Vec2 &lightPos, float screenWidth,
                        float screenHeight, float borderLimit);
    
    void RenderFace(RenderBatch *batch, float x, float y,
                    float size, const FloatRect &clip);

public:
    LensFlare(Texture *tex);
    
    void Update(Scene *scene, const Vec3 &lightPos,
                const Vec3 &camPos, const Vec3 &camForward);
    
    void Render(RenderBatch *batch, const Mat4 &view, const Mat4 &projection,
                int screenWidth, int screenHeight);
};

 

struct EffectVertex
{
    Vec3 position;   
    Vec2 texCoord;
    Vec4 color;
};

class TrailRenderer
{
private:
    struct TrailPoint
    {
        Vec3 position;
        float width;
        float time;
        Vec3 color;
        float alpha;
    };

    std::vector<TrailPoint> points;
    std::vector<EffectVertex> vertices;
    
    VertexArray *buffer;
    VertexBuffer *vb;
    Texture *texture;
    
    int maxPoints;
    float lifetime;
    float minDistance;
    float startWidth;
    float endWidth;
    Vec3 startColor;
    Vec3 endColor;

public:
    TrailRenderer(Texture *tex, int maxPts = 50, float life = 1.0f);
    ~TrailRenderer();

    void SetWidth(float start, float end);
    void SetColor(const Vec3 &start, const Vec3 &end);
    void SetLifetime(float life);
    void SetMinDistance(float dist);
    
    void AddPoint(const Vec3 &position, float currentTime);
    void Update(float currentTime);
    void Render(Camera *camera );
    void Clear();
    
    int GetPointCount() const { return points.size(); }
};

 

// class RibbonTrail
// {
// public:
//     struct TrailElement
//     {
//         Vec3 position;
//         Quat orientation;
//         Vec3 color;
//         float width;
//         float alpha;
//         float timeCreated;
//     };
//     const int MAX_CHAINS = 4;

//     struct Chain
//     {
//         std::vector<TrailElement> elements;
//         Node3D* node;
//         Vec3 offset;
//         bool headVisible;
//         bool tailVisible;
        
//         void RemoveOldest();
//     };

// private:
//     std::vector<Chain> chains;
//     VertexArray* vao;
//     VertexBuffer* vb;
//     IndexBuffer* ib;
//     Texture* texture;
    
//     int maxChainElements;
//     float trailLength;
//     float minDistance;
    
//     float initialWidth[4];
//     float widthChange[4];
//     Vec3 initialColor[4];
//     Vec3 colorChange[4];
    
//     bool faceCamera;
//     bool dynamic;
    
//     std::vector<EffectVertex> vertices;
//     std::vector<u32> indices;

//     void UpdateChain(Chain& chain, int chainIndex, float time);
//     void BuildGeometry(Camera* camera);

// public:
//     RibbonTrail(int maxElements = 100, int numChains = 1);
//     ~RibbonTrail();

//     // Setters
//     void SetMaxChainElements(int maxElements);
//     void SetTrailLength(float length);
//     void SetMinDistance(float dist);
//     void SetNumberOfChains(int num);
    
//     void SetInitialColor(int chainIndex, const Vec3& color);
//     void SetColorChange(int chainIndex, const Vec3& change);
//     void SetInitialWidth(int chainIndex, float width);
//     void SetWidthChange(int chainIndex, float change);

 


//     void SetFaceCamera(bool face);
//     void SetTexture(Texture* tex);

//     // Node management
//     void AddNode(Node3D* node, int chainIndex = 0);
//     void RemoveNode(Node3D* node);
//     void SetChainOffset(int chainIndex, const Vec3& offset);

//     // Chain management
//     void ClearChain(int chainIndex);
//     void ClearAllChains();

//     // Update and render
//     void Update(float time, Camera* camera);
//     void Render( );

//     // Getters
//     int GetChainCount() const;
//     int GetMaxChainElements() const;
//     float GetTrailLength() const;
// };
