#pragma once
#include "Config.hpp"
#include "GraphicsTypes.hpp"
#include "LoadTypes.hpp"
#include "Node.hpp"
#include "Node3D.hpp"
#include "Stream.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class VertexBuffer;
class IndexBuffer;
class VertexArray;
class MeshManager;
class Stream;
class Texture;
class Spatial;
class Object;
class Driver;
class RenderBatch;
class MeshWriter;
class Animator;
class Pixmap;
class Material;
class Visual;

 class DetailMeshBuffer;
 class MapMesh;


struct DetailVertex
{
    float x, y, z; // Posição
    float nx, ny, nz;
    float u, v;   // UV textura
    float lu, lv; // UV lightmap

    DetailVertex() = default;
    DetailVertex(float px, float py, float pz,

                 float tu, float tv,
                 float ltu, float ltv)
        : x(px), y(py), z(pz)

          ,
          u(tu), v(tv), lu(ltu), lv(ltv)
    {
    }
};

class DetailMeshBuffer : public Spatial
{
public:
    std::vector<DetailVertex> vertices;
    std::vector<u32> indices;

private:
    VertexArray *buffer;
    VertexBuffer *vb;
    IndexBuffer *ib;

    u32 m_material;
    u32 m_lightmap;
    bool m_vdirty;
    bool m_idirty;
    bool m_DynamicVertexBuffer;
    bool m_DynamicIndexBuffer;
    friend class MapMesh;

public:
    DetailMeshBuffer(const std::string &name = "DetailMeshBuffer");
    virtual ~DetailMeshBuffer();

    u32 AddVertex(const DetailVertex &v);
    u32 AddVertex(float x, float y, float z,
       
                  float u, float v,
                  float lu, float lv);

    u32 AddIndex(u32 index);
    u32 AddFace(u32 i0, u32 i1, u32 i2);

    void SetMaterial(u32 material) { m_material = material; }
    void SetLightmap(u32 lightmap) { m_lightmap = lightmap; }
    u32 GetMaterial() const { return m_material; }
    u32 GetLightmap() const { return m_lightmap; }

    void Clear();
 
    void Build();
    void Render();
    void Render(PrimitiveType type, u32 count);
    void Debug(RenderBatch *batch);

    void CalculateBoundingBox() override;

    size_t GetVertexCount() const { return vertices.size(); }
    size_t GetIndexCount() const { return indices.size(); }
};

class MapMesh : public Spatial
{
private:
    std::vector<DetailMeshBuffer *> buffers;

    std::vector<Texture *> lightMaps;
    std::vector<Texture *> textures;



public:
    MapMesh(const std::string &name = "MapMesh");
    virtual ~MapMesh();

    // Adicionar buffers
    DetailMeshBuffer *AddBuffer(u32 material = 0, u32 lightmap = 0);
    bool AppendBuffer(DetailMeshBuffer *buffer);

    // Remover buffers
    DetailMeshBuffer *DropBuffer(u32 index);
    bool RemoveBuffer(u32 index);

    // Acesso
    size_t GetBufferCount() const { return buffers.size(); }
    DetailMeshBuffer *GetBuffer(size_t index) const;

 


    bool Load(const std::string &file);
    bool Save(const std::string &file);

    

    static MapMesh* CreateTestPlane(float width, float height, 
                                    u32 subdivisionsX, u32 subdivisionsZ);

    // Override de Visual
    void Clear() ;
    void Build() ;
    void Render() ;
    void CalculateBoundingBox() override;
    void Debug(RenderBatch *batch) ;
};



  