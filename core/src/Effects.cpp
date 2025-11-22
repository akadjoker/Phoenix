#include "pch.h"
#include "Effects.hpp"
#include "Batch.hpp"
#include "Texture.hpp"
#include "Node3D.hpp"

LensFlare::LensFlare(Texture *tex)
    : texture(tex)
    , lightPosWorld(-2.0f, 8.0f, -4.0f)
    , viewAngle(0.0f)
    , borderLimit(0.0f)
    , occluded(false)
    , flareCount(8)
{
    if (!texture || !texture->IsValid())
    {
        LogWarning("[LensFlare] Texture not valid");
        return;
    }
    
    textureWidth = tex->GetWidth();
    textureHeight = tex->GetHeight();
    
    burnClip = FloatRect(185, 423, 4, 4);
    
    // Initialize clips
    clips.push_back(FloatRect(128, 236, 128, 128));
    clips.push_back(FloatRect(256, 411, 64, 64));
    clips.push_back(FloatRect(256, 347, 64, 64));
    clips.push_back(FloatRect(256, 283, 64, 64));
    clips.push_back(FloatRect(256, 219, 64, 64));
    clips.push_back(FloatRect(238, 155, 64, 64));
    clips.push_back(FloatRect(238, 155, 64, 64));
    clips.push_back(FloatRect(284, 475, 28, 28));
    clips.push_back(FloatRect(302, 91, 27, 26));
    
    // Initialize offsets and scales
    offsets = {-0.8f, -0.6f, -0.4f, -0.2f, 0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f};
    scales = {0.2f, 0.15f, 0.25f, 0.5f, 0.75f, 0.5f, 0.3f, 0.4f, 0.2f};
    indexes = {0, 4, 1, 2, 2, 3, 1, 5, 3, 2, 4, 1, 2, 3};
    
    // Initialize colors
    colors.push_back(Vec3(1.0f, 1.0f, 1.0f));
    colors.push_back(Vec3(1.0f, 0.6f, 0.6f));
    colors.push_back(Vec3(1.0f, 0.7f, 1.0f));
    colors.push_back(Vec3(1.0f, 1.0f, 1.0f));
    colors.push_back(Vec3(1.0f, 0.8f, 1.0f));
    colors.push_back(Vec3(0.7f, 1.0f, 0.6f));
    colors.push_back(Vec3(1.0f, 0.9f, 0.5f));
    colors.push_back(Vec3(1.0f, 1.0f, 1.0f));
    colors.push_back(Vec3(1.0f, 0.6f, 0.6f));
}

float LensFlare::angle_between_camera_and_point(const Vec3 &point,
                                                 const Vec3 &cameraPos,
                                                 const Vec3 &cameraForwardDir)
{
    Vec3 forward = cameraForwardDir.normalized();
    Vec3 toPoint = (point - cameraPos).normalized();
    
    float cos_angle = Clamp(forward.dot(toPoint), -1.0f, 1.0f);
    float angle_rad = std::acos(cos_angle);
    return angle_rad * RAD2DEG;
}

bool LensFlare::IsLightVisible()
{
    float fov = 90.0f;
    viewAngle = angle_between_camera_and_point(lightPosWorld, cameraPosition, cameraForward);
    return viewAngle <= fov * 0.5f;
}

float LensFlare::CalculateBurnByAngle(float angleDeg, float minAngle, float maxAngle)
{
    if (angleDeg > maxAngle)
        return 0.0f;
    if (angleDeg < minAngle)
        return 0.6f;
    
    float t = (maxAngle - angleDeg) / (maxAngle - minAngle);
    return Lerp(0.0f, 0.6f, t);
}

