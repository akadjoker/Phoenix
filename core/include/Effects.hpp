#pragma once
#include "Config.hpp"
#include "GraphicsTypes.hpp"
#include "Math.hpp"
#include "Texture.hpp"
#include <string>
#include <vector>

class RenderBatch;
class Node3D;

struct EffectVertex
{
    Vec3 position;
    Vec2 texCoord;
    Vec4 color;
};

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

class TrailRenderer
{
private:
    enum class OrientationMode
    {
        FaceCamera,
        FixedNormal
    };

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

    TrailRenderer::OrientationMode orientationMode;
    Vec3 fixedNormal;

public:
    TrailRenderer(Texture *tex, int maxPts = 50, float life = 1.0f);
    ~TrailRenderer();

    void SetWidth(float start, float end);
    void SetColor(const Vec3 &start, const Vec3 &end);
    void SetLifetime(float life);
    void SetMinDistance(float dist);

    void AddPoint(const Vec3 &position, float currentTime);
    void Update(float currentTime);
    void Render(Camera *camera);
    void Clear();

    void SetOrientation(bool faceCamera);
    void SetFixedNormal(const Vec3 &normal);

    int GetPointCount() const { return points.size(); }
};

class RibbonTrail
{
private:
    const int MAX_CHAINS = 4;

    struct TrailElement
    {
        Vec3 position;
        Quat orientation;
        Vec3 color;
        float width;
        float alpha;
        float timeCreated;
    };

    struct Chain
    {
        std::vector<TrailElement> elements;
        Node3D *node;           // Segue um node (automático)
        Vec3 *externalPosition; // Ou segue uma posição externa (manual)
        Vec3 offset;
        Vec3 localOffset; // Offset local ao node/position
        bool headVisible;
        bool tailVisible;
        bool enabled;

        void RemoveOldest();
        Vec3 GetWorldPosition() const; // Helper para obter posição atual
    };

    enum class ConnectionMode
    {
        Independent, // Chains separadas (padrão atual)
        Connected    // Chains conectadas formando volume
    };

private:
    std::vector<Chain> chains;
    VertexArray *vao;
    VertexBuffer *vb;
    IndexBuffer *ib;
    Texture *texture;

    int maxChainElements;
    float trailLength;
    float minDistance;

    float initialWidth[4];
    float widthChange[4];
    Vec3 initialColor[4];
    Vec3 colorChange[4];

    bool faceCamera;
    bool dynamic;
    ConnectionMode connectionMode;

    std::vector<EffectVertex> vertices;
    std::vector<s16> indices;

    void UpdateChain(Chain &chain, int chainIndex, float time);
    void BuildGeometryIndependent(Camera *camera);
    void BuildGeometryConnected(Camera *camera);

public:
    RibbonTrail(int maxElements = 100, int numChains = 1);
    ~RibbonTrail();

    void SetMaxChainElements(int maxElements);
    void SetTrailLength(float length);
    void SetMinDistance(float dist);
    void SetNumberOfChains(int num);
    void SetConnectionAtachment(bool connected);

    void SetInitialColor(int chainIndex, const Vec3 &color);
    void SetColorChange(int chainIndex, const Vec3 &change);
    void SetInitialWidth(int chainIndex, float width);
    void SetWidthChange(int chainIndex, float change);

    void SetFaceCamera(bool face);
    void SetTexture(Texture *tex);

    void AddNode(Node3D *node, int chainIndex = 0);
    void RemoveNode(Node3D *node);

    void SetChainPosition(int chainIndex, Vec3 *position);
    void SetChainOffset(int chainIndex, const Vec3 &offset);
    void SetChainEnabled(int chainIndex, bool enabled);

    void ClearChain(int chainIndex);
    void ClearAllChains();

    void Update(float time, Camera *camera);
    void Render();

    // Getters
    int GetChainCount() const;
    int GetMaxChainElements() const;
    float GetTrailLength() const;
    ConnectionMode GetConnectionMode() const;
};

