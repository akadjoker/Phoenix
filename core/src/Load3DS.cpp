#include "pch.h"
#include "Vertex.hpp"
#include "Mesh.hpp"
#include "Stream.hpp"
#include "Utils.hpp"

std::vector<std::string> Loader3DS::GetExtensions() const
{
    return {"3ds"};
}

bool Loader3DS::Load(Stream *stream, Mesh *mesh)
{
    if (!stream || !stream->IsOpen())
    {
        LogError("[3DSLoader] Invalid stream");
        return false;
    }

    m_stream = stream;
    m_materials.clear();
    m_objects.clear();

    // Formato 3DS é little-endian
    m_stream->SetBigEndian(false);

    // Lê o chunk principal
    Chunk mainChunk = ReadChunk();

    if (mainChunk.id != MAIN3DS)
    {
        LogError("[3DSLoader] Not a valid 3DS file (magic: 0x%04X)", mainChunk.id);
        return false;
    }

    ProcessMainChunk();
 
    BuildMesh(mesh);

    LogInfo("[3DSLoader] Loaded: %zu objects, %zu materials",
            m_objects.size(), m_materials.size());

    return true;
}

Loader3DS::Chunk Loader3DS::ReadChunk()
{
    Chunk chunk;
    chunk.id = m_stream->ReadUShort();
    chunk.length = m_stream->ReadUInt();
    chunk.bytesRead = 6; // ID (2) + Length (4)
    return chunk;
}

void Loader3DS::SkipChunk(const Chunk &chunk)
{
    if (chunk.bytesRead < chunk.length)
    {
        m_stream->Seek(chunk.length - chunk.bytesRead, SeekOrigin::Current);
    }
}

std::string Loader3DS::ReadString()
{
    return m_stream->ReadCString();
}

void Loader3DS::ReadColorChunk(float *color)
{
    Chunk chunk = ReadChunk();

    switch (chunk.id)
    {
    case COLOR_F:
    case LIN_COLOR_F:
        color[0] = m_stream->ReadFloat();
        color[1] = m_stream->ReadFloat();
        color[2] = m_stream->ReadFloat();
        break;

    case COLOR_24:
    case LIN_COLOR_24:
        color[0] = m_stream->ReadByte() / 255.0f;
        color[1] = m_stream->ReadByte() / 255.0f;
        color[2] = m_stream->ReadByte() / 255.0f;
        break;
    }
}

float Loader3DS::ReadPercentage()
{
    Chunk chunk = ReadChunk();

    switch (chunk.id)
    {
    case INT_PERCENTAGE:
        return m_stream->ReadShort() / 100.0f;
    case FLOAT_PERCENTAGE:
        return m_stream->ReadFloat();
    }

    return 0.0f;
}

void Loader3DS::ProcessMainChunk()
{
    while (!m_stream->IsEOF())
    {
        long pos = m_stream->Tell();
        Chunk chunk = ReadChunk();

        switch (chunk.id)
        {
        case EDIT3DS:
            ProcessEditChunk(chunk);
            break;

        case KEYF3DS:
            // Keyframe data - ignorar por agora
            SkipChunk(chunk);
            break;

        default:
            SkipChunk(chunk);
            break;
        }

        // Garante que avançamos corretamente
        long newPos = pos + chunk.length;
        if (m_stream->Tell() < newPos)
            m_stream->Seek(newPos, SeekOrigin::Begin);

        if (m_stream->Tell() >= m_stream->Size())
            break;
    }
}

void Loader3DS::ProcessEditChunk(const Chunk &parent)
{
    long endPos = m_stream->Tell() + parent.length - parent.bytesRead;

    while (m_stream->Tell() < endPos)
    {
        long pos = m_stream->Tell();
        Chunk chunk = ReadChunk();

        switch (chunk.id)
        {
        case EDIT_MATERIAL:
            ProcessMaterialChunk(chunk);
            break;

        case EDIT_OBJECT:
            ProcessObjectChunk(chunk);
            break;

        default:
            SkipChunk(chunk);
            break;
        }

        long newPos = pos + chunk.length;
        if (m_stream->Tell() < newPos)
            m_stream->Seek(newPos, SeekOrigin::Begin);
    }
}