Vec3 LensFlare::CalculateScreenPosition(const Mat4 &view, const Mat4 &projection,
                                        int screenWidth, int screenHeight)
{
    Vec4 clipSpace = projection * view * Vec4(lightPosWorld.x, lightPosWorld.y,
                                               lightPosWorld.z, 1.0f);
    
    if (clipSpace.w == 0.0f)
        return Vec3(0, 0, -1);
    
    Vec3 ndc = Vec3(clipSpace.x / clipSpace.w,
                    clipSpace.y / clipSpace.w,
                    clipSpace.z / clipSpace.w);
    
    float x = (ndc.x + 1.0f) * 0.5f * screenWidth;
    float y = (1.0f - ndc.y) * 0.5f * screenHeight;
    
    return Vec3(x, y, ndc.z);
}

float LensFlare::CalculateFade(const Vec2 &lightPos, float screenWidth,
                                float screenHeight, float borderLimit)
{
    float awayX = 0.0f;
    if (lightPos.x < borderLimit)
        awayX = borderLimit - lightPos.x;
    else if (lightPos.x > screenWidth - borderLimit)
        awayX = lightPos.x - (screenWidth - borderLimit);
    
    float awayY = 0.0f;
    if (lightPos.y < borderLimit)
        awayY = borderLimit - lightPos.y;
    else if (lightPos.y > screenHeight - borderLimit)
        awayY = lightPos.y - (screenHeight - borderLimit);
    
    float away = Max(awayX, awayY);
    if (away > borderLimit)
        away = borderLimit;
    
    float intensity = 1.0f - (away / borderLimit);
    return Clamp(intensity, 0.0f, 1.0f);
}

void LensFlare::RenderFace(RenderBatch *batch, float x, float y,
                           float size, const FloatRect &clip)
{
    batch->QuadCentered(texture, x, y, size, clip);
}

void LensFlare::Update(Scene *scene, const Vec3 &lightPos,
                       const Vec3 &camPos, const Vec3 &camForward)
{
    lightPosWorld = lightPos;
    cameraPosition = camPos;
    cameraForward = camForward;
    
    Vec3 rayDir = (camPos - lightPos);
    rayDir.normalize();
    
    Ray ray(lightPos, rayDir);
    // occluded = scene->RayIntersects(ray);
}

void LensFlare::Render(RenderBatch *batch, const Mat4 &view, const Mat4 &projection,
                       int screenWidth, int screenHeight)
{
    if (occluded)
        return;
    
    Vec3 lightPosScreen3D = CalculateScreenPosition(view, projection,
                                                     screenWidth, screenHeight);
    
    if (lightPosScreen3D.z < 0.0f)
        return;
    
    Vec2 lightPosScreen(lightPosScreen3D.x, lightPosScreen3D.y);
    
    if (lightPosScreen.x < 0.0f || lightPosScreen.x > screenWidth ||
        lightPosScreen.y < 0.0f || lightPosScreen.y > screenHeight)
        return;
    
    Vec2 screenCenter(screenWidth / 2.0f, screenHeight / 2.0f);
    Vec2 flareDirection = screenCenter - lightPosScreen;
    
    borderLimit = screenWidth * 0.2f;
    
    float fadeIntensity = CalculateFade(lightPosScreen, screenWidth,
                                        screenHeight, borderLimit);
    
    if (IsLightVisible())
    {
        Driver &driver = Driver::Instance();
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::One);
        
        float burnIntensity = CalculateBurnByAngle(viewAngle, 0.0f, 20.0f);
        
        if (burnIntensity > 0.0f)
        {
            float quadOpacity = Lerp(0.0f, 1.0f, burnIntensity);
            Vec3 quadColor = Vec3(0.8f, 0.8f, 0.8f);
            Vec4 color(quadColor.x, quadColor.y, quadColor.z, quadOpacity);
            
            batch->SetColor(color.x, color.y, color.z);
            batch->SetAlpha(quadOpacity);
            RenderFace(batch, screenCenter.x, screenCenter.y, 8.0f, burnClip);
        }
        
        Vec3 col = colors[0];
        batch->SetAlpha(fadeIntensity);
        batch->SetColor(col.x, col.y, col.z);
        RenderFace(batch, lightPosScreen.x, lightPosScreen.y, scales[0], clips[0]);
        
        for (int i = 1; i < flareCount; i++)
        {
            float x = lightPosScreen.x - (flareDirection.x * offsets[i]) * 2.0f;
            float y = lightPosScreen.y - (flareDirection.y * offsets[i]) * 2.0f;
            
            Vec3 elementColor = colors[i] * fadeIntensity;
            int index = indexes[i];
            
            batch->SetAlpha(fadeIntensity);
            batch->SetColor(elementColor.x, elementColor.y, elementColor.z);
            RenderFace(batch, x, y, scales[i], clips[index]);
        }
        
        batch->Render();
    }
}

 

