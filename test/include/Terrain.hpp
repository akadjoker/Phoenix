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



struct TerrainRaycastHit
{
    bool hit;
    Vec3 position;
    Vec3 normal;
    float distance;
    int gridX;
    int gridZ;
    
    TerrainRaycastHit() : hit(false), position(Vec3::Zero), normal(Vec3(0,1,0)), 
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

    void render() override;

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

        TerrainData(int pSize, int mLOD, const Vec3 &pos,   const Vec3 &scl)
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
    void render() override;
    void debug(RenderBatch *batch);

 
    Material *GetMaterial() const { return material; }


      float GetHeight(float worldX, float worldZ) const;
    float GetHeight(const Vec3& worldPos) const;
    
    void SetHeight(float worldX, float worldZ, float newHeight, float radius);
    void ModifyHeight(float worldX, float worldZ, float deltaHeight, float radius);
    void Flatten(float worldX, float worldZ, float targetHeight, float radius, float strength = 0.5f);
    void Smooth(int smoothFactor); 
    void SmoothArea(float worldX, float worldZ, float radius, int iterations = 1);
    
       TerrainRaycastHit Raycast(const Ray& ray, float maxDistance = 1000.0f) const;
    TerrainRaycastHit RaycastFromCamera(float maxDistance = 1000.0f) const;
    TerrainRaycastHit RaycastFromMouse(int mouseX, int mouseY, 
                                       int screenWidth, int screenHeight,
                                       float maxDistance = 1000.0f) const;
    
    bool IsPositionInBounds(float worldX, float worldZ) const;
    Vec3 GetNormalAt(float worldX, float worldZ) const;

private:
    void ApplyTransformation();
    bool PreRenderLODCalculations( );
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