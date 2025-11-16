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

 

class Terrain : public Visual
{
  
    float *m_heightData;
    int m_heightmapWidth;
    int m_heightmapHeight;
    Vec3 m_scale;
    Material *material;
    std::vector<MeshBuffer *> m_blocks;

    friend class TerrainRenderer;

    void FilterHeightMap();

public:
    Terrain(const std::string& name = "Terrain");
    ~Terrain();
    
    bool LoadFromHeightmap(const std::string& heightmapPath,
                          float scaleX, float scaleY, float scaleZ,
                          float texScaleU = 1.0f, float texScaleV = 1.0f);
    
    void Build();
    
    MeshBuffer *AddBuffer();

    MeshBuffer* GetBlock(int x, int z) const;
    MeshBuffer* GetBlock(u32 index) const;

    u32 GetBlockCount() const { return m_blocks.size(); }

    void Render() override;

    void Debug(RenderBatch *batch) override;

    void ApplyMaterial();

    Material *GetMaterial() const { return material; }

 

private:
    bool GenerateBlock(MeshBuffer* block, int blockX, int blockZ, float texScaleU, float texScaleV);
    float GetHeight(int x, int z) const;
    Vec3 CalculateNormal(int x, int z) const;
};