TrailRenderer::TrailRenderer(Texture *tex, int maxPts, float life)
    : texture(tex)
    , maxPoints(maxPts)
    , lifetime(life)
    , minDistance(0.1f)
    , startWidth(1.0f)
    , endWidth(0.1f)
    , startColor(1, 1, 1)
    , endColor(1, 1, 1)
{
    points.reserve(maxPoints);
    vertices.reserve(maxPoints * 2); // 2 vértices por ponto para strip
    
    buffer = new VertexArray();
    vb = buffer->AddVertexBuffer(sizeof(EffectVertex), 
                                  maxPoints * 2 * sizeof(EffectVertex), true);
    
    auto *decl = buffer->GetVertexDeclaration();
    decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
    decl->AddElement(0, 3 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    decl->AddElement(0, 5 * sizeof(float), VET_FLOAT4, VES_COLOR);
}

TrailRenderer::~TrailRenderer()
{
    delete buffer;
}

void TrailRenderer::SetWidth(float start, float end)
{
    startWidth = start;
    endWidth = end;
}

void TrailRenderer::SetColor(const Vec3 &start, const Vec3 &end)
{
    startColor = start;
    endColor = end;
}

void TrailRenderer::SetLifetime(float life)
{
    lifetime = life;
}

void TrailRenderer::SetMinDistance(float dist)
{
    minDistance = dist;
}

void TrailRenderer::AddPoint(const Vec3 &position, float currentTime)
{
    // Check minimum distance to avoid too many points
    if (!points.empty())
    {
        Vec3 diff = position - points.back().position;
        if (diff.length() < minDistance)
            return;
    }

    TrailPoint pt;
    pt.position = position;
    pt.time = currentTime;
    pt.width = startWidth;
    pt.color = startColor;
    pt.alpha = 1.0f;

    points.push_back(pt);

    // Remove oldest point if exceeding max
    if (points.size() > maxPoints)
        points.erase(points.begin());
}

void TrailRenderer::Update(float currentTime)
{
    // Remove expired points and update properties
    for (int i = points.size() - 1; i >= 0; i--)
    //    for (size_t i = 0; i < points.size(); i++)
    
    {
        float age = currentTime - points[i].time;
        if (age > lifetime)
        {
            points.erase(points.begin() + i);
            continue;
        }

        // Interpolate properties based on age
        float t = age / lifetime;
        points[i].width = Lerp(startWidth, endWidth, t);
        points[i].color = startColor * (1.0f - t) + endColor * t;
        points[i].alpha = 1.0f - t;
    }
}

void TrailRenderer::Render(Camera *camera )
{
    if (points.size() < 2)
        return;

    vertices.clear();
    Vec3 cameraPos = camera->getPosition();

    // Generate vertices for triangle strip
    for (size_t i = 0; i < points.size(); i++)
    //for (int i = points.size() - 1; i >= 0; i--)
    {
        Vec3 pos = points[i].position;
        
        // Calculate direction (tangent to trail)
        Vec3 direction;
        if (i < points.size() - 1)
            direction = (points[i + 1].position - pos).normalized();
        else
            direction = (pos - points[i - 1].position).normalized();
        
        // Calculate right vector (billboard orientation)
        Vec3 toCamera = (cameraPos - pos).normalized();
        Vec3 right = direction.cross(toCamera).normalized();
        
        // Smooth out twist artifacts
        if (i > 0 && vertices.size() >= 2)
        {
            Vec3 prevRight = (vertices[vertices.size() - 1].position - 
                             vertices[vertices.size() - 2].position).normalized();
            float dot = right.dot(prevRight);
            if (dot < 0.0f)
                right = right * -1.0f;
        }
        
        float width = points[i].width;
        float u = (float)i / (points.size() - 1);
        // float u = 1.0f - ((float)i / (points.size() - 1));
        //float u = (float)(points.size() - 1 - i) / (points.size() - 1);
        
        Vec3 col = points[i].color;
        Vec4 color(col.x, col.y, col.z, points[i].alpha);
        
        // Two vertices per point (left and right) for strip
        Vec3 leftPos = pos - right * width;
        Vec3 rightPos = pos + right * width;
        
        vertices.push_back({leftPos, Vec2(u, 0), color});
        vertices.push_back({rightPos, Vec2(u, 1), color});
    }

    if (vertices.empty())
        return;

    // Setup rendering state
    Driver &driver = Driver::Instance();
 
    driver.SetDepthWrite(false);
    driver.SetBlendEnable(true);
    driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::One);

  
    
    if (texture)
        texture->Bind(0);

    // Upload and draw
    vb->SetSubData(0, vertices.size() * sizeof(EffectVertex), vertices.data());
    buffer->Render(PrimitiveType::PT_TRIANGLE_STRIP,  vertices.size());

    driver.SetDepthWrite(true);
 
}

