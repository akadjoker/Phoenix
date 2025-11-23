#include "pch.h"
#include "Effects.hpp"
#include "Batch.hpp"
#include "Texture.hpp"
#include "Node3D.hpp"

LensFlare::LensFlare(Texture *tex)
    : texture(tex), lightPosWorld(-2.0f, 8.0f, -4.0f), viewAngle(0.0f), borderLimit(0.0f), occluded(false), flareCount(8)
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
    Vec4 clipSpace = projection * view * Vec4(lightPosWorld.x, lightPosWorld.y, lightPosWorld.z, 1.0f);

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
            //RenderFace(batch, screenCenter.x, screenCenter.y, 8.0f, burnClip);
            batch->Rectangle(0,0, screenWidth, screenHeight, true);
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
    : texture(tex), maxPoints(maxPts), lifetime(life), minDistance(0.1f), startWidth(1.0f), endWidth(0.1f), startColor(1, 1, 1), endColor(1, 1, 1), orientationMode(OrientationMode::FaceCamera), fixedNormal(0, 1, 0)
{
    points.reserve(maxPoints);
    vertices.reserve(maxPoints * 2);

    buffer = new VertexArray();
    vb = buffer->AddVertexBuffer(sizeof(EffectVertex),
                                 maxPoints * 2 , true);

    auto *decl = buffer->GetVertexDeclaration();
    decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
    decl->AddElement(0, 3 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    decl->AddElement(0, 5 * sizeof(float), VET_FLOAT4, VES_COLOR);
}

TrailRenderer::~TrailRenderer()
{
    delete buffer;
}

void TrailRenderer::SetOrientation(bool faceCamera)
{
    if (faceCamera)
        orientationMode = OrientationMode::FaceCamera;
    else
        orientationMode = OrientationMode::FixedNormal;
}

void TrailRenderer::SetFixedNormal(const Vec3 &normal)
{
    fixedNormal = normal.normalized();
    orientationMode = OrientationMode::FixedNormal;
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

void TrailRenderer::Render(Camera *camera)
{
    if (points.size() < 2)
        return;

    vertices.clear();
    Vec3 cameraPos = camera->getPosition();

    // Generate vertices for triangle strip
    for (size_t i = 0; i < points.size(); i++)
    {
        Vec3 pos = points[i].position;

        // Calculate direction (tangent to trail)
        Vec3 direction;
        if (i < points.size() - 1)
            direction = (points[i + 1].position - pos).normalized();
        else
            direction = (pos - points[i - 1].position).normalized();

        // Calculate right vector based on orientation mode
        Vec3 right;

        if (orientationMode == OrientationMode::FaceCamera)
        {
            // Billboard: sempre virado para a câmera
            Vec3 toCamera = (cameraPos - pos).normalized();
            right = direction.cross(toCamera).normalized();
        }
        else // OrientationMode::FixedNormal
        {
            // Usa normal fixo para calcular a orientação
            right = direction.cross(fixedNormal).normalized();

            // Fallback se a direção for paralela à normal
            if (right.length() < 0.1f)
            {
                // Encontra um vetor perpendicular alternativo
                Vec3 fallback = (fabs(fixedNormal.y) < 0.9f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
                right = direction.cross(fallback).normalized();
            }
        }

        // Smooth out twist artifacts (só para face camera)
        if (orientationMode == OrientationMode::FaceCamera &&
            i > 0 && vertices.size() >= 2)
        {
            Vec3 prevRight = (vertices[vertices.size() - 1].position -
                              vertices[vertices.size() - 2].position)
                                 .normalized();
            float dot = right.dot(prevRight);
            if (dot < 0.0f)
                right = right * -1.0f;
        }

        float width = points[i].width;
        float u = (float)i / (points.size() - 1);

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
    driver.SetCulling(CullMode::None);

    if (texture)
        texture->Bind(0);

    // Upload and draw
    vb->SetSubData(0, vertices.size() * sizeof(EffectVertex), vertices.data());
    buffer->Render(PrimitiveType::PT_TRIANGLE_STRIP, vertices.size());

    driver.SetDepthWrite(true);
    driver.SetCulling(CullMode::Back);
}

void TrailRenderer::Clear()
{
    points.clear();
}

//*************************************************************************************************************** */

void RibbonTrail::Chain::RemoveOldest()
{
    if (!elements.empty())
        elements.erase(elements.begin());
}

Vec3 RibbonTrail::Chain::GetWorldPosition() const
{
    if (node)
    {
        Vec3 nodePos = node->getPosition(TransformSpace::World);
        Quat nodeRot = node->getRotation(TransformSpace::World);
        return nodePos + nodeRot * localOffset;
    }
    else if (externalPosition)
    {
        return *externalPosition + localOffset;
    }
    return Vec3(0, 0, 0);
}

RibbonTrail::RibbonTrail(int maxElements, int numChains)
    : texture(nullptr), maxChainElements(maxElements), trailLength(2.0f), minDistance(0.1f), faceCamera(false), dynamic(true), connectionMode(ConnectionMode::Independent)
{
    if (numChains > MAX_CHAINS)
        numChains = MAX_CHAINS;
    chains.resize(numChains);

    for (auto &chain : chains)
    {
        chain.elements.reserve(maxElements);
        chain.node = nullptr;
        chain.externalPosition = nullptr;
        chain.offset = Vec3(0, 0, 0);
        chain.localOffset = Vec3(0, 0, 0);
        chain.headVisible = true;
        chain.tailVisible = true;
        chain.enabled = true;
    }

    for (int i = 0; i < 4; i++)
    {
        initialWidth[i] = 1.0f;
        widthChange[i] = -0.9f;
        initialColor[i] = Vec3(1, 1, 1);
        colorChange[i] = Vec3(0, 0, 0);
    }

    vao = new VertexArray();

    // Para modo conectado, precisamos de mais vértices e índices
    int maxVertices = maxElements * numChains * 4; // Extra para conexões
    vb = vao->AddVertexBuffer(sizeof(EffectVertex),
                              maxVertices ,
                              true);

    auto *decl = vao->GetVertexDeclaration();
    decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
    decl->AddElement(0, 3 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    decl->AddElement(0, 5 * sizeof(float), VET_FLOAT4, VES_COLOR);

    vertices.reserve(maxVertices);

    int maxIndices = (maxElements - 1) * numChains * 12; // Extra para conexões
    indices.reserve(maxIndices);
    ib = vao->CreateIndexBuffer(maxIndices, true, true);
}

RibbonTrail::~RibbonTrail()
{
    delete vao;
}

void RibbonTrail::UpdateChain(Chain &chain, int chainIndex, float time)
{
    if (!chain.enabled)
        return;

    if (!chain.node && !chain.externalPosition)
        return;

    Vec3 worldPos = chain.GetWorldPosition();

    bool needsUpdate = chain.elements.empty();
    if (!needsUpdate)
    {
        Vec3 lastPos = chain.elements.back().position;
        float dist = (worldPos - lastPos).length();
        needsUpdate = dist > minDistance;
    }

    if (needsUpdate)
    {
        TrailElement elem;
        elem.position = worldPos;
        elem.orientation = chain.node ? chain.node->getRotation(TransformSpace::World) : Quat();
        elem.timeCreated = time;

        int idx = Clamp(chainIndex, 0, 3);
        elem.width = initialWidth[idx];
        elem.color = initialColor[idx];
        elem.alpha = 1.0f;

        chain.elements.push_back(elem);

        if (chain.elements.size() > maxChainElements)
            chain.RemoveOldest();
    }

    // Update existing elements
    for (int i = chain.elements.size() - 1; i >= 0; i--)
    {
        TrailElement &elem = chain.elements[i];
        float age = time - elem.timeCreated;
        float t = (trailLength > 0.0f) ? (age / trailLength) : 0.0f;
        t = Clamp(t, 0.0f, 1.0f);

        int idx = Clamp(chainIndex, 0, 3);
        elem.width = initialWidth[idx] + widthChange[idx] * t;
        elem.color = initialColor[idx] + colorChange[idx] * t;
        elem.alpha = 1.0f - t;

        if (age > trailLength && trailLength > 0.0f)
        {
            chain.elements.erase(chain.elements.begin() + i);
        }
    }
}

void RibbonTrail::BuildGeometryIndependent(Camera *camera)
{
    vertices.clear();
    indices.clear();

    Vec3 cameraPos = camera->getPosition(TransformSpace::World);

    for (size_t chainIdx = 0; chainIdx < chains.size(); chainIdx++)
    {
        Chain &chain = chains[chainIdx];
        if (!chain.enabled || chain.elements.size() < 2)
            continue;

        u32 baseVertex = vertices.size();

        for (size_t i = 0; i < chain.elements.size(); i++)
        {
            TrailElement &elem = chain.elements[i];
            Vec3 direction;

            if (i == 0)
                direction = (chain.elements[i + 1].position - elem.position).normalized();
            else if (i == chain.elements.size() - 1)
                direction = (elem.position - chain.elements[i - 1].position).normalized();
            else
            {
                Vec3 avg = (chain.elements[i + 1].position -
                            chain.elements[i - 1].position) *
                           0.5f;
                direction = avg.normalized();
            }

            Vec3 perpendicular;
            if (faceCamera)
            {
                Vec3 toCamera = (cameraPos - elem.position).normalized();
                perpendicular = direction.cross(toCamera).normalized();
            }
            else
            {
                perpendicular = direction.cross(Vec3(0, 1, 0)).normalized();
                if (perpendicular.length() < 0.1f)
                    perpendicular = direction.cross(Vec3(1, 0, 0)).normalized();
            }

            float width = elem.width;
            Vec3 left = elem.position - perpendicular * width;
            Vec3 right = elem.position + perpendicular * width;

            float u = (float)i / (chain.elements.size() - 1);

            EffectVertex v0, v1;
            v0.position = left;
            v0.texCoord = Vec2(u, 0.0f);
            v0.color = Vec4(elem.color.x, elem.color.y, elem.color.z, elem.alpha);

            v1.position = right;
            v1.texCoord = Vec2(u, 1.0f);
            v1.color = Vec4(elem.color.x, elem.color.y, elem.color.z, elem.alpha);

            vertices.push_back(v0);
            vertices.push_back(v1);

            if (i < chain.elements.size() - 1)
            {
                u32 v0idx = baseVertex + i * 2;
                u32 v1idx = baseVertex + i * 2 + 1;
                u32 v2idx = baseVertex + (i + 1) * 2;
                u32 v3idx = baseVertex + (i + 1) * 2 + 1;

                indices.push_back(v0idx);
                indices.push_back(v2idx);
                indices.push_back(v1idx);

                indices.push_back(v1idx);
                indices.push_back(v2idx);
                indices.push_back(v3idx);
            }
        }
    }
}

void RibbonTrail::BuildGeometryConnected(Camera *camera)
{
    vertices.clear();
    indices.clear();

    // Precisamos de pelo menos 2 chains ativas para conectar
    int activeChains = 0;
    for (auto &chain : chains)
        if (chain.enabled && chain.elements.size() >= 2)
            activeChains++;

    if (activeChains < 2)
    {
        BuildGeometryIndependent(camera);
        return;
    }

    //  Vec3 cameraPos = camera->getPosition(TransformSpace::World);

    // Encontrar o número mínimo de elementos entre todas as chains ativas
    size_t minElements = maxChainElements;
    for (auto &chain : chains)
        if (chain.enabled && !chain.elements.empty())
            minElements = std::min(minElements, chain.elements.size());

    if (minElements < 2)
        return;

    // Gerar vértices para cada ponto de todas as chains
    for (size_t i = 0; i < minElements; i++)
    {
        for (size_t chainIdx = 0; chainIdx < chains.size(); chainIdx++)
        {
            Chain &chain = chains[chainIdx];
            if (!chain.enabled || i >= chain.elements.size())
                continue;

            TrailElement &elem = chain.elements[i];
            float u = (float)i / (minElements - 1);
            float v = (float)chainIdx / (chains.size() - 1);

            EffectVertex vert;
            vert.position = elem.position;
            vert.texCoord = Vec2(u, v);
            vert.color = Vec4(elem.color.x, elem.color.y, elem.color.z, elem.alpha);

            vertices.push_back(vert);
        }
    }

    // Gerar índices conectando as chains
    u32 numChains = chains.size();
    for (size_t i = 0; i < minElements - 1; i++)
    {
        for (size_t c = 0; c < numChains - 1; c++)
        {
            u32 v0 = i * numChains + c;
            u32 v1 = i * numChains + c + 1;
            u32 v2 = (i + 1) * numChains + c;
            u32 v3 = (i + 1) * numChains + c + 1;

            // Triângulo 1
            indices.push_back(v0);
            indices.push_back(v2);
            indices.push_back(v1);

            // Triângulo 2
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v3);
        }
    }
}

void RibbonTrail::SetChainPosition(int chainIndex, Vec3 *position)
{
    if (chainIndex > MAX_CHAINS)
        chainIndex = MAX_CHAINS;
    if (chainIndex >= 0 && chainIndex < chains.size())
    {
        chains[chainIndex].externalPosition = position;
        chains[chainIndex].node = nullptr; // Prioriza position sobre node
    }
}

void RibbonTrail::SetChainEnabled(int chainIndex, bool enabled)
{
    if (chainIndex > MAX_CHAINS)
        chainIndex = MAX_CHAINS;

    if (chainIndex >= 0 && chainIndex < chains.size())
        chains[chainIndex].enabled = enabled;
}

void RibbonTrail::SetMaxChainElements(int maxElements)
{
    maxChainElements = maxElements;
}

void RibbonTrail::SetTrailLength(float length)
{
    trailLength = length;
}

void RibbonTrail::SetMinDistance(float dist)
{
    minDistance = dist;
}

void RibbonTrail::SetNumberOfChains(int num)
{
    if (num > 4)
        num = 4;
    chains.resize(num);
    for (auto &chain : chains)
    {
        chain.elements.reserve(maxChainElements);
        chain.enabled = true;
    }
}

void RibbonTrail::SetConnectionAtachment(bool connected)
{
    if (connected)
        connectionMode = ConnectionMode::Connected;
    else
        connectionMode = ConnectionMode::Independent;
}

void RibbonTrail::SetInitialColor(int chainIndex, const Vec3 &color)
{
    if (chainIndex >= 0 && chainIndex < 4)
        initialColor[chainIndex] = color;
}

void RibbonTrail::SetColorChange(int chainIndex, const Vec3 &change)
{
    if (chainIndex >= 0 && chainIndex < 4)
        colorChange[chainIndex] = change;
}

void RibbonTrail::SetInitialWidth(int chainIndex, float width)
{
    if (chainIndex >= 0 && chainIndex < 4)
        initialWidth[chainIndex] = width;
}

void RibbonTrail::SetWidthChange(int chainIndex, float change)
{
    if (chainIndex >= 0 && chainIndex < 4)
        widthChange[chainIndex] = change;
}

void RibbonTrail::SetFaceCamera(bool face)
{
    faceCamera = face;
}

void RibbonTrail::SetTexture(Texture *tex)
{
    texture = tex;
}

void RibbonTrail::AddNode(Node3D *node, int chainIndex)
{
    if (chainIndex >= 0 && chainIndex < chains.size() && chainIndex < 4)
    {
        chains[chainIndex].node = node;
        chains[chainIndex].externalPosition = nullptr;
    }
}

void RibbonTrail::RemoveNode(Node3D *node)
{
    for (auto &chain : chains)
    {
        if (chain.node == node)
        {
            chain.node = nullptr;
            chain.elements.clear();
        }
    }
}

void RibbonTrail::SetChainOffset(int chainIndex, const Vec3 &offset)
{
    if (chainIndex >= 0 && chainIndex < chains.size())
        chains[chainIndex].localOffset = offset;
}

void RibbonTrail::ClearChain(int chainIndex)
{
    if (chainIndex >= 0 && chainIndex < chains.size())
        chains[chainIndex].elements.clear();
}

void RibbonTrail::ClearAllChains()
{
    for (auto &chain : chains)
        chain.elements.clear();
}

void RibbonTrail::Update(float time, Camera *camera)
{
    for (size_t i = 0; i < chains.size(); i++)
        UpdateChain(chains[i], i, time);

    if (dynamic)
    {
        if (connectionMode == ConnectionMode::Connected)
            BuildGeometryConnected(camera);
        else
            BuildGeometryIndependent(camera);
    }
}

void RibbonTrail::Render()
{
    if (vertices.empty() || indices.empty())
        return;

    Driver &driver = Driver::Instance();

    driver.SetBlendEnable(true);
    driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::One);
    driver.SetDepthTest(true);
    driver.SetDepthWrite(false);
    driver.SetCulling(CullMode::None);

    if (texture)
        texture->Bind(0);

    vb->SetSubData(0, vertices.size() * sizeof(EffectVertex), vertices.data());
    ib->SetSubData(0, indices.size(), indices.data());

    vao->Render(PrimitiveType::PT_TRIANGLES, indices.size());

    driver.SetDepthWrite(true);
    driver.SetBlendEnable(false);
}

int RibbonTrail::GetChainCount() const
{
    return chains.size();
}

int RibbonTrail::GetMaxChainElements() const
{
    return maxChainElements;
}

float RibbonTrail::GetTrailLength() const
{
    return trailLength;
}

RibbonTrail::ConnectionMode RibbonTrail::GetConnectionMode() const
{
    return connectionMode;
}

// // ============================================================
// // EXEMPLOS DE USO
// // ============================================================

// void ExemploTrailsIndependentes()
// {
//     // Múltiplos trails separados (modo atual)
//     RibbonTrail* trail = new RibbonTrail(100, 3);
//     trail->SetConnectionMode(RibbonTrail::ConnectionMode::Independent);

//     trail->AddNode(pontaAsaEsquerda, 0);
//     trail->AddNode(pontaAsaDireita, 1);
//     trail->AddNode(cauda, 2);
// }

// void ExemploEspadaDeLuz()
// {
//     // Espada de luz - volume conectado
//     RibbonTrail* lightsaber = new RibbonTrail(50, 2);
//     lightsaber->SetConnectionMode(RibbonTrail::ConnectionMode::Connected);

//     lightsaber->AddNode(pontaEspada, 0);
//     lightsaber->AddNode(caboEspada, 1);

//     // Configurar cor (azul brilhante)
//     lightsaber->SetInitialColor(0, Vec3(0.3f, 0.5f, 1.0f));
//     lightsaber->SetInitialColor(1, Vec3(0.3f, 0.5f, 1.0f));
//     lightsaber->SetTrailLength(0.5f);
// }

// void ExemploUsandoPosicoes()
// {
//     // Usando posições diretas em vez de nodes
//     Vec3 pos1(0, 0, 0);
//     Vec3 pos2(1, 0, 0);

//     RibbonTrail* trail = new RibbonTrail(100, 2);
//     trail->SetConnectionMode(RibbonTrail::ConnectionMode::Connected);

//     trail->SetChainPosition(0, &pos1);
//     trail->SetChainPosition(1, &pos2);

//     //  atualizar pos1 e pos2 diretamente!
//     // O trail vai seguir automaticamente
// }

// void ExemploAsasAviao()
// {
//     // 4 trails conectados formando superfície nas asas
//     RibbonTrail* wingTrail = new RibbonTrail(80, 4);
//     wingTrail->SetConnectionMode(RibbonTrail::ConnectionMode::Connected);

//     wingTrail->AddNode(asaEsquerdaPonta, 0);
//     wingTrail->AddNode(asaEsquerdaMeio, 1);
//     wingTrail->AddNode(asaDireitaMeio, 2);
//     wingTrail->AddNode(asaDireitaPonta, 3);

//     // Gradient de cor
//     wingTrail->SetInitialColor(0, Vec3(1, 0, 0));
//     wingTrail->SetInitialColor(3, Vec3(0, 0, 1));
// }

//************************************************************************************ */

BillboardSet::BillboardSet(int maxBillboards, BillboardType type)
    : type(type), texture(nullptr), maxBillboards(maxBillboards), defaultAxis(0, 1, 0), autoUpdate(true)
{
    billboards.reserve(maxBillboards);

    vao = new VertexArray();

    // Para Cross e CrossY, cada billboard tem 2 quads = 8 vértices
    int maxVertices = maxBillboards * 8;
    vb = vao->AddVertexBuffer(sizeof(EffectVertex),
                              maxVertices,
                              true);

    auto *decl = vao->GetVertexDeclaration();
    decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
    decl->AddElement(0, 3 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    decl->AddElement(0, 5 * sizeof(float), VET_FLOAT4, VES_COLOR);

    vertices.reserve(maxVertices);

    // Para Cross e CrossY: 2 quads = 12 índices por billboard
    int maxIndices = maxBillboards * 12;
    indices.reserve(maxIndices);
    ib = vao->CreateIndexBuffer(maxIndices, true, true);
}

BillboardSet::~BillboardSet()
{
    delete vao;
}

int BillboardSet::AddBillboard(const Vec3 &position, const Vec2 &size)
{
    if (billboards.size() >= maxBillboards)
        return -1;

    Billboard bb;
    bb.position = position;
    bb.size = size;
    bb.axis = defaultAxis;

    billboards.push_back(bb);
    return billboards.size() - 1;
}

void BillboardSet::RemoveBillboard(int index)
{
    if (index >= 0 && index < billboards.size())
        billboards.erase(billboards.begin() + index);
}

void BillboardSet::Clear()
{
    billboards.clear();
}

void BillboardSet::SetBillboardPosition(int index, const Vec3 &position)
{
    if (index >= 0 && index < billboards.size())
        billboards[index].position = position;
}

void BillboardSet::SetBillboardSize(int index, const Vec2 &size)
{
    if (index >= 0 && index < billboards.size())
        billboards[index].size = size;
}

void BillboardSet::SetBillboardColor(int index, const Vec3 &color)
{
    if (index >= 0 && index < billboards.size())
        billboards[index].color = color;
}

void BillboardSet::SetBillboardAlpha(int index, float alpha)
{
    if (index >= 0 && index < billboards.size())
        billboards[index].alpha = alpha;
}

void BillboardSet::SetBillboardRotation(int index, float rotation)
{
    if (index >= 0 && index < billboards.size())
        billboards[index].rotation = rotation;
}

void BillboardSet::SetBillboardDirection(int index, const Vec3 &direction)
{
    if (index >= 0 && index < billboards.size())
        billboards[index].direction = direction.normalized();
}

void BillboardSet::SetBillboardVisible(int index, bool visible)
{
    if (index >= 0 && index < billboards.size())
        billboards[index].visible = visible;
}

void BillboardSet::SetBillboardType(BillboardType newType)
{
    type = newType;
}

void BillboardSet::SetTexture(Texture *tex)
{
    texture = tex;
}

void BillboardSet::SetDefaultAxis(const Vec3 &axis)
{
    defaultAxis = axis.normalized();
}

void BillboardSet::SetAutoUpdate(bool enabled)
{
    autoUpdate = enabled;
}

void BillboardSet::SetAllColors(const Vec3 &color)
{
    for (auto &bb : billboards)
        bb.color = color;
}

void BillboardSet::SetAllSizes(const Vec2 &size)
{
    for (auto &bb : billboards)
        bb.size = size;
}

void BillboardSet::SetAllAlpha(float alpha)
{
    for (auto &bb : billboards)
        bb.alpha = alpha;
}

BillboardSet::Billboard *BillboardSet::GetBillboard(int index)
{
    if (index >= 0 && index < billboards.size())
        return &billboards[index];
    return nullptr;
}

int BillboardSet::GetBillboardCount() const
{
    return billboards.size();
}

BillboardSet::BillboardType BillboardSet::GetBillboardType() const
{
    return type;
}

void BillboardSet::AddQuad(const Vec3 &center, const Vec3 &right, const Vec3 &up,
                           const Vec2 &size, const Vec4 &color)
{
    Vec3 halfRight = right * (size.x * 0.5f);
    Vec3 halfUp = up * (size.y * 0.5f);

    u32 baseVertex = vertices.size();

    // 4 vértices do quad
    EffectVertex v0, v1, v2, v3;

    v0.position = center - halfRight - halfUp;
    v0.texCoord = Vec2(0, 0);
    v0.color = color;

    v1.position = center + halfRight - halfUp;
    v1.texCoord = Vec2(1, 0);
    v1.color = color;

    v2.position = center + halfRight + halfUp;
    v2.texCoord = Vec2(1, 1);
    v2.color = color;

    v3.position = center - halfRight + halfUp;
    v3.texCoord = Vec2(0, 1);
    v3.color = color;

    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);

    // 2 triângulos
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 1);
    indices.push_back(baseVertex + 2);

    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 3);
}

void BillboardSet::BuildGeometry(Camera *camera)
{
    vertices.clear();
    indices.clear();

    Vec3 cameraPos = camera->getPosition(TransformSpace::World);
    Vec3 cameraUp = camera->getUp();
    //Vec3 cameraRight = camera->getRight(TransformSpace::World);

    for (const auto &bb : billboards)
    {
        if (!bb.visible)
            continue;

        Vec4 color(bb.color.x, bb.color.y, bb.color.z, bb.alpha);

        switch (type)
        {
        case BillboardType::Point:
        {
            // Billboard clássico: sempre virado para câmera
            Vec3 toCamera = (cameraPos - bb.position).normalized();
            Vec3 right = toCamera.cross(cameraUp).normalized();
            Vec3 up = right.cross(toCamera).normalized();

            // Aplicar rotação se necessário
            if (bb.rotation != 0.0f)
            {
                float c = cos(bb.rotation);
                float s = sin(bb.rotation);
                Vec3 newRight = right * c - up * s;
                Vec3 newUp = right * s + up * c;
                right = newRight;
                up = newUp;
            }

            AddQuad(bb.position, right, up, bb.size, color);
            break;
        }

        case BillboardType::Oriented:
        {
            // Billboard com direção custom
            Vec3 forward = bb.direction.normalized();
            Vec3 right = forward.cross(Vec3(0, 1, 0)).normalized();

            // Fallback se direção é paralela ao Y
            if (right.length() < 0.1f)
                right = forward.cross(Vec3(1, 0, 0)).normalized();

            Vec3 up = right.cross(forward).normalized();

            AddQuad(bb.position, right, up, bb.size, color);
            break;
        }

        case BillboardType::AxisAligned:
        {
            // Billboard alinhado a um eixo (geralmente Y)
            Vec3 toCamera = (cameraPos - bb.position).normalized();
            Vec3 right = toCamera.cross(bb.axis).normalized();
            Vec3 up = bb.axis.normalized();

            AddQuad(bb.position, right, up, bb.size, color);
            break;
        }

        case BillboardType::Cross:
        {
            // 2 quads perpendiculares (forma de X vista de cima)
            Vec3 right1(1, 0, 0);
            Vec3 up1(0, 1, 0);
            Vec3 right2(0, 0, 1);
            Vec3 up2(0, 1, 0);

            AddQuad(bb.position, right1, up1, bb.size, color);
            AddQuad(bb.position, right2, up2, bb.size, color);
            break;
        }

        case BillboardType::CrossY:
        {
            // 2 quads perpendiculares alinhados ao eixo Y
 
            Vec3 toCamera = (cameraPos - bb.position).normalized();
            toCamera.y = 0; // Projetar no plano XZ
            toCamera = toCamera.normalized();

            Vec3 right1 = toCamera;
            Vec3 up1(0, 1, 0);

            Vec3 right2 = toCamera.cross(Vec3(0, 1, 0)).normalized();
            Vec3 up2(0, 1, 0);

            AddQuad(bb.position, right1, up1, bb.size, color);
            AddQuad(bb.position, right2, up2, bb.size, color);
            break;
        }
        }
    }
}

void BillboardSet::Update(Camera *camera)
{
    if (autoUpdate)
        BuildGeometry(camera);
}

void BillboardSet::Render()
{
    if (vertices.empty() || indices.empty())
        return;

    Driver &driver = Driver::Instance();

    driver.SetBlendEnable(true);
    driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    driver.SetDepthTest(true);
    driver.SetDepthWrite(false);

    if (texture)
        texture->Bind(0);

    vb->SetSubData(0, vertices.size() * sizeof(EffectVertex), vertices.data());
    ib->SetSubData(0, indices.size(), indices.data());

    vao->Render(PrimitiveType::PT_TRIANGLES, indices.size());

    driver.SetDepthWrite(true);
}


//*************************************************************************************************************** */

StaticBillboardBatch::StaticBillboardBatch(BillboardType type)
    :  vao(nullptr), vb(nullptr), ib(nullptr), texture(nullptr),type(type), defaultSize(1, 1), isBuilt(false), billboardCount(0)
{
}

StaticBillboardBatch::~StaticBillboardBatch()
{
    if (vao)
        delete vao;
}

void StaticBillboardBatch::Begin()
{
    vertices.clear();
    indices.clear();
    billboardCount = 0;
    isBuilt = false;
    if (vao)
    {
        delete vao;
        vao = nullptr;
        vb = nullptr;
        ib = nullptr;
    }
}

void StaticBillboardBatch::AddQuad(const Vec3 &center, const Vec3 &right, const Vec3 &up,const Vec2 &size, const Vec4 &color)
{
    Vec3 halfRight = right * (size.x * 0.5f);
    Vec3 halfUp = up * (size.y * 0.5f);

    u32 baseVertex = vertices.size();

    EffectVertex v0, v1, v2, v3;

    // Bottom-left
    v0.position = center - halfRight - halfUp;
    v0.texCoord = Vec2(0, 0);
    v0.color = color;

    // Bottom-right
    v1.position = center + halfRight - halfUp;
    v1.texCoord = Vec2(1, 0);
    v1.color = color;

    // Top-right
    v2.position = center + halfRight + halfUp;
    v2.texCoord = Vec2(1, 1);
    v2.color = color;

    // Top-left
    v3.position = center - halfRight + halfUp;
    v3.texCoord = Vec2(0, 1);
    v3.color = color;

    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);

    // 2 triângulos
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 1);
    indices.push_back(baseVertex + 2);

    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 3);
}

