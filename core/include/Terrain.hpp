#pragma once
#include "Config.hpp"
#include "Object.hpp"
#include "GraphicsTypes.hpp"
#include "Vertex.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class Visual;
class BoundingBox;
class Terrain;
class MeshBuffer;
class TerrainRenderer;
class Frustum;

struct TerrainRaycastHit
{
    bool hit;
    Vec3 position;
    Vec3 normal;
    float distance;
    int gridX;
    int gridZ;

    TerrainRaycastHit() : hit(false), position(Vec3::Zero), normal(Vec3(0, 1, 0)),
                          distance(0.0f), gridX(-1), gridZ(-1) {}
};

class Terrain : public Node3D
{

    float *m_heightData;
    int m_heightmapWidth;
    int m_heightmapHeight;
    Vec3 m_scale;
    Material *material;
    std::vector<MeshBuffer *> m_blocks;

    void FilterHeightMap();

public:
    Terrain(const std::string &name = "Terrain");
    ~Terrain();

    bool LoadFromHeightmap(const std::string &heightmapPath,
                           float scaleX, float scaleY, float scaleZ,
                           float texScaleU = 1.0f, float texScaleV = 1.0f);

    void Build();

    MeshBuffer *AddBuffer();

    MeshBuffer *GetBlock(int x, int z) const;
    MeshBuffer *GetBlock(u32 index) const;

    u32 GetBlockCount() const { return m_blocks.size(); }

    void render(Shader *shader, bool useMaterial) override;

    void renderDebug(RenderBatch *batch);

    void ApplyMaterial();

    Material *GetMaterial() const { return material; }

private:
    bool GenerateBlock(MeshBuffer *block, int blockX, int blockZ, float texScaleU, float texScaleV);
    float GetHeight(int x, int z) const;
    Vec3 CalculateNormal(int x, int z) const;
};

class TerrainLod : public Node3D
{
public:
private:
    struct Patch
    {
        Patch() : top(nullptr), bottom(nullptr), right(nullptr), left(nullptr),
                  currentLOD(-1) {}

        Patch *top;
        Patch *bottom;
        Patch *right;
        Patch *left;
        int currentLOD;
        BoundingBox boundBox;
        Vec3 center;
    };

    struct TerrainData
    {
        Patch *patches;
        int size; // Tamanho total do heightmap (ex: 513x513)
        Vec3 position;

        Vec3 scale;
        Vec3 center;
        int patchSize;     // Tamanho de cada patch (17, 33, etc)
        int calcPatchSize; // patchSize - 1
        int patchCount;    // Número de patches em cada direção
        int maxLOD;

        std::vector<float> lodDistanceThreshold;

        TerrainData(int pSize, int mLOD, const Vec3 &pos, const Vec3 &scl)
            : patches(nullptr), size(0), position(pos),
              scale(scl), patchSize(pSize), calcPatchSize(pSize - 1),
              patchCount(0), maxLOD(mLOD) {}
    };

    TerrainData m_terrainData;

    u32 m_verticesToRender;
    u32 m_indicesToRender;

    bool m_overrideDistanceThreshold;

    Vec3 m_oldCameraPosition;
    Vec3 m_oldCameraRotation;
    float m_cameraMovementDelta;
    float m_cameraRotationDelta;

    float m_textureScale;

    int m_smoothFactor;

    Material *material;
    MeshBuffer *meshBuffer;

public:
    TerrainLod(const std::string &name = "Terrain",
               int maxLOD = 4,
               PatchSize patchSize = PATCH_17,
               const Vec3 &position = Vec3(0, 0, 0),

               const Vec3 &scale = Vec3(1, 1, 1));

    ~TerrainLod();

    bool LoadHeightMap(const std::string &filename,
                       float heightScale = 1.0f,
                       int smoothFactor = 0);

    void SetPosition(const Vec3 &pos);
    void SetRotation(const Vec3 &rot);
    void SetScale(const Vec3 &scl);
    void SetRotationPivot(const Vec3 &pivot);

    Vec3 GetScale() const { return m_terrainData.scale; }
    Vec3 GetCenter() const { return m_terrainData.center; }