void TrailRenderer::Clear()
{
    points.clear();
}


 

// void RibbonTrail::Chain::RemoveOldest()
// {
//     if (!elements.empty())
//         elements.erase(elements.begin());
// }

// RibbonTrail::RibbonTrail(int maxElements, int numChains)
//     : texture(nullptr)
//     , maxChainElements(maxElements)
//     , trailLength(2.0f)
//     , minDistance(0.1f)
//     , faceCamera(false)
//     , dynamic(true)
// {

//     chains.resize(numChains);
    
//     for (auto& chain : chains)
//     {
//         chain.elements.reserve(maxElements);
//         chain.node = nullptr;
//         chain.offset = Vec3(0, 0, 0);
//         chain.headVisible = true;
//         chain.tailVisible = true;
//     }
    
//     for (int i = 0; i < 4; i++)
//     {
//         initialWidth[i] = 1.0f;
//         widthChange[i] = -0.9f;
//         initialColor[i] = Vec3(1, 1, 1);
//         colorChange[i] = Vec3(0, 0, 0);
//     }

//     vao = new VertexArray();
//     vb = vao->AddVertexBuffer(sizeof(EffectVertex), 
//                               maxElements * numChains * 2 * sizeof(EffectVertex), 
//                               true);
    
//     auto* decl = vao->GetVertexDeclaration();
//     decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
//     decl->AddElement(0, 3 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
//     decl->AddElement(0, 5 * sizeof(float), VET_FLOAT4, VES_COLOR);
    
//     vertices.reserve(maxElements * numChains * 2);


//     int maxIndices = (maxElements - 1) * numChains * 6;

//     indices.reserve(maxIndices);

//     ib = vao->CreateIndexBuffer(maxIndices   , true, false);

    
   
// }

// RibbonTrail::~RibbonTrail()
// {
//     delete vao;
// }
// void RibbonTrail::UpdateChain(Chain& chain, int chainIndex, float time)
// {
//     if (!chain.node) return;

//     Vec3 nodePos = chain.node->getPosition(TransformSpace::World);
//     Quat nodeRot = chain.node->getRotation(TransformSpace::World);
//     Vec3 worldPos = nodePos + nodeRot * chain.offset;

