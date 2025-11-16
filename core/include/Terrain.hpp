#pragma once
#include "Config.hpp"
#include "Object.hpp"
#include "LoadTypes.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class VertexBuffer;
class IndexBuffer;
class VertexArray;
class Visual;
class BoundingBox;
class Terrain;
class MeshBuffer;

struct TerrainVertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

class TerrainBlock
{
    VertexArray *buffer;
    VertexBuffer *vb;
    IndexBuffer *ib;
    BoundingBox m_boundBox;
    std::vector<TerrainVertex> m_vertices;
    std::vector<u32> m_indices;
    BoundingBox m_bounds;
    bool m_vdirty;
    bool m_idirty;
    friend class Terrain;

public:
    TerrainBlock(); 
    ~TerrainBlock();

    const BoundingBox& GetBounds() const { return m_bounds; }
    BoundingBox& GetBounds() { return m_bounds; }

    void Build();
    void Render();
};

 

class Terrain : public Visual
{
    std::vector<TerrainBlock*> m_blocks;
    float *m_heightData;
    int m_heightmapWidth;
    int m_heightmapHeight;
    Vec3 m_scale;
    Material *material;
  

    void FilterHeightMap();

public:
    Terrain(const std::string& name = "Terrain");
    ~Terrain();
    
    bool LoadFromHeightmap(const std::string& heightmapPath,
                          float scaleX, float scaleY, float scaleZ,
                          float texScaleU = 1.0f, float texScaleV = 1.0f);
    
    void Build();
    
    TerrainBlock* GetBlock(int x, int z) const;
    TerrainBlock* GetBlock(u32 index) const;

    u32 GetBlockCount() const { return m_blocks.size(); }

    void Render() override;

    void Debug(RenderBatch *batch) override;

    void ApplyMaterial();

    Material *GetMaterial() const { return material; }

 

private:
    bool GenerateBlock(TerrainBlock* block, int blockX, int blockZ, float texScaleU, float texScaleV);
    float GetHeight(int x, int z) const;
    Vec3 CalculateNormal(int x, int z) const;
};
