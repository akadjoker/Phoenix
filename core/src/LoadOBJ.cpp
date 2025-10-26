#include "pch.h"
#include "Vertex.hpp"
#include "Mesh.hpp"
#include "Stream.hpp"
#include "Texture.hpp"
#include "Utils.hpp"

std::vector<std::string> OBJMeshLoader::GetExtensions() const
{
    return {"obj"};
}

u32 OBJMeshLoader::FindMaterialID(const std::string &name)
{
    auto it = m_materialMap.find(name);
    if (it != m_materialMap.end())
        return it->second;

    return 0;
}

bool OBJMeshLoader::LoadMTL(const std::string &mtlFilename)
{

    std::string mtlPath = m_basePath + mtlFilename;

    FileStream mtlStream(mtlPath, "r");
    if (!mtlStream.IsOpen())
    {
        LogWarning("[OBJLoader] Could not open MTL file: %s", mtlPath.c_str());
        return false;
    }

    OBJMaterial *currentMaterial = nullptr;

    while (!mtlStream.IsEOF())
    {
        std::string line = mtlStream.ReadLine();

        // Remove espaços
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos || line.empty() || line[start] == '#')
            continue;

        if (start > 0)
            line = line.substr(start);

        // newmtl: novo material
        if (line.substr(0, 7) == "newmtl ")
        {
            OBJMaterial mat;
            mat.name = line.substr(7);
            mat.diffuse = Vec3(0.8f, 0.8f, 0.8f);
            mat.specular = Vec3(0.5f, 0.5f, 0.5f);
            mat.shininess = 32.0f;

            m_materials.push_back(mat);
            currentMaterial = &m_materials.back();

            // Mapeia nome -> ID
            m_materialMap[mat.name] = static_cast<u32>(m_materials.size() - 1);
        }
        else if (currentMaterial)
        {
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;

            // Kd: diffuse color
            if (cmd == "Kd")
            {
                iss >> currentMaterial->diffuse.x >> currentMaterial->diffuse.y >> currentMaterial->diffuse.z;
            }
            // Ks: specular color
            else if (cmd == "Ks")
            {
                iss >> currentMaterial->specular.x >> currentMaterial->specular.y >> currentMaterial->specular.z;
            }
            // Ns: shininess
            else if (cmd == "Ns")
            {
                iss >> currentMaterial->shininess;
            }
            // map_Kd: texture
            else if (cmd == "map_Kd" || cmd == "map_Ka")
            {
                std::string texName;
                std::getline(iss, texName);
                // LogInfo("[OBJLoader] texture: %s", texName.c_str());
                
                // Remove espaços iniciais
                size_t texStart = texName.find_first_not_of(" \t");
                if (texStart != std::string::npos)
                {
                    texName = texName.substr(texStart);
                    currentMaterial->texture = texName;
                }
            }
        }
    }

    LogInfo("[OBJLoader] Loaded %zu materials from %s",
            m_materials.size(), mtlFilename.c_str());

    return true;
}

bool OBJMeshLoader::Load(Stream *stream, Mesh *mesh)
{
    if (!stream || !stream->IsOpen())
    {
        LogError("[OBJLoader] Invalid stream");
        return false;
    }

    // Guarda o diretório base para carregar MTL
  
    m_basePath = stream->GetPath();

    std::vector<TempVertex> positions;
    std::vector<TempTexCoord> texcoords;
    std::vector<TempNormal> normals;

    // Cria material default
    OBJMaterial defaultMat;
    defaultMat.name = "default";
    defaultMat.diffuse = Vec3(0.8f, 0.8f, 0.8f);
    defaultMat.specular = Vec3(0.5f, 0.5f, 0.5f);
    defaultMat.shininess = 32.0f;
    m_materials.push_back(defaultMat);
    m_materialMap["default"] = 0;

    MeshBuffer *currentBuffer = mesh->AddBuffer(0);
    std::unordered_map<FaceIndex, u32, FaceIndexHash> vertexCache;
    bool hasNormals = false;
    u32 currentMaterialID = 0;

    // Lê o arquivo linha por linha
    while (!stream->IsEOF())
    {
        std::string line = stream->ReadLine();

        // Remove espaços em branco
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos || line.empty() || line[start] == '#')
            continue;

        if (start > 0)
            line = line.substr(start);

        // mtllib: carrega biblioteca de materiais
        if (line.substr(0, 7) == "mtllib ")
        {
            std::string mtlFile = line.substr(7);
            LoadMTL(mtlFile);
        }
        // usemtl: muda material atual
        else if (line.substr(0, 7) == "usemtl ")
        {
            std::string materialName = line.substr(7);

            // Remove espaços
            size_t nameStart = materialName.find_first_not_of(" \t");
            if (nameStart != std::string::npos)
                materialName = materialName.substr(nameStart);

            currentMaterialID = FindMaterialID(materialName);

            // Cria novo buffer para este material
            currentBuffer = mesh->AddBuffer(currentMaterialID);
            vertexCache.clear(); // Cache por buffer

           // LogInfo("[OBJLoader] Switching to material: %s (ID: %u)",
           //         materialName.c_str(), currentMaterialID);
        }
        else
        {
            ParseLine(line, currentBuffer, positions, texcoords, normals,
                      vertexCache, hasNormals);
        }
    }

    // Exporta materiais para a mesh
    for (size_t i = 0; i < m_materials.size(); i++)
    {
        const auto &mat = m_materials[i];
        Material *material = mesh->AddMaterial(mat.name);
 
        material->SetDiffuse(mat.diffuse );
        material->SetSpecular(mat.specular );
        material->SetShininess(mat.shininess);

 
        if (!mat.texture.empty())
        {
            // Caminho completo da textura
            std::string texPath = mat.texture;

         


            Texture *tex = TextureManager::Instance().Add(texPath, true);
            if (tex)
            {

                tex->SetAnisotropy(8.0f);
                tex->SetMinFilter(FilterMode::LINEAR_MIPMAP);
                tex->SetMagFilter(FilterMode::LINEAR);
                material->SetTexture(0, tex);
                

            } else
            {
                
                LogError("[OBJLoader] Failed to load texture: %s", texPath.c_str());
            }

 
        } else 
        {
            LogWarning("[OBJLoader] Material %s has no texture", mat.name.c_str());
        }
 
    }
 
    if (!hasNormals)
    {
        mesh->CalculateNormals();
    }

    mesh->Build();

    LogInfo("[OBJLoader] Loaded: %zu vertices, %zu materials, %zu buffers",
            positions.size(), m_materials.size(), mesh->GetBufferCount());

    return true;
}

