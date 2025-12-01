#include "pch.h"
#include "Vertex.hpp"
#include "Driver.hpp"
#include "Mesh.hpp"
#include "Utils.hpp"
#include "Texture.hpp"
#include "Stream.hpp"
#include "md3.hpp"

MeshM3D::MeshM3D(const std::string &name)
{
}

MeshM3D::~MeshM3D()
{
    for (int i = 0; i < surfaces.size(); i++)
    {
        surfaces[i].release();
    }
    surfaces.clear();
}

static Vec3 DecodeNormal(unsigned char lat, unsigned char lng)
{
    // Converte bytes para ângulos
    float latAngle = (lat * 2.0f * M_PI) / 255.0f;
    float lngAngle = (lng * 2.0f * M_PI) / 255.0f;

    // Coordenadas esféricas para cartesianas
    Vec3 normal;
    normal.x = std::cos(latAngle) * std::sin(lngAngle);
    normal.y = std::sin(latAngle) * std::sin(lngAngle);
    normal.z = std::cos(lngAngle);

    return normal.normalized();
}

void VectorNormalize(Vec3& v)
{
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length > 0.0f)
    {
        float ilength = 1.0f / length;
        v.x *= ilength;
        v.y *= ilength;
        v.z *= ilength;
    }
}


bool MeshM3D::Load(const std::string &filename, float scale)
{
    

    tMd3Header m_Header;

    FileStream file;

    if (file.Open(filename, "rb"))
    {

        LogInfo("Loading MD3 file: %s", filename.c_str());

        file.Read(&m_Header, sizeof(tMd3Header));

        numFrames = m_Header.numFrames;

        char *ID = m_Header.fileID;

        if ((ID[0] != 'I' || ID[1] != 'D' || ID[2] != 'P' || ID[3] != '3') || m_Header.version != 15)
        {
            LogError("Bad MD3 file: %s", filename.c_str());
            return false;
        }

        int i = 0;
        tMd3Bone *pBones = new tMd3Bone[m_Header.numFrames];
        file.Read(&pBones[0], sizeof(tMd3Bone) * m_Header.numFrames);
        //LogInfo("Scale %f creator  %s", pBones[0].scale, pBones[0].creator);

        m_scale =  scale / pBones[0].scale;

        numOfTags = m_Header.numTags;

        for (i = 0; i < m_Header.numTags; i++)
        {
            MeshM3D::Bone bone;
            bones.push_back(bone);
        }

        for (i = 0; i < m_Header.numFrames * m_Header.numTags; i++)
        {
            char strName[64];
            Tag &tag = AddTag();

            file.Read(&strName[0], 64);
            tag.name = strName;

            tag.position.x = file.ReadFloat() ;
            tag.position.y = file.ReadFloat() ;
            tag.position.z = file.ReadFloat() ;
            tag.axes[0].x = file.ReadFloat();
            tag.axes[0].y = file.ReadFloat();
            tag.axes[0].z = file.ReadFloat();

            tag.axes[1].x = file.ReadFloat();
            tag.axes[1].y = file.ReadFloat();
            tag.axes[1].z = file.ReadFloat();

            tag.axes[2].x = file.ReadFloat();
            tag.axes[2].y = file.ReadFloat();
            tag.axes[2].z = file.ReadFloat();
        }
        for (i = 0; i < m_Header.numTags; i++)
        {

            LogInfo("Tag: %s (%d)", tags[i].name.c_str(), i);
        }
        delete[] pBones;

        long meshOffset = file.Tell();

        tMd3MeshInfo meshHeader;

        for (i = 0; i < m_Header.numMeshes; i++)
        {

            MeshM3D::Surface &surface = AddSurface();

            file.Seek(meshOffset, SeekOrigin::Begin);
            file.Read(&meshHeader, sizeof(tMd3MeshInfo));

            int numOfVerts = meshHeader.numVertices;
            int numOfTris = meshHeader.numTriangles;
            int numOfSkins = meshHeader.numSkins;
            int numOfFrames = meshHeader.numMeshFrames;
            int numOfFaces = meshHeader.numTriangles;
            surface.numFrames = numOfFrames;

            LogInfo("Name: %s", meshHeader.strName);
            LogInfo("Num of verts: %d", numOfVerts);
            LogInfo("Num of tris: %d", numOfTris);
            LogInfo("Num of skins: %d", numOfSkins);
            LogInfo("Num of frames: %d", numOfFrames);
            LogInfo("Num of faces: %d", numOfFaces);

            surface.name = meshHeader.strName;
            surface.numTriangles = meshHeader.numTriangles;

            tMd3Skin *m_pSkins = new tMd3Skin[meshHeader.numSkins];

            file.Read(&m_pSkins[0], sizeof(tMd3Skin) * meshHeader.numSkins);
            for (int i = 0; i < meshHeader.numSkins; i++)
            {
                LogInfo("Skin %d: %s", i, m_pSkins[i].strName);
                Material *material = AddMaterial(m_pSkins[i].strName);
                material->SetTexture(0, TextureManager::Instance().Get("checker"));
            }
            delete[] m_pSkins;

            file.Seek(meshOffset + meshHeader.triStart, SeekOrigin::Begin);

            for (int i = 0; i < meshHeader.numTriangles; i++)
            {
                int v1 = file.ReadInt();
                int v2 = file.ReadInt();
                int v3 = file.ReadInt();
                surface.indices.push_back(v3);
                surface.indices.push_back(v2);
                surface.indices.push_back(v1);
                //   LogInfo("Tri: %d %d %d", v1, v2, v3);
            }

            file.Seek(meshOffset + meshHeader.vertexStart, SeekOrigin::Begin);

            for (int i = 0; i < meshHeader.numMeshFrames * meshHeader.numVertices; i++)
            {
                float x = static_cast<float>(file.ReadShort()) / 64.0f;
                float y = static_cast<float>(file.ReadShort()) / 64.0f;
                float z = static_cast<float>(file.ReadShort()) / 64.0f;
                unsigned char lat = file.ReadByte(); // Latitude (0-255)
                unsigned char lng = file.ReadByte(); // Longitude (0-255)

                Vec3 normal = DecodeNormal(lat, lng);
                surface.normals.push_back(normal);
                Vec3 position = Vec3(x , y , z );
                surface.positions.push_back(position);
                m_boundBox.expand(position);
            }

            //  surface.bounds.min = surface.vertices[0];
            //  surface.bounds.max = surface.vertices[0];
            //   bounds.min = {FLT_MAX, FLT_MAX, FLT_MAX};
            //   bounds.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

            file.Seek(meshOffset + meshHeader.uvStart, SeekOrigin::Begin);
            for (int i = 0; i < meshHeader.numVertices; i++)
            {
                float u = static_cast<float>(file.ReadFloat());
                float v = static_cast<float>(file.ReadFloat());

                Vec3 pos = surface.positions[i];
                Vec3 normal = surface.normals[i];
                Vertex vertex;
                vertex.u = u;
                vertex.v = v;
                vertex.x = pos.x;
                vertex.y = pos.y;
                vertex.z = pos.z;
                vertex.nx = normal.x;
                vertex.ny = normal.y;
                vertex.nz = normal.z;
                // LogInfo("Vertex %d: %f %f %f", i, vertex.x, vertex.y, vertex.z);

                surface.vertices.push_back(vertex);
            }
 
            surface.init();

            LogInfo("Surface %s: total positions = %d (frames=%d, verts/frame=%d)",
                    surface.name.c_str(),
                    surface.positions.size(),
                    surface.numFrames,
                    surface.positions.size() / surface.numFrames);

            meshOffset += meshHeader.meshSize;
        }

        return true;
    }
    return false;
}

void MeshM3D::Render()
{

    for (u32 i = 0; i < surfaces.size(); i++)
    {
        if (surfaces[i].material >= 0)
        {
            Material *material = materials[surfaces[i].material];
            if (material == nullptr)
            {
                LogWarning("[MeshM3D] Invalid material index: %d", surfaces[i].material);
                continue;
            }
            const u8 layer = material->GetLayers();
            for (u8 i = 0; i < layer; i++)
            {
                const Texture *texture = material->GetTexture(i);
                if (texture)
                {

                    texture->Bind(i);
                }
            }
        }
        else
        {
            LogWarning("[MeshM3D] Invalid material index: %d", surfaces[i].material);
        }
        surfaces[i].render();
    }
}

void MeshM3D::Debug(RenderBatch *batch)
{

    for (u32 i = 0; i < surfaces.size(); i++)
    {
        surfaces[i].debug(batch);
    }
    batch->SetColor(255, 255, 0, 255);
    // for (u32 i = 0; i < bones.size(); i++)
    // {

    //        batch->Cube(bones[i]., m_scale,m_scale,m_scale );
    // }
    // for (u32 i = 0; i < tags.size(); i++)
    // {
    //       batch->Cube(tags[i].position,  m_scale,m_scale,m_scale );

    // }
}

void MeshM3D::SetSurfaceMaterial(u32 index, u32 material)
{
    DEBUG_BREAK_IF(index >= surfaces.size());
    DEBUG_BREAK_IF(material >= materials.size());
    surfaces[index].material = material;
}

void MeshM3D::SetFrame(int currentFrame, int nextFrame, float pol)
{
    if (surfaces.empty())
    {
        LogWarning("[MeshM3D] No surfaces");
        return;
    }
    int totalFrames = numFrames;
    if (currentFrame >= totalFrames)
    {
        currentFrame = totalFrames - 1;
    }
    if (nextFrame >= totalFrames)
    {
        nextFrame = totalFrames - 1;
    }

    m_boundBox.min = {FLT_MAX, FLT_MAX, FLT_MAX};
    m_boundBox.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    UpdateTags(currentFrame, nextFrame, pol);
    for (u32 i = 0; i < surfaces.size(); i++)
    {

        const MeshM3D::Surface &surface = surfaces[i];
        int numVertices = surface.vertices.size();
        if (surfaces[i].numFrames > 1 && numVertices > 0)
        {

            int currentOffsetVertex = currentFrame * numVertices;
            int nextOffsetVertex = nextFrame * numVertices;
            for (int j = 0; j < numVertices; j++)
            {
                Vec3 pos = Vec3::Lerp(surface.positions[currentOffsetVertex + j], surface.positions[nextOffsetVertex + j], pol);
                Vec3 normal = Vec3::Lerp(
                                  surface.normals[currentOffsetVertex + j],
                                  surface.normals[nextOffsetVertex + j],
                                  pol)
                                  .normalized();

                surfaces[i].vertices[j].x = pos.x;
                surfaces[i].vertices[j].y = pos.y;
                surfaces[i].vertices[j].z = pos.z;
                m_boundBox.expand(pos);

                surfaces[i].vertices[j].nx = normal.x;
                surfaces[i].vertices[j].ny = normal.y;
                surfaces[i].vertices[j].nz = normal.z;
            }
            //  LogInfo("Surface %s: vertices = %d (frames=%d)", surface.name.c_str(), surface.vertices.size(),surfaces[i].numFrames);
            surfaces[i].update();
        }
    }
}

void MeshM3D::UpdateTags(int currentFrame, int nextFrame, float pol)
{
    int currentOffset = currentFrame * numOfTags;
    int nextOffset = nextFrame * numOfTags;
    for (u32 i = 0; i < numOfTags; i++)
    {
       
        const Tag& start = tags[currentOffset + i];
        const Tag& end   = tags[nextOffset + i];
        
        float frontLerp = pol;
        float backLerp = 1.0f -pol;
    
        Tag tag; 
  
        tag.position.x = start.position.x * backLerp + end.position.x * frontLerp;
        tag.position.y = start.position.y * backLerp + end.position.y * frontLerp;
        tag.position.z = start.position.z * backLerp + end.position.z * frontLerp;
        // Eixo X (coluna 0)
        tag.axes[0].x = start.axes[0].x * backLerp + end.axes[0].x * frontLerp;
        tag.axes[0].y = start.axes[0].y * backLerp + end.axes[0].y * frontLerp;
        tag.axes[0].z = start.axes[0].z * backLerp + end.axes[0].z * frontLerp;

        // Eixo Y (coluna 1)
        tag.axes[1].x = start.axes[1].x * backLerp + end.axes[1].x * frontLerp;
        tag.axes[1].y = start.axes[1].y * backLerp + end.axes[1].y * frontLerp;
        tag.axes[1].z = start.axes[1].z * backLerp + end.axes[1].z * frontLerp;

        // Eixo Z (coluna 2)
        tag.axes[2].x = start.axes[2].x * backLerp + end.axes[2].x * frontLerp;
        tag.axes[2].y = start.axes[2].y * backLerp + end.axes[2].y * frontLerp;
        tag.axes[2].z = start.axes[2].z * backLerp + end.axes[2].z * frontLerp;

 
        VectorNormalize(tag.axes[0]);
        VectorNormalize(tag.axes[1]);
        VectorNormalize(tag.axes[2]);

        bones[i].transform.identity();
        // Coluna 0 (axis X)
        bones[i].transform.m[0] = tag.axes[0].x;
        bones[i].transform.m[1] = tag.axes[0].y;
        bones[i].transform.m[2] = tag.axes[0].z;
        
        // Coluna 1 (axis Y)
        bones[i].transform.m[4] = tag.axes[1].x;
        bones[i].transform.m[5] = tag.axes[1].y;
        bones[i].transform.m[6] = tag.axes[1].z;
        
        // Coluna 2 (axis Z)
        bones[i].transform.m[8] = tag.axes[2].x;
        bones[i].transform.m[9] = tag.axes[2].y;
        bones[i].transform.m[10] = tag.axes[2].z;
        
        // Coluna 3 (posição)
        bones[i].transform.m[12] = tag.position.x;
        bones[i].transform.m[13] = tag.position.y;
        bones[i].transform.m[14] = tag.position.z;

    }
}

const MeshM3D::Bone *MeshM3D::GetBone(u32 index) const
{
    DEBUG_BREAK_IF(index >= bones.size());
    return &bones[index];
}

MeshM3D::Surface &MeshM3D::AddSurface()
{
    MeshM3D::Surface surface;
    surfaces.push_back(surface);
    return surfaces.back();
}

MeshM3D::Tag &MeshM3D::AddTag()
{
    Tag tag;
    tags.push_back(std::move(tag));
    return tags.back();
}

MeshM3D::Surface::Surface()
{
    buffer = new VertexArray();
    numFrames = 0;
    numTriangles = 0;
    name = "";
    vb = nullptr;
    ib = nullptr;
    material = 0;
}

void MeshM3D::Surface::init()
{
    vb = buffer->AddVertexBuffer(sizeof(Vertex), vertices.size(), true);
    auto *decl = buffer->GetVertexDeclaration();
    decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
    decl->AddElement(0, 3 * sizeof(float), VET_FLOAT3, VES_NORMAL);
    decl->AddElement(0, 6 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    vb->SetData(vertices.data());
    ib = buffer->CreateIndexBuffer(indices.size(), false, true);
    ib->SetData(indices.data());
    buffer->Build();
}

void MeshM3D::Surface::update()
{
    vb->SetSubData(0, vertices.size(), vertices.data());
}

void MeshM3D::Surface::release()
{
    delete buffer;
}

void MeshM3D::Surface::render()
{
    Driver::Instance().DrawVertexArray(buffer, vertices.size(), indices.size(), PrimitiveType::PT_TRIANGLES);
}

void MeshM3D::Surface::debug(RenderBatch *batch)
{
    batch->SetColor(255, 0, 0, 255);
    for (u32 j = 0; j < indices.size() / 3; j++)
    {
        int i0 = indices[j * 3 + 0];
        int i1 = indices[j * 3 + 1];
        int i2 = indices[j * 3 + 2];

        batch->Line3D(vertices[i0].x, vertices[i0].y, vertices[i0].z,
                      vertices[i1].x, vertices[i1].y, vertices[i1].z);
        batch->Line3D(vertices[i1].x, vertices[i1].y, vertices[i1].z,
                      vertices[i2].x, vertices[i2].y, vertices[i2].z);
        batch->Line3D(vertices[i2].x, vertices[i2].y, vertices[i2].z,
                      vertices[i0].x, vertices[i0].y, vertices[i0].z);
    }
    float normalLength = 0.01f;
    batch->SetColor(0, 255, 0, 255);
    for (u32 i = 0; i < vertices.size(); i++)
    {
        Vec3 pos(vertices[i].x, vertices[i].y, vertices[i].z);
        Vec3 normal(vertices[i].nx, vertices[i].ny, vertices[i].nz);
        Vec3 end = pos + normal * normalLength;

        // LogInfo("pos %f %f %f", pos.x, pos.y, pos.z);
        // LogInfo("normal %f %f %f", normal.x, normal.y, normal.z);
        // LogInfo("end %f %f %f", end.x, end.y, end.z);

        batch->Line3D(pos.x, pos.y, pos.z,
                      end.x, end.y, end.z);
    }
}