void StaticBillboardBatch::AddPoint(const Vec3 &position, const Vec3 &normal,const Vec2 &size, const Vec4 &color)
{
    if (isBuilt)
    {
        LogWarning("StaticBillboardBatch::AddPoint  already built. Call Begin() to reset.");
        return;
    }

    Vec3 up = normal.normalized();

    // Calcular vetores perpendiculares à normal
    Vec3 right;
    if (fabs(up.y) < 0.99f)
        right = up.cross(Vec3(0, 1, 0)).normalized();
    else
        right = up.cross(Vec3(1, 0, 0)).normalized();

    Vec3 forward = right.cross(up).normalized();

    switch (type)
    {
    case BillboardType::Single:
    {
        // 1 quad alinhado à normal
        AddQuad(position, right, up, size, color);
        break;
    }

    case BillboardType::Cross:
    {
        // 2 quads perpendiculares
        AddQuad(position, right, up, size, color);
        AddQuad(position, forward, up, size, color);
        break;
    }

    case BillboardType::TriCross:
    {
        // 3 quads com 60 graus entre eles
        AddQuad(position, right, up, size, color);

        // Rodar 60 graus
        float angle1 = 1.0472f; // 60 graus em radianos
        Vec3 right1 = right * cos(angle1) + forward * sin(angle1);
        AddQuad(position, right1, up, size, color);

        // Rodar 120 graus
        float angle2 = 2.0944f; // 120 graus
        Vec3 right2 = right * cos(angle2) + forward * sin(angle2);
        AddQuad(position, right2, up, size, color);
        break;
    }
    }

    billboardCount++;
}

void StaticBillboardBatch::AddPoint(const Vec3 &position, const Vec3 &normal)
{

    AddPoint(position, normal, defaultSize, Vec4(1, 1, 1, 1));
}

void StaticBillboardBatch::End()
{
    if (isBuilt || vertices.empty())
        return;

    vao = new VertexArray();

    vb = vao->AddVertexBuffer(sizeof(EffectVertex),
                              vertices.size(),
                              false);

    auto *decl = vao->GetVertexDeclaration();
    decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
    decl->AddElement(0, 3 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    decl->AddElement(0, 5 * sizeof(float), VET_FLOAT4, VES_COLOR);

    ib = vao->CreateIndexBuffer(indices.size(), false, true);
    

    vb->SetData(vertices.data());
    ib->SetData(indices.data());

    isBuilt = true;

    vertices.clear();
    indices.clear();
}

void StaticBillboardBatch::SetTexture(Texture *tex)
{
    texture = tex;
}

void StaticBillboardBatch::SetDefaultSize(const Vec2 &size)
{
    defaultSize = size;
}

void StaticBillboardBatch::SetBillboardType(BillboardType newType)
{
    if (!isBuilt)
        type = newType;
}

void StaticBillboardBatch::Render()
{
    if (!isBuilt || !vao)
        return;

    Driver &driver = Driver::Instance();

    driver.SetBlendEnable(true);
    driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    driver.SetDepthTest(true);
    driver.SetDepthWrite(false);
    driver.SetCulling(CullMode::None);

    if (texture)
        texture->Bind(0);

    vao->Render(PrimitiveType::PT_TRIANGLES, ib->GetIndexCount());

    driver.SetDepthWrite(true);
}

void StaticBillboardBatch::Clear()
{
    vertices.clear();
    indices.clear();
    billboardCount = 0;
    isBuilt = false;

    if (vao)
    {
        delete vao;
        vao = nullptr;
        vb = nullptr;
        ib = nullptr;
    }
}


//************************************************************************************


DecalManager::DecalManager(int maxDecals)
    : maxDecals(maxDecals)
    , defaultLifetime(10.0f)
    , defaultFadeStart(0.8f)
    , defaultSize(1, 1)
{
    decals.reserve(maxDecals);
    
    vao = new VertexArray();
    
 
    int maxVertices = maxDecals * 4;
    vb = vao->AddVertexBuffer(sizeof(EffectVertex), 
                              maxVertices , 
                              true);
    
    auto* decl = vao->GetVertexDeclaration();
    decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
    decl->AddElement(0, 3 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    decl->AddElement(0, 5 * sizeof(float), VET_FLOAT4, VES_COLOR);
    
    vertices.reserve(maxVertices);
    
 
    int maxIndices = maxDecals * 6;
    indices.reserve(maxIndices);
    ib = vao->CreateIndexBuffer(maxIndices, true, true);
}

DecalManager::~DecalManager()
{
    delete vao;
}

int DecalManager::AddDecal(const Vec3& position, const Vec3& normal, float lifetime)
{
    return AddDecal(position, normal, defaultSize, Vec4(1, 1, 1, 1), lifetime);
}

int DecalManager::AddDecal(const Vec3& position, const Vec3& normal, 
                           const Vec2& size, const Vec4& color, float lifetime)
{
     
    if (decals.size() >= maxDecals)
    {
        RemoveOldestDecal();
    }
    
    Decal decal;
    decal.position = position;
    decal.normal = normal.normalized();
    decal.size = size;
    decal.color = color;
    decal.lifetime = (lifetime < 0) ? defaultLifetime : lifetime;
    decal.fadeStart = defaultFadeStart;
    decal.timeAlive = 0.0f;
    decal.active = true;
    
    // Calcular tangente
    Vec3 up = Vec3(0, 1, 0);
    if (fabs(decal.normal.dot(up)) > 0.99f)
        up = Vec3(1, 0, 0);
    
    decal.tangent = decal.normal.cross(up).normalized();
    
    decals.push_back(decal);
    return decals.size() - 1;
}

int DecalManager::AddDecalOriented(const Vec3& position, const Vec3& normal, 
                                   float rotation, const Vec2& size, float lifetime)
{
    int index = AddDecal(position, normal, size, Vec4(1, 1, 1, 1), lifetime);
    if (index >= 0)
        decals[index].rotation = rotation;
    return index;
}

void DecalManager::SetDecalColor(int index, const Vec4& color)
{
    if (index >= 0 && index < decals.size())
        decals[index].color = color;
}

void DecalManager::SetDecalSize(int index, const Vec2& size)
{
    if (index >= 0 && index < decals.size())
        decals[index].size = size;
}

void DecalManager::SetDecalRotation(int index, float rotation)
{
    if (index >= 0 && index < decals.size())
        decals[index].rotation = rotation;
}

 

void DecalManager::SetDefaultLifetime(float lifetime)
{
    defaultLifetime = lifetime;
}

void DecalManager::SetDefaultFadeStart(float fadeStart)
{
    defaultFadeStart = Clamp(fadeStart, 0.0f, 1.0f);
}

void DecalManager::SetDefaultSize(const Vec2& size)
{
    defaultSize = size;
}

 

void DecalManager::SetTexture(  Texture* tex)
{
    texture = tex;
}

 

void DecalManager::RemoveDecal(int index)
{
    if (index >= 0 && index < decals.size())
        decals.erase(decals.begin() + index);
}

void DecalManager::RemoveAllDecals()
{
    decals.clear();
}

void DecalManager::RemoveOldestDecal()
{
    if (decals.empty())
        return;
    
 
    int oldestIndex = 0;
    float maxAge = decals[0].timeAlive;
    
    for (size_t i = 1; i < decals.size(); i++)
    {
        if (decals[i].timeAlive > maxAge)
        {
            maxAge = decals[i].timeAlive;
            oldestIndex = i;
        }
    }
    
    decals.erase(decals.begin() + oldestIndex);
}

void DecalManager::AddDecalQuad(const Decal& decal)
{
    Vec3 normal = decal.normal;
    Vec3 tangent = decal.tangent;
    Vec3 bitangent = normal.cross(tangent).normalized();
    
    // Aplicar rotação se necessário
    if (decal.rotation != 0.0f)
    {
        float c = cos(decal.rotation);
        float s = sin(decal.rotation);
        Vec3 newTangent = tangent * c + bitangent * s;
        Vec3 newBitangent = tangent * -s + bitangent * c;
        tangent = newTangent;
        bitangent = newBitangent;
    }
    
    Vec3 halfRight = tangent * (decal.size.x * 0.5f);
    Vec3 halfUp = bitangent * (decal.size.y * 0.5f);
    
    // Offset ligeiro ao longo da normal para evitar z-fighting
    Vec3 center = decal.position + normal * 0.01f;
    
    u32 baseVertex = vertices.size();
    
 
    float alpha = decal.color.w;
    if (decal.lifetime > 0.0f)
    {
        float t = decal.timeAlive / decal.lifetime;
        if (t > decal.fadeStart)
        {
            float fadeT = (t - decal.fadeStart) / (1.0f - decal.fadeStart);
            alpha *= (1.0f - fadeT);
        }
    }
    
    Vec4 color(decal.color.x, decal.color.y, decal.color.z, alpha);
    
    EffectVertex v0, v1, v2, v3;
    
    v0.position = center - halfRight - halfUp;
    v0.texCoord = Vec2(0, 0);
    v0.color = color;
    
    v1.position = center + halfRight - halfUp;
    v1.texCoord = Vec2(1, 0);
    v1.color = color;
    
    v2.position = center + halfRight + halfUp;
    v2.texCoord = Vec2(1, 1);
    v2.color = color;
    
    v3.position = center - halfRight + halfUp;
    v3.texCoord = Vec2(0, 1);
    v3.color = color;
    
    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 1);
    indices.push_back(baseVertex + 2);
    
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 3);
}