//     bool needsUpdate = chain.elements.empty();
//     if (!needsUpdate)
//     {
//         Vec3 lastPos = chain.elements.back().position;
//         float dist = (worldPos - lastPos).length();
//         needsUpdate = dist > minDistance;
//     }

//     if (needsUpdate)
//     {
//         TrailElement elem;
//         elem.position = worldPos;
//         elem.orientation = nodeRot;
//         elem.timeCreated = time;
        
//         // ✓ Usa o índice correto da chain
//         int idx = Clamp(chainIndex, 0, 3);
//         elem.width = initialWidth[idx];
//         elem.color = initialColor[idx];
//         elem.alpha = 1.0f;
        
//         chain.elements.push_back(elem);
        
//         if (chain.elements.size() > maxChainElements)
//             chain.RemoveOldest();
//     }

//     for (int i = chain.elements.size() - 1; i >= 0; i--)
//     {
//         TrailElement& elem = chain.elements[i];
//         float age = time - elem.timeCreated;
//         float t = (trailLength > 0.0f) ? (age / trailLength) : 0.0f;
//         t = Clamp(t, 0.0f, 1.0f);
 
//         int idx = Clamp(chainIndex, 0, 3);
//         elem.width = initialWidth[idx] + widthChange[idx] * t;
//         elem.color = initialColor[idx] + colorChange[idx] * t;
//         elem.alpha = 1.0f - t;

//         if (age > trailLength && trailLength > 0.0f)
//         {
//             chain.elements.erase(chain.elements.begin() + i);
//         }
//     }
// }

// void RibbonTrail::BuildGeometry(Camera* camera)
// {
//     vertices.clear();
//     indices.clear();  
 

//     Vec3 cameraPos = camera->getPosition(TransformSpace::World);

//     for (size_t chainIdx = 0; chainIdx < chains.size(); chainIdx++)
//     {
//         Chain& chain = chains[chainIdx];
//         if (chain.elements.size() < 2) continue;

//         u32 baseVertex = vertices.size();

//         for (size_t i = 0; i < chain.elements.size(); i++)
//         {
//             TrailElement& elem = chain.elements[i];
//             Vec3 direction;

//             if (i == 0)
//                 direction = (chain.elements[i + 1].position - elem.position).normalized();
//             else if (i == chain.elements.size() - 1)
//                 direction = (elem.position - chain.elements[i - 1].position).normalized();
//             else
//             {
//                 Vec3 avg = (chain.elements[i + 1].position - 
//                            chain.elements[i - 1].position) * 0.5f;
//                 direction = avg.normalized();
//             }

//             Vec3 perpendicular;
//             if (faceCamera)
//             {
//                 Vec3 toCamera = (cameraPos - elem.position).normalized();
//                 perpendicular = direction.cross(toCamera).normalized();
//             }
//             else
//             {
//                 perpendicular = direction.cross(Vec3(0, 1, 0)).normalized();
//                 if (perpendicular.length() < 0.1f)
//                     perpendicular = direction.cross(Vec3(1, 0, 0)).normalized();
//             }

//             float width = elem.width;
//             Vec3 left = elem.position - perpendicular * width;
//             Vec3 right = elem.position + perpendicular * width;

//             float u = (float)i / (chain.elements.size() - 1);
            
//             EffectVertex v0, v1;
//             v0.position = left;
//             v0.texCoord = Vec2(u, 0.0f);
//             v0.color = Vec4(elem.color.x, elem.color.y, elem.color.z, elem.alpha);

//             v1.position = right;
//             v1.texCoord = Vec2(u, 1.0f);
//             v1.color = Vec4(elem.color.x, elem.color.y, elem.color.z, elem.alpha);

//             vertices.push_back(v0);
//             vertices.push_back(v1);

//             if (i < chain.elements.size() - 1)
//             {
//                 u32 v0idx = baseVertex + i * 2;
//                 u32 v1idx = baseVertex + i * 2 + 1;
//                 u32 v2idx = baseVertex + (i + 1) * 2;
//                 u32 v3idx = baseVertex + (i + 1) * 2 + 1;