class BillboardSet
{
public:
    enum class BillboardType
    {
        Point,       // 1 quad virado para câmera (standard billboard)
        Oriented,    // 1 quad com orientação custom
        AxisAligned, // 1 quad alinhado a um eixo (Y por padrão)
        Cross,       // 2 quads cruzados (X shape)
        CrossY       // 2 quads cruzados alinhados ao Y
    };

    struct Billboard
    {
        Vec3 position;
        Vec3 color;
        Vec2 size;      // width, height
        float rotation; // Rotação em radianos (para Point type)
        Vec3 direction; // Direção para Oriented type
        Vec3 axis;      // Eixo para AxisAligned type
        float alpha;
        bool visible;

        Billboard()
            : position(0, 0, 0), color(1, 1, 1), size(1, 1), rotation(0.0f), direction(0, 0, 1), axis(0, 1, 0), alpha(1.0f), visible(true)
        {
        }
    };

private:
    std::vector<Billboard> billboards;
    BillboardType type;
    Texture *texture;

    VertexArray *vao;
    VertexBuffer *vb;
    IndexBuffer *ib;

    std::vector<EffectVertex> vertices;
    std::vector<s16> indices;

    int maxBillboards;
    Vec3 defaultAxis; // Eixo padrão para AxisAligned
    bool autoUpdate;  // Se true, atualiza geometria automaticamente

    void BuildGeometry(Camera *camera);
    void AddQuad(const Vec3 &center, const Vec3 &right, const Vec3 &up,
                 const Vec2 &size, const Vec4 &color);

public:
    BillboardSet(int maxBillboards = 1000, BillboardType type = BillboardType::Point);
    ~BillboardSet();

    int AddBillboard(const Vec3 &position, const Vec2 &size = Vec2(1, 1));
    void RemoveBillboard(int index);
    void Clear();

    void SetBillboardPosition(int index, const Vec3 &position);
    void SetBillboardSize(int index, const Vec2 &size);
    void SetBillboardColor(int index, const Vec3 &color);
    void SetBillboardAlpha(int index, float alpha);
    void SetBillboardRotation(int index, float rotation);
    void SetBillboardDirection(int index, const Vec3 &direction);
    void SetBillboardVisible(int index, bool visible);

    void SetBillboardType(BillboardType type);
    void SetTexture(Texture *tex);
    void SetDefaultAxis(const Vec3 &axis);
    void SetAutoUpdate(bool enabled);

    void SetAllColors(const Vec3 &color);
    void SetAllSizes(const Vec2 &size);
    void SetAllAlpha(float alpha);

    Billboard *GetBillboard(int index);
    int GetBillboardCount() const;
    BillboardType GetBillboardType() const;

    void Update(Camera *camera);
    void Render();
};

class StaticBillboardBatch
{
public:
    enum class BillboardType
    {
        Single,  // 1 quad
        Cross,   // 2 quads cruzados (X)
        TriCross // 3 quads (melhor coverage)
    };

private:
    std::vector<EffectVertex> vertices;
    std::vector<s16> indices;

    VertexArray *vao;
    VertexBuffer *vb;
    IndexBuffer *ib;

    Texture *texture;
    BillboardType type;
    Vec2 defaultSize;
    bool isBuilt;
    int billboardCount;

    void AddQuad(const Vec3 &center, const Vec3 &right, const Vec3 &up,
                 const Vec2 &size, const Vec4 &color);

public:
    StaticBillboardBatch(BillboardType type = BillboardType::Cross);
    ~StaticBillboardBatch();

    void Begin();
    void AddPoint(const Vec3 &position, const Vec3 &normal, const Vec2 &size, const Vec4 &color);
    void AddPoint(const Vec3 &position, const Vec3 &normal);
    void End();

    void SetTexture(Texture *tex);
    void SetDefaultSize(const Vec2 &size);
    void SetBillboardType(BillboardType type);

    void Render();

    int GetBillboardCount() const { return billboardCount; }
    int GetVertexCount() const { return vertices.size(); }
    bool IsBuilt() const { return isBuilt; }

    void Clear();
};

class DecalManager
{
public:
    struct Decal
    {
        Vec3 position;
        Vec3 normal;
        Vec3 tangent;
        Quat orientation;
        Vec2 size;
        Vec4 color;
        float rotation;  // Rotação em radianos
        float lifetime;  // Tempo total de vida
        float timeAlive; // Tempo já vivido
        float fadeStart; // Quando começa o fade (0-1)
        bool active;
        FloatRect clip;

        Decal()
            : position(0, 0, 0), normal(0, 1, 0), tangent(1, 0, 0), size(1, 1), color(1, 1, 1, 1), rotation(0.0f), lifetime(10.0f), timeAlive(0.0f), fadeStart(0.8f), active(true), clip(0, 0, 1, 1)

        {
        }
    };

private:
    std::vector<Decal> decals;
    Texture *texture;

    VertexArray *vao;
    VertexBuffer *vb;
    IndexBuffer *ib;

    std::vector<EffectVertex> vertices;
    std::vector<s16> indices;

    int maxDecals;
    float defaultLifetime;
    float defaultFadeStart;
    Vec2 defaultSize;

    void BuildGeometry();
    void AddDecalQuad(const Decal &decal);

public:
    DecalManager(int maxDecals = 500);
    ~DecalManager();

    int AddDecal(const Vec3 &position, const Vec3 &normal,
                 float lifetime = -1.0f); // -1   default

    int AddDecal(const Vec3 &position, const Vec3 &normal,
                 const Vec2 &size, const Vec4 &color,
                 float lifetime = -1.0f);

    int AddDecalOriented(const Vec3 &position, const Vec3 &normal,
                         float rotation, const Vec2 &size,
                         float lifetime = -1.0f);

 
    void SetDecalColor(int index, const Vec4 &color);
    void SetDecalSize(int index, const Vec2 &size);
    void SetDecalRotation(int index, float rotation);
    void SetDecalTexture(int index, int textureIndex);

    void SetDefaultLifetime(float lifetime);
    void SetDefaultFadeStart(float fadeStart); // 0-1 (ex: 0.8 = fade nos últimos 20%)
    void SetDefaultSize(const Vec2 &size);

    void SetTexture(Texture *tex);

    void RemoveDecal(int index);
    void RemoveAllDecals();
    void RemoveOldestDecal();

    void Update(float deltaTime);
    void Render();

    int GetDecalCount() const;
    int GetActiveDecalCount() const;
    Decal *GetDecal(int index);
};


 
struct FlareElement
{
    Vec3 color;
    float position;
    float size;
    FlareElement();
    FlareElement(float pos, float sz, const Vec3 &col);
};

class LensFlareSystem
{
private:
    VertexArray *m_vao;
    VertexBuffer *m_vbo;
    IndexBuffer *m_ibo;
    u32 m_sunTexture, m_flareTexture;
    Vec3 m_sunColor, m_sunDirection;
    float m_sunSize, m_flareBaseSize, m_cameraDistance;
    bool m_checkOcclusion, m_wasOccluded, m_lensFlareEnabled;
    Shader *m_shader;
    std::vector<FlareElement> m_flares;

    bool isFlareInScreen(const Vec3&, const Mat4&, const Mat4&, int, int);
    void buildSunQuad(const Mat4&, std::vector<float>&);
    void buildFlareVertexData(const Mat4&, const Vec3&, const Vec3&,
                              std::vector<float>&, const Mat4&, int, int);
    void createVertexArray();
    bool isSunOnScreenWithSize(const Vec3&, const Mat4&, const Mat4&,
                               float, int, int);
    float computeSunScreenFactor(const Vec3&, const Mat4&, const Mat4&);

public:
    LensFlareSystem(u32 sunTex, u32 flareTex, float sunSz, float cameraDistance = 1000.0f);
    ~LensFlareSystem();

    void initializeDefaultFlares(int count);
    void setSunColor(float r, float g, float b);
    void setSunColor(const Vec3 &color);
    void setSunDirection(const Vec3 &dir);
    void setLensFlareEnabled(bool enabled);
    void setCheckOcclusion(bool check);

    void checkOcclusion(const Mat4 &viewMatrix, const Mat4 &projectionMatrix,
                        int viewportWidth, int viewportHeight);
    void render(const Mat4 &viewMatrix, const Mat4 &projectionMatrix,
                const Vec3 &cameraPos, const Vec3 &cameraDir);
};