void Loader3DS::ProcessMaterialChunk(const Chunk &parent)
{
    Material3DS mat;
    mat.ambient[0] = mat.ambient[1] = mat.ambient[2] = 0.2f;
    mat.diffuse[0] = mat.diffuse[1] = mat.diffuse[2] = 0.8f;
    mat.specular[0] = mat.specular[1] = mat.specular[2] = 1.0f;
    mat.materialID = m_materials.size();
    LogInfo("[3DSLoader] Material: %d", mat.materialID);

    long endPos = m_stream->Tell() + parent.length - parent.bytesRead;

    while (m_stream->Tell() < endPos)
    {
        long pos = m_stream->Tell();
        Chunk chunk = ReadChunk();

        switch (chunk.id)
        {
        case MAT_NAME:
            mat.name = ReadString();
            break;

        case MAT_AMBIENT:
            ReadColorChunk(mat.ambient);
            break;

        case MAT_DIFFUSE:
            ReadColorChunk(mat.diffuse);
            break;

        case MAT_SPECULAR:
            ReadColorChunk(mat.specular);
            break;

        case MAT_TEXMAP:
        {
            long texEndPos = m_stream->Tell() + chunk.length - chunk.bytesRead;
            while (m_stream->Tell() < texEndPos)
            {
                Chunk texChunk = ReadChunk();
                if (texChunk.id == MAT_MAPNAME)
                {
                    mat.textureName = ReadString();
                    LogInfo("[3DSLoader] Texture: %s", mat.textureName.c_str());
                }
                else
                {
                    SkipChunk(texChunk);
                }
            }
            break;
        }

        default:
            SkipChunk(chunk);
            break;
        }

        long newPos = pos + chunk.length;
        if (m_stream->Tell() < newPos)
            m_stream->Seek(newPos, SeekOrigin::Begin);
    }

    m_materials.push_back(mat);
}

void Loader3DS::ProcessObjectChunk(const Chunk &parent)
{
    Object3DS obj;
    obj.name = ReadString();

    long endPos = m_stream->Tell() + parent.length - parent.bytesRead - (obj.name.length() + 1);

    while (m_stream->Tell() < endPos)
    {
        long pos = m_stream->Tell();
        Chunk chunk = ReadChunk();

        switch (chunk.id)
        {
        case OBJ_TRIMESH:
            ProcessMeshChunk(obj, chunk);
            break;

        case OBJ_LIGHT:
        case OBJ_CAMERA:
            // Ignorar por agora
            SkipChunk(chunk);
            break;

        default:
            SkipChunk(chunk);
            break;
        }

        long newPos = pos + chunk.length;
        if (m_stream->Tell() < newPos)
            m_stream->Seek(newPos, SeekOrigin::Begin);
    }

    if (!obj.vertices.empty())
    {
        m_objects.push_back(obj);
    }
}

void Loader3DS::ProcessMeshChunk(Object3DS &obj, const Chunk &parent)
{
    long endPos = m_stream->Tell() + parent.length - parent.bytesRead;

    while (m_stream->Tell() < endPos)
    {
        long pos = m_stream->Tell();
        Chunk chunk = ReadChunk();

        switch (chunk.id)
        {
        case TRI_VERTEXL:
        {
            u16 numVertices = m_stream->ReadUShort();
            obj.vertices.reserve(numVertices * 3);

            for (u16 i = 0; i < numVertices; i++)
            {
                float x = m_stream->ReadFloat();
                float z = m_stream->ReadFloat();
                float y = m_stream->ReadFloat();

                obj.vertices.push_back(x);
                obj.vertices.push_back(y);
                obj.vertices.push_back(z);
            }
            break;
        }

        case TRI_FACEL1:
        {
            u16 numFaces = m_stream->ReadUShort();
            obj.indices.reserve(numFaces * 3);

            for (u16 i = 0; i < numFaces; i++)
            {
                u16 c = m_stream->ReadUShort();
                u16 b = m_stream->ReadUShort();
                u16 a = m_stream->ReadUShort();
                u16 flags = m_stream->ReadUShort(); // flags de visibilidade
                (void)  flags;

                obj.indices.push_back(a);
                obj.indices.push_back(b);
                obj.indices.push_back(c);
            }

            // Pode ter sub-chunks com materiais
            long faceEndPos = pos + chunk.length;
            while (m_stream->Tell() < faceEndPos)
            {
                Chunk subChunk = ReadChunk();
                if (subChunk.id == TRI_MATERIAL)
                {
                    std::string matName = ReadString();
                    u16 numEntries = m_stream->ReadUShort();

                    // Marca quais faces usam este material
                    for (u16 i = 0; i < numEntries; i++)
                    {
                        u16 faceIndex = m_stream->ReadUShort();
                        // Por simplicidade, vamos criar um buffer por material
                        if (faceIndex < obj.materials.size())
                            obj.materials[faceIndex] = matName;
                        else
                            obj.materials.push_back(matName);
                    }
                }
                else
                {
                    SkipChunk(subChunk);
                }
            }
            break;
        }

        case TRI_MAPPINGCOORS:
        {
            u16 numCoords = m_stream->ReadUShort();
            obj.texCoords.reserve(numCoords * 2);

            for (u16 i = 0; i < numCoords; i++)
            {
                float u = m_stream->ReadFloat();
                float v = m_stream->ReadFloat();

                obj.texCoords.push_back(u);
                obj.texCoords.push_back(v);
            }
            break;
        }

        case TRI_SMOOTH:
        {
            u16 numFaces = obj.indices.size() / 3;
            obj.smoothGroups.reserve(numFaces);

            for (u16 i = 0; i < numFaces; i++)
            {
                u32 smoothGroup = m_stream->ReadUInt();
                obj.smoothGroups.push_back(smoothGroup);
            }
            break;
        }

        default:
            SkipChunk(chunk);
            break;
        }

        long newPos = pos + chunk.length;
        if (m_stream->Tell() < newPos)
            m_stream->Seek(newPos, SeekOrigin::Begin);
    }
}