void DecalManager::BuildGeometry()
{
    vertices.clear();
    indices.clear();
    
    for (const auto& decal : decals)
    {
        if (decal.active)
            AddDecalQuad(decal);
    }
}

void DecalManager::Update(float deltaTime)
{
    
    for (int i = decals.size() - 1; i >= 0; i--)
    {
        Decal& decal = decals[i];
        decal.timeAlive += deltaTime;
        
   
        if (decal.lifetime > 0.0f && decal.timeAlive >= decal.lifetime)
        {
            decals.erase(decals.begin() + i);
        }
    }
    
 
    BuildGeometry();
}

void DecalManager::Render()
{
    if (vertices.empty() || indices.empty())
        return;
    
    Driver& driver = Driver::Instance();
    
    driver.SetBlendEnable(true);
    driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    driver.SetDepthTest(true);
    driver.SetDepthWrite(false);
    driver.SetCulling(CullMode::None);
    
 
    if (texture)
        texture->Bind(0);
    
    vb->SetSubData(0, vertices.size() * sizeof(EffectVertex), vertices.data());
    ib->SetSubData(0, indices.size(), indices.data());
    
    vao->Render(PrimitiveType::PT_TRIANGLES, indices.size());
    
    driver.SetDepthWrite(true);
}

int DecalManager::GetDecalCount() const
{
    return decals.size();
}

int DecalManager::GetActiveDecalCount() const
{
    int count = 0;
    for (const auto& decal : decals)
        if (decal.active)
            count++;
    return count;
}

DecalManager::Decal* DecalManager::GetDecal(int index)
{
    if (index >= 0 && index < decals.size())
        return &decals[index];
    return nullptr;
}