//                 indices.push_back(v0idx);
//                 indices.push_back(v2idx);
//                 indices.push_back(v1idx);

//                 indices.push_back(v1idx);
//                 indices.push_back(v2idx);
//                 indices.push_back(v3idx);
//             }
//         }
//     }
// }

// void RibbonTrail::SetMaxChainElements(int maxElements)
// {
//     maxChainElements = maxElements;
// }

// void RibbonTrail::SetTrailLength(float length)
// {
//     trailLength = length;
// }

// void RibbonTrail::SetMinDistance(float dist)
// {
//     minDistance = dist;
// }

// void RibbonTrail::SetNumberOfChains(int num)
// {
//     chains.resize(num);
//     for (auto& chain : chains)
//         chain.elements.reserve(maxChainElements);
// }

// void RibbonTrail::SetInitialColor(int chainIndex, const Vec3& color)
// {
//     if (chainIndex < 4)
//         initialColor[chainIndex] = color;
// }

// void RibbonTrail::SetColorChange(int chainIndex, const Vec3& change)
// {
//     if (chainIndex < 4)
//         colorChange[chainIndex] = change;
// }

// void RibbonTrail::SetInitialWidth(int chainIndex, float width)
// {
//     if (chainIndex < 4)
//         initialWidth[chainIndex] = width;
// }

// void RibbonTrail::SetWidthChange(int chainIndex, float change)
// {
//     if (chainIndex < 4)
//         widthChange[chainIndex] = change;
// }

// void RibbonTrail::SetFaceCamera(bool face)
// {
//     faceCamera = face;
// }

// void RibbonTrail::SetTexture(Texture* tex)
// {
//     texture = tex;
// }

// void RibbonTrail::AddNode(Node3D* node, int chainIndex)
// {
//     if (chainIndex >= 0 && chainIndex < chains.size() && chainIndex < 4)
//     {
//         chains[chainIndex].node = node;
//     }
// }

// void RibbonTrail::RemoveNode(Node3D* node)
// {
//     for (auto& chain : chains)
//     {
//         if (chain.node == node)
//         {
//             chain.node = nullptr;
//             chain.elements.clear();
//         }
//     }
// }

// void RibbonTrail::SetChainOffset(int chainIndex, const Vec3& offset)
// {
//     if (chainIndex < chains.size())
//         chains[chainIndex].offset = offset;
// }

// void RibbonTrail::ClearChain(int chainIndex)
// {
//     if (chainIndex < chains.size())
//         chains[chainIndex].elements.clear();
// }

// void RibbonTrail::ClearAllChains()
// {
//     for (auto& chain : chains)
//         chain.elements.clear();
// }

// void RibbonTrail::Update(float time, Camera* camera)
// {
//     for (size_t i = 0; i < chains.size(); i++)
//         UpdateChain(chains[i], i, time);  

//     if (dynamic)
//         BuildGeometry(camera);
// }

// void RibbonTrail::Render( )
// {
//     if (vertices.empty() || indices.empty())
//         return;

//     Driver& driver = Driver::Instance();
    
//     driver.SetBlendEnable(true);
//     driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::One);
//     driver.SetDepthTest(true);
//     driver.SetDepthWrite(false);
 
    
//     if (texture)
//         texture->Bind(0);

//     vb->SetSubData(0, vertices.size() * sizeof(EffectVertex), vertices.data());
//     ib->SetSubData(0, indices.size()  , indices.data());
    
//     vao->Render(PrimitiveType::PT_TRIANGLES, indices.size());
    
//     driver.SetDepthWrite(true);
// }

// int RibbonTrail::GetChainCount() const
// {
//     return chains.size();
// }

// int RibbonTrail::GetMaxChainElements() const
// {
//     return maxChainElements;
// }

// float RibbonTrail::GetTrailLength() const
// {
//     return trailLength;
// }