void OBJMeshLoader::ParseLine(const std::string &line, MeshBuffer *buffer,
                              std::vector<TempVertex> &positions,
                              std::vector<TempTexCoord> &texcoords,
                              std::vector<TempNormal> &normals,
                              std::unordered_map<FaceIndex, u32, FaceIndexHash> &vertexCache,
                              bool &hasNormals)
{
    if (line.substr(0, 2) == "v ")
    {
        TempVertex v;
        std::istringstream iss(line.substr(2));
        iss >> v.x >> v.y >> v.z;
        positions.push_back(v);
    }
    else if (line.substr(0, 3) == "vt ")
    {
        TempTexCoord vt;
        std::istringstream iss(line.substr(3));
        iss >> vt.u >> vt.v;
        texcoords.push_back(vt);
    }
    else if (line.substr(0, 3) == "vn ")
    {
        TempNormal vn;
        std::istringstream iss(line.substr(3));
        iss >> vn.nx >> vn.ny >> vn.nz;
        normals.push_back(vn);
        hasNormals = true;
    }
    else if (line.substr(0, 2) == "f ")
    {
        ParseFace(line, buffer, positions, texcoords, normals, vertexCache);
    }
}

void OBJMeshLoader::ParseFace(const std::string &line, MeshBuffer *buffer,
                              const std::vector<TempVertex> &positions,
                              const std::vector<TempTexCoord> &texcoords,
                              const std::vector<TempNormal> &normals,
                              std::unordered_map<FaceIndex, u32, FaceIndexHash> &vertexCache)
{
    std::istringstream iss(line.substr(2));
    std::vector<u32> faceIndices;
    std::string vertex;

    while (iss >> vertex)
    {
        FaceIndex fi = {0, 0, 0};

        size_t pos1 = vertex.find('/');
        if (pos1 == std::string::npos)
        {
            fi.v = std::stoi(vertex);
        }
        else
        {
            fi.v = std::stoi(vertex.substr(0, pos1));
            size_t pos2 = vertex.find('/', pos1 + 1);

            if (pos2 == std::string::npos)
            {
                fi.vt = std::stoi(vertex.substr(pos1 + 1));
            }
            else
            {
                if (pos2 > pos1 + 1)
                    fi.vt = std::stoi(vertex.substr(pos1 + 1, pos2 - pos1 - 1));
                fi.vn = std::stoi(vertex.substr(pos2 + 1));
            }
        }

        // Converte índices
        if (fi.v < 0)
            fi.v = positions.size() + fi.v + 1;
        if (fi.vt < 0)
            fi.vt = texcoords.size() + fi.vt + 1;
        if (fi.vn < 0)
            fi.vn = normals.size() + fi.vn + 1;

        auto it = vertexCache.find(fi);
        u32 index;

        if (it != vertexCache.end())
        {
            index = it->second;
        }
        else
        {
            Vertex v = {};

            if (fi.v > 0 && fi.v <= (int)positions.size())
            {
                v.x = positions[fi.v - 1].x;
                v.y = positions[fi.v - 1].y;
                v.z = positions[fi.v - 1].z;
            }

            if (fi.vt > 0 && fi.vt <= (int)texcoords.size())
            {
                v.u = texcoords[fi.vt - 1].u;
                v.v = texcoords[fi.vt - 1].v;
            }

            if (fi.vn > 0 && fi.vn <= (int)normals.size())
            {
                v.nx = normals[fi.vn - 1].nx;
                v.ny = normals[fi.vn - 1].ny;
                v.nz = normals[fi.vn - 1].nz;
            }
            else
            {
                v.nz = 1.0f;
            }

            index = buffer->AddVertex(v);
            vertexCache[fi] = index;
        }

        faceIndices.push_back(index);
    }

    // Triangula
    for (size_t i = 1; i + 1 < faceIndices.size(); i++)
    {
        buffer->AddFace(faceIndices[0], faceIndices[i], faceIndices[i + 1]);
    }
}