    void SetCameraMovementDelta(float delta) { m_cameraMovementDelta = delta; }
    void SetCameraRotationDelta(float delta) { m_cameraRotationDelta = delta; }
    bool OverrideLODDistance(int lod, float newDistance);
    void SetLODOfPatch(int patchX, int patchZ, int lod);
    int GetCurrentLODOfPatches(std::vector<int> &lods) const;

    void ScaleTexture(float scale1 = 1.0f, float scale2 = 0.0f);

    BoundingBox GetBoundingBox(int patchX, int patchZ) const;

    void update(float dt) override;
    void render(Shader *shader, bool useMaterial) override;
    void debug(RenderBatch *batch);

    Material *GetMaterial() const { return material; }

    float GetHeight(float worldX, float worldZ) const;
    float GetHeight(const Vec3 &worldPos) const;

    void SetHeight(float worldX, float worldZ, float newHeight, float radius);
    void ModifyHeight(float worldX, float worldZ, float deltaHeight, float radius);
    void Flatten(float worldX, float worldZ, float targetHeight, float radius, float strength = 0.5f);
    void Smooth(int smoothFactor);
    void SmoothArea(float worldX, float worldZ, float radius, int iterations = 1);

    TerrainRaycastHit Raycast(const Ray &ray, float maxDistance = 1000.0f) const;
    TerrainRaycastHit RaycastFromCamera(float maxDistance = 1000.0f) const;
    TerrainRaycastHit RaycastFromMouse(int mouseX, int mouseY,
                                       int screenWidth, int screenHeight,
                                       float maxDistance = 1000.0f) const;

    bool IsPositionInBounds(float worldX, float worldZ) const;
    Vec3 GetNormalAt(float worldX, float worldZ) const;

private:
    void ApplyTransformation();
    bool PreRenderLODCalculations();
    void PreRenderIndicesCalculations();
    void ApplyMaterial();

    bool ValidateTerrainData() const;

    u32 GetIndex(int patchX, int patchZ, int patchIndex, u32 vX, u32 vZ) const;

    void CalculateNormals();
    void CreatePatches();
    void CalculatePatchData();
    void CalculateDistanceThresholds(bool scaleChanged = false);
    void SetCurrentLODOfPatches(int lod);
    void SetCurrentLODOfPatches(const std::vector<int> &lodArray);

    Vec3 getVertex(u32 index) const;
    Vec3 getVertex(u32 index);
};

class TiledTerrain : public Node3D
{
private:
    u32 m_mapWidth;
    u32 m_mapHeight;
    u8 *m_tileMap;

    MeshBuffer *m_patchBuffer;

    int m_tilesInTextureSide; // Atlas size (ex: 8 = 8x8 tiles)
    float m_patchSideLength;  // World units per patch
    int m_tilesInPatchSide;   // Tiles per patch side
    u8 m_defaultTile;         // Tile ID for out-of-bounds
    u8 m_border;
    Material *material;

public:
    TiledTerrain(int tilesInTextureSide,
                 float patchLength, int tilesPerPatch,
                 u8 defaultTile = 0, u8 border = 0, const std::string &name = "TiledTerrain");
    ~TiledTerrain();

    void SetBorder(int borderPatches) { m_border = borderPatches; }
    int GetBorder() const { return m_border; }

    void LoadTilemap(Pixmap *tileMapImage);
    void LoadTilemap(u32 width, u32 height, const u8 *data);
    void SetTileAt(u32 x, u32 z, u8 tile);
    u8 GetTileAt(u32 x, u32 z) const;

    u32 GetMapWidth() const { return m_mapWidth; }
    u32 GetMapHeight() const { return m_mapHeight; }
    float GetPatchSize() const { return m_patchSideLength; }

    Material *GetMaterial() const { return material; }

    void render(Shader *shader, bool useMaterial) override;

private:
    void ApplyMaterial();

    void RenderPatch(int patchX, int patchZ);
    bool IsPatchVisible(int patchX, int patchZ, const Frustum *frustum) const;

    void Initialize(int tilesInTextureSide,
                    float patchLength, int tilesPerPatch,
                    u8 defaultTile = 0);

    u8 GetTileAtWrapped(int x, int z) const;
    void GetTileUVs(u8 tile, float stepUV, float uvs[4][2]) const;
};

class PatchInfiniteTerrain : public Node3D
{
public:
    PatchInfiniteTerrain(const std::string& name = "PatchInfiniteTerrain");
    ~PatchInfiniteTerrain();

    bool LoadHeightmap(const std::string& filename, float patchSize,
                       float heightScale, int numPatches = 16);
    void render(Shader* shader, bool useMaterial = true) override;
    Material* GetMaterial() { return m_material; }
    float GetHeightAt(float worldX, float worldZ) const;

private:
    // Base heightmap data (tiles seamlessly)
    Pixmap* m_heightmap;
    u8* m_heights;           // Cache local do heightmap

    float m_length;          // Tamanho total do heightmap (ex: 1024)
    float m_height;          // Escala vertical (ex: 20)
    int m_grids;             // Resolução do heightmap (ex: 256)
    int m_patches;           // Número de patches (ex: 16)
    float m_gridLen;         // Tamanho de cada grid cell
    float m_invLen;          // 1/length para cálculos rápidos

    // Pre-compiled patch meshes (reutilizáveis)
    struct PatchMesh {
        MeshBuffer* mesh;
        int refX, refZ;      // Posição de referência (0 a patches-1)
    };
    std::vector<PatchMesh> m_patchMeshes;

    Material* m_material;

    // Helpers
    void BuildAllPatches();
    void BuildPatchMesh(MeshBuffer* mesh, int refPatchX, int refPatchZ);
    float SampleHeight(int gridX, int gridZ) const;
    bool IsClipped(const Camera* cam, const Frustum *frustum,float pX, float pY, float pZ, float size) const;
};


class InfiniteTerrain : public Node3D
{
public:
    InfiniteTerrain(const std::string &name = "InfiniteTerrain");
    ~InfiniteTerrain();

    // Setup
    bool LoadBaseHeightmap(const std::string &filename, float heightScale);
    void SetPatchConfig(int patchesPerSide, int verticesPerPatch, float patchSize);

    // Rendering
    void render(Shader *shader, bool useMaterial = true) override;
    void debug(RenderBatch *batch);

    // Queries
    float GetHeightAt(float worldX, float worldZ) const;
    Vec3 GetNormalAt(float worldX, float worldZ) const;

    // Material
    Material *GetMaterial() { return m_material; }
    void SetMaterial(Material *mat) { m_material = mat; }

private:
    static constexpr int MAX_CACHED_PATCHES = 200;
    static constexpr int MAX_LOD_LEVELS = 4;

    // === BASE DATA (repeats infinitely) ===
    struct BaseData
    {
        float *heights;
        u32 width;
        u32 height;
        float heightScale;

        BaseData() : heights(nullptr), width(0), height(0), heightScale(100.0f) {}
    } m_baseData;

    // === PATCH CONFIG ===
    int m_patchesPerSide;
    int m_verticesPerPatch;
    float m_patchWorldSize;

    // === PATCH STORAGE ===
    struct PatchMesh
    {
        MeshBuffer *meshes[MAX_LOD_LEVELS];
        BoundingBox bounds;
        u32 lastAccessFrame;
        int worldX, worldZ; // FIXED: Store world position

        PatchMesh() : lastAccessFrame(0), worldX(0), worldZ(0)
        {
            for (int i = 0; i < MAX_LOD_LEVELS; i++)
                meshes[i] = nullptr;

            bounds.min = Vec3(0, 0, 0);
            bounds.max = Vec3(0, 0, 0);
        }

        ~PatchMesh()
        {
            for (int i = 0; i < MAX_LOD_LEVELS; i++)
                if (meshes[i])
                    delete meshes[i];
        }
    };

    // FIXED: Use world coords as key instead of modulo ID
    std::unordered_map<long long, PatchMesh *> m_patchCache;

    // === MATERIAL ===
    Material *m_material;
    u32 m_currentFrame;

    // === HELPERS ===
    long long GetPatchKey(int patchX, int patchZ) const;
    PatchMesh *GetOrCreatePatch(int patchX, int patchZ, int lod);
    void BuildPatchMeshLOD(int patchX, int patchZ, int lod, PatchMesh *patch);
    bool IsPatchVisible(int patchX, int patchZ, const Frustum *frustum) const;
    void CleanOldPatches();
    int CalculateLODWithHysteresis(float distSq, int patchX, int patchZ) const;

    float SampleHeightFromBase(float u, float v) const;
    Vec3 CalculateNormal(float u, float v) const;
    int CalculateLODWithNeighbors(float distSq, int patchX, int patchZ) const;
};