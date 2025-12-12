#include "pch.h"
#include "Vertex.hpp"
#include "Driver.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "Stream.hpp"
#include "Batch.hpp"
#include "Pixmap.hpp"
#include "glad/glad.h"
#include "Utils.hpp"
#include "Map.hpp"

DetailMeshBuffer::DetailMeshBuffer(const std::string &name)
    : Spatial(name), m_material(0), m_vdirty(true), m_idirty(true), m_DynamicVertexBuffer(false), m_DynamicIndexBuffer(false)
{
    buffer = new VertexArray();
    vb = nullptr;
    ib = nullptr;
    m_boundBox.clear();
}

DetailMeshBuffer::~DetailMeshBuffer()
{
    delete buffer;
}

u32 DetailMeshBuffer::AddVertex(const DetailVertex &v)
{
    m_boundBox.expand(v.x, v.y, v.z);
    vertices.push_back(v);
    m_vdirty = true;
    return vertices.size() - 1;
}

u32 DetailMeshBuffer::AddVertex(float x, float y, float z,

                                float u, float v,
                                float lu, float lv)
{
    return AddVertex(DetailVertex(x, y, z, u, v, lu, lv));
}

u32 DetailMeshBuffer::AddIndex(u32 index)
{
    indices.push_back(index);
    m_idirty = true;
    return indices.size() - 1;
}

u32 DetailMeshBuffer::AddFace(u32 i0, u32 i1, u32 i2)
{
    indices.push_back(i0);
    indices.push_back(i1);
    indices.push_back(i2);
    m_idirty = true;
    return indices.size() - 3;
}

void DetailMeshBuffer::Clear()
{
    vertices.clear();
    indices.clear();
    m_vdirty = true;
    m_idirty = true;
}

void DetailMeshBuffer::Build()
{
    if (vertices.empty() || indices.empty())
    {
        LogWarning("[DetailMeshBuffer] Empty buffer");
        return;
    }

    // Cria VBO
    if (!vb)
    {
        vb = buffer->AddVertexBuffer(sizeof(DetailVertex), vertices.size(), m_DynamicVertexBuffer);

        auto *decl = buffer->GetVertexDeclaration();

        // 0: position (3 floats)
        decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);

        // 1: texcoord0 (2 floats) - textura
        decl->AddElement(0, 3 * sizeof(float), VET_FLOAT2, VES_TEXCOORD);

        // 2: texcoord1 (2 floats) - lightmap
        decl->AddElement(0, 5 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    }

    // Cria IBO
    if (!ib)
    {
        ib = buffer->CreateIndexBuffer(indices.size(), m_DynamicIndexBuffer, false);
    }

    // Atualiza dados
    if (m_vdirty)
    {
        if (m_DynamicVertexBuffer)
        {
            vb->SetSubData(0, vertices.size(), vertices.data());
        }
        else
        {
            vb->SetData(vertices.data());
        }
    }

    if (m_idirty)
    {
        if (m_DynamicIndexBuffer)
        {
            ib->SetSubData(0, indices.size(), indices.data());
        }
        else
        {
            ib->SetData(indices.data());
        }
    }

    buffer->Build();

    m_vdirty = false;
    m_idirty = false;
}

void DetailMeshBuffer::Render()
{
    if (m_idirty || m_vdirty)
    {
        Build();
    }
    
    
    Driver::Instance().DrawVertexArray(buffer, vertices.size(), indices.size(), PrimitiveType::PT_TRIANGLES);
    //Driver::Instance().DrawVertexArray(buffer, vertices.size(), indices.size(), PrimitiveType::PT_TRIANGLES);

}

void DetailMeshBuffer::Render(PrimitiveType type, u32 count)
{
    if (m_idirty || m_vdirty)
    {
        Build();
    }
    buffer->Render(type, count);
}

void DetailMeshBuffer::Debug(RenderBatch *batch)
{

    // tris lines

     batch->SetColor(255, 0, 255);
     batch->Box(m_boundBox);
    // for (size_t i = 0; i < indices.size(); i += 3)
    // {

    //     const DetailVertex v0 = vertices[indices[i]];
    //     const DetailVertex v1 = vertices[indices[i + 1]];
    //     const DetailVertex v2 = vertices[indices[i + 2]];
    //     const Vec3 p0 = Vec3(v0.x, v0.y, v0.z);
    //     const Vec3 p1 = Vec3(v1.x, v1.y, v1.z);
    //     const Vec3 p2 = Vec3(v2.x, v2.y, v2.z);
    //     batch->TriangleLines(p0, p1, p2);
    //     // batch->Line3D(p0, p1);
    // }

    /// vertex animaltions lines
}

void DetailMeshBuffer::CalculateBoundingBox()
{
    m_boundBox.clear();
    for (const auto &v : vertices)
    {
        m_boundBox.expand(Vec3(v.x, v.y, v.z));
    }
}

MapMesh::MapMesh(const std::string &name)
    : Spatial(name)
{
}

MapMesh::~MapMesh()
{
    textures.clear();
    lightMaps.clear();
    Clear();
}

DetailMeshBuffer *MapMesh::AddBuffer(u32 material, u32 lightmap)
{
    DetailMeshBuffer *buffer = new DetailMeshBuffer("Buffer_" + std::to_string(buffers.size()));
    buffer->SetMaterial(material);
    buffer->SetLightmap(lightmap);
    buffers.push_back(buffer);
    return buffer;
}

bool MapMesh::AppendBuffer(DetailMeshBuffer *buffer)
{
    if (!buffer)
        return false;
    buffers.push_back(buffer);
    return true;
}

DetailMeshBuffer *MapMesh::DropBuffer(u32 index)
{
    if (index >= buffers.size())
        return nullptr;

    DetailMeshBuffer *buffer = buffers[index];
    buffers.erase(buffers.begin() + index);
    return buffer;
}

bool MapMesh::RemoveBuffer(u32 index)
{
    DetailMeshBuffer *buffer = DropBuffer(index);
    if (buffer)
    {
        delete buffer;
        return true;
    }
    return false;
}

DetailMeshBuffer *MapMesh::GetBuffer(size_t index) const
{
    if (index >= buffers.size())
        return nullptr;
    return buffers[index];
}

 

void MapMesh::Clear()
{
    for (auto *buffer : buffers)
    {
        delete buffer;
    }
    buffers.clear();

   
}

void MapMesh::Build()
{
    for (auto *buffer : buffers)
    {
        buffer->Build();
    }
}

void MapMesh::Render()
{

    for (u32 i = 0; i < buffers.size(); i++)
    {

        

        const int texture  = buffers[i]->GetMaterial();
        const int detail = buffers[i]->GetLightmap();

        if (texture >= 0 && texture < textures.size())
        {
            Texture *map = textures[texture];
            if (map)
            {
                map->Bind();
            }
        }

        if (detail >= 0 && detail <= lightMaps.size())
        {
            Texture *map = lightMaps[detail];
            if (map)
            {
                map->Bind(1);
            }
            
        }

        buffers[i]->Render();
    }

    
 
}

void MapMesh::CalculateBoundingBox()
{
    m_boundBox.clear();

    for (auto *buffer : buffers)
    {
        buffer->CalculateBoundingBox();
        m_boundBox.merge(buffer->getBoundingBox());
    }
}

void MapMesh::Debug(RenderBatch *batch)
{
    // // Desenha bounding box
     batch->SetColor(0, 255, 0);
     batch->Box(m_boundBox);

    // // Desenha spawn points
    // batch->SetColor(255, 0, 0);
    // for (const Vec3 &spawn : spawnPoints)
    // {
    //     // batch->DrawSphere(spawn, 16.0f);
    // }
    for (auto *buffer : buffers)
    {
        buffer->Debug(batch);
    }
}

MapMesh *MapMesh::CreateTestPlane(float width, float height,
                                  u32 subdivisionsX, u32 subdivisionsZ)
{
    MapMesh *mesh = new MapMesh("TestPlane");

 

    DetailMeshBuffer *buffer = mesh->AddBuffer(0);

    float stepX = width / subdivisionsX;
    float stepZ = height / subdivisionsZ;

    for (u32 z = 0; z <= subdivisionsZ; z++)
    {
        for (u32 x = 0; x <= subdivisionsX; x++)
        {
            // Posição
            float px = -width * 0.5f + x * stepX;
            float py = 0.0f;
            float pz = -height * 0.5f + z * stepZ;

            // UVs textura (repete 4x)
            float u = (float)x / subdivisionsX * 4.0f;
            float v = (float)z / subdivisionsZ * 4.0f;

            // UVs lightmap (0-1, sem repetir)
            float lu = (float)x / subdivisionsX;
            float lv = (float)z / subdivisionsZ;

            buffer->AddVertex(px, py, pz, u, v, lu, lv);
        }
    }

    // Gera faces (2 triângulos por quad)
    for (u32 z = 0; z < subdivisionsZ; z++)
    {
        for (u32 x = 0; x < subdivisionsX; x++)
        {
            u32 i0 = z * (subdivisionsX + 1) + x;
            u32 i1 = i0 + 1;
            u32 i2 = i0 + (subdivisionsX + 1);
            u32 i3 = i2 + 1;

            // Triângulo 1
            buffer->AddFace(i0, i2, i1);

            // Triângulo 2
            buffer->AddFace(i1, i2, i3);
        }
    }

    buffer->Build();
    mesh->CalculateBoundingBox();

    LogInfo("[TestMapMesh] Created plane: %.1fx%.1f, %dx%d subdivisions, %zu vertices, %zu triangles",
            width, height, subdivisionsX, subdivisionsZ,
            buffer->GetVertexCount(), buffer->GetIndexCount() / 3);

    return mesh;
}

bool MapMesh::Load(const std::string &filename)
{
    FileStream file;
    if (!file.Open(filename, "rb"))
    {
        LogError("Failed to open file: %s", filename.c_str());
        return false;
    }

    u32 magic = file.ReadUInt();
    u32 version = file.ReadUInt();
    std::string name = Utils::GetFileNameWithoutExt(filename.c_str());

    if (magic != 0x4D415032)
    {
        LogError("Invalid file magic: 0x%08X", magic);
        file.Close();
        return false;
    }

    if (version != 1)
    {
        LogError("Unsupported file version: %u", version);
        file.Close();
        return false;
    }

    // Materiais
    u32 textureCount = file.ReadUInt();
    for (u32 i = 0; i < textureCount; i++)
    {
        std::string texture = file.ReadUTF();
        Texture *tex = TextureManager::Instance().TryLoad(texture);
        tex->SetAnisotropy(8.0f);

        textures.push_back(tex);

       





    }



    // Lightmaps
    int lightmapCount = file.ReadUInt();
    LogInfo("Lightmaps: %d", lightmapCount);

 
    for (int i = 0; i < lightmapCount; i++)
    {
        std::string lightmapName = name + std::to_string(i) ;
       // LogInfo("Lightmap %d: %s", i, lightmapName.c_str());
        Texture *lightmap = TextureManager::Instance().TryLoad(lightmapName);
        lightMaps.push_back(lightmap);
    }
 


    // BUFFERS
    u32 bufferCount = file.ReadUInt();

    LogInfo("Buffers: %d", bufferCount);

    for (u32 i = 0; i < bufferCount; i++)
    {
        u32 textureID = file.ReadUInt();
        u32 lightmapID = file.ReadUInt();

        DetailMeshBuffer *buffer = AddBuffer(textureID, lightmapID);

        u32 vertCount = file.ReadUInt();
        buffer->vertices.resize(vertCount);
        file.Read(buffer->vertices.data(), vertCount * sizeof(DetailVertex));

        u32 idxCount = file.ReadUInt();
        buffer->indices.resize(idxCount);
        file.Read(buffer->indices.data(), idxCount * sizeof(u32));
        buffer->Build();
    }
    CalculateBoundingBox();

    file.Close();
    return true;
}