u32 Loader3DS::FindMaterialID(const std::string &name)
{
    for (size_t i = 0; i < m_materials.size(); i++)
    {
        if (m_materials[i].name == name)
            return m_materials[i].materialID;
    }
    return 0;
}

void Loader3DS::BuildMesh(Mesh *mesh)
{
    // for (const auto &obj : m_objects)
    // {
    //     if (obj.vertices.empty() || obj.indices.empty())
    //         continue;

        // Cria um buffer por objeto
        
    //     size_t count = obj.materials.size();
    //     if (count > 0)
    //     {
             
    //             LogInfo("Material %s index %d ", obj.materials[0].c_str(), count);
            
            
    //     }
    //     MeshBuffer *buffer = mesh->AddBuffer(0);

    //     // Adiciona vértices
    //     size_t numVertices = obj.vertices.size() / 3;
    //     bool hasTexCoords = obj.texCoords.size() == numVertices * 2;

    //     for (size_t i = 0; i < numVertices; i++)
    //     {
    //         float x = obj.vertices[i * 3 + 0];
    //         float y = obj.vertices[i * 3 + 1];
    //         float z = obj.vertices[i * 3 + 2];

    //         float u = 0.0f, v = 0.0f;
    //         if (hasTexCoords)
    //         {
    //             u = obj.texCoords[i * 2 + 0];
    //             v = obj.texCoords[i * 2 + 1];
    //         }

    //         // Normais serão calculadas depois
    //         buffer->AddVertex(x, y, z, 0, 1, 0, u, v);
    //     }

    //     // Adiciona faces
    //     for (size_t i = 0; i < obj.indices.size(); i += 3)
    //     {
    //         buffer->AddFace(obj.indices[i], obj.indices[i + 1], obj.indices[i + 2]);
    //     }

    //     buffer->CalculateNormals();
    //     buffer->Build();
    // }


 
    for (const auto &obj : m_objects)
    {
        if (obj.vertices.empty() || obj.indices.empty())
            continue;


        MeshBuffer *buffer = mesh->AddBuffer(0);
        
        bool hasTexCoords = obj.texCoords.size() == (obj.vertices.size() / 3) * 2;
        

        
        for (size_t i = 0; i < obj.indices.size(); i += 3)
        {
            u32 i0 = obj.indices[i + 0];
            u32 i1 = obj.indices[i + 1];
            u32 i2 = obj.indices[i + 2];
            
            // Vértice 0
            float x0 = obj.vertices[i0 * 3 + 0];
            float y0 = obj.vertices[i0 * 3 + 1];
            float z0 = obj.vertices[i0 * 3 + 2];
            float u0 = hasTexCoords ? obj.texCoords[i0 * 2 + 0] : 0.0f;
            float v0 = hasTexCoords ? obj.texCoords[i0 * 2 + 1] : 0.0f;
            
            // Vértice 1
            float x1 = obj.vertices[i1 * 3 + 0];
            float y1 = obj.vertices[i1 * 3 + 1];
            float z1 = obj.vertices[i1 * 3 + 2];
            float u1 = hasTexCoords ? obj.texCoords[i1 * 2 + 0] : 0.0f;
            float v1 = hasTexCoords ? obj.texCoords[i1 * 2 + 1] : 0.0f;
            
            // Vértice 2
            float x2 = obj.vertices[i2 * 3 + 0];
            float y2 = obj.vertices[i2 * 3 + 1];
            float z2 = obj.vertices[i2 * 3 + 2];
            float u2 = hasTexCoords ? obj.texCoords[i2 * 2 + 0] : 0.0f;
            float v2 = hasTexCoords ? obj.texCoords[i2 * 2 + 1] : 0.0f;
            
       
            u32 idx0 = buffer->AddVertex(x0, y0, z0, 0, 0, 1, u0, v0);
            u32 idx1 = buffer->AddVertex(x1, y1, z1, 0, 0, 1, u1, v1);
            u32 idx2 = buffer->AddVertex(x2, y2, z2, 0, 0, 1, u2, v2);
            
            buffer->AddFace(idx0, idx1, idx2);
        }
        
 
        buffer->CalculateNormals(false); // false = flat shading
        buffer->Build();
    }
}

