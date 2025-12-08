

#include "Core.hpp"

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <cstring>
#include <algorithm>

// ============================================================================
// ESTRUTURAS BSP/BSX
// ============================================================================

struct BSPVector
{
    float x, y, z;
    BSPVector() : x(0), y(0), z(0) {}
    BSPVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct BSPVertex
{
    BSPVector position;
    struct
    {
        float u, v;
    } textureCoord;
    struct
    {
        float u, v;
    } lightmapCoord;
};

struct BSPTexture
{
    char name[64];
    s32 flags;
    s32 contents;
};

struct BSPFace
{
    s32 textureID;
    s32 effect;
    s32 type;
    s32 firstVertex;
    s32 vertexesCount;
    s32 firstMeshVertex;
    s32 meshVertexesCount;
    s32 lightmapID;
    s32 lightmapCorner[2];
    s32 lightmapSize[2];
    float lightmapPosition[3];
    float lightmapVectors[6];
    float normal[3];
    s32 patchSize[2];
    // TOTAL: 104 bytes

    enum Type
    {
        POLYGON = 1,
        PATCH = 2,
        MESH = 3,
        BILLBOARD = 4
    };
};

struct BSPLump
{
    s32 offset;
    s32 length;
};

struct BSPHeader
{
    char magic[4]; // "IBSP"
    s32 version;
};

struct BSPFullVertex
{
    BSPVector position;
    float textureCoord[2];
    float lightmapCoord[2];
    BSPVector normal;
    unsigned char color[4];
};

// ============================================================================
// PATCH TESSELLATOR (Bezier Surface)
// ============================================================================

class PatchTessellator
{
public:
    // Tessela um patch 3x3 (9 pontos de controle)
    static void Tessellate(
        const BSPVertex *controlPoints,
        int level,
        std::vector<DetailVertex> &outVertices,
        std::vector<u32> &outIndices,
        u32 baseIndex)
    {
        int numVerts = (1 << level) + 1; // 2^level + 1

        // Gerar grid de vértices
        for (int i = 0; i < numVerts; i++)
        {
            float t = (float)i / (numVerts - 1);

            for (int j = 0; j < numVerts; j++)
            {
                float s = (float)j / (numVerts - 1);

                DetailVertex v = EvalBezier(controlPoints, t, s);
                outVertices.push_back(v);
            }
        }

        // Gerar índices (triângulos)
        for (int i = 0; i < numVerts - 1; i++)
        {
            for (int j = 0; j < numVerts - 1; j++)
            {
                u32 tl = baseIndex + i * numVerts + j;
                u32 tr = tl + 1;
                u32 bl = tl + numVerts;
                u32 br = bl + 1;

                outIndices.push_back(tl);
                outIndices.push_back(bl);
                outIndices.push_back(tr);

                outIndices.push_back(tr);
                outIndices.push_back(bl);
                outIndices.push_back(br);
            }
        }
    }

private:
    static DetailVertex EvalBezier(const BSPVertex *cp, float t, float s)
    {
        DetailVertex result;
        result.x = result.y = result.z = 0;
        result.u = result.v = 0;
        result.lu = result.lv = 0;

        // Avaliar superfície Bezier bicúbica (3x3 = quadrática)
        for (int i = 0; i < 3; i++)
        {
            float bi = Bernstein(i, 2, t);
            for (int j = 0; j < 3; j++)
            {
                float bj = Bernstein(j, 2, s);
                float b = bi * bj;

                int idx = i * 3 + j;
                result.x += cp[idx].position.x * b;
                result.y += cp[idx].position.y * b;
                result.z += cp[idx].position.z * b;
                result.u += cp[idx].textureCoord.u * b;
                result.v += cp[idx].textureCoord.v * b;
                result.lu += cp[idx].lightmapCoord.u * b;
                result.lv += cp[idx].lightmapCoord.v * b;
            }
        }

        return result;
    }

    static float Bernstein(int i, int n, float t)
    {
        return Binomial(n, i) * powf(t, i) * powf(1.0f - t, n - i);
    }

    static int Binomial(int n, int k)
    {
        if (k > n)
            return 0;
        if (k == 0 || k == n)
            return 1;

        int result = 1;
        for (int i = 0; i < k; i++)
        {
            result *= (n - i);
            result /= (i + 1);
        }
        return result;
    }
};

// ============================================================================
// MATERIAL GROUP (para agrupar faces)
// ============================================================================

struct MaterialGroup
{
    u32 textureID;
    u32 lightmapID;
    std::string textureName;
    std::vector<DetailVertex> vertices;
    std::vector<u32> indices;

    MaterialGroup() : textureID(0), lightmapID(0) {}
};

// ============================================================================
// CONVERSOR BSX
// ============================================================================

class BSXConverter
{
private:
    BSPVertex *vertices;
    s32 vertexCount;
    s32 *meshVertices;
    s32 meshVertexCount;
    BSPFace *faces;
    s32 faceCount;
    BSPTexture *textures;
    s32 textureCount;
    s32 lightmapsCount;

    std::vector<Vec3> spawnPoints;
    std::vector<Vec3> healthPacks;
    std::vector<Vec3> weapons;
    std::vector<Vec3> armor;
    std::vector<Vec3> ammo;

public:
    BSXConverter() : vertices(nullptr), vertexCount(0),
                     meshVertices(nullptr), meshVertexCount(0),
                     faces(nullptr), faceCount(0),
                     textures(nullptr), textureCount(0), lightmapsCount(0) {}

    ~BSXConverter() { Clear(); }

    void Clear()
    {
        delete[] vertices;
        delete[] meshVertices;
        delete[] faces;
        delete[] textures;
        vertices = nullptr;
        meshVertices = nullptr;
        faces = nullptr;
        textures = nullptr;
    }

    bool Load(const std::string &filename)
    {
        FileStream file;
        if (!file.Open(filename, "rb"))
        {
            printf("ERRO: Não consegui abrir %s\n", filename.c_str());
            return false;
        }

        printf("Lendo entidades...\n");
        LoadEntities(file);

        printf("Lendo vértices...\n");
        vertexCount = file.ReadInt();
        printf("  - %d vértices\n", vertexCount);
        vertices = new BSPVertex[vertexCount];
        file.Read(vertices, vertexCount * sizeof(BSPVertex));
        for (s32 i = 0; i < vertexCount; ++i)
        {
            LogInfo("Vertice %d: %f %f %f\n", i, vertices[i].position.x, vertices[i].position.y, vertices[i].position.z);
        }

        printf("Lendo mesh vertices...\n");
        meshVertexCount = file.ReadInt();
        printf("  - %d indices\n", meshVertexCount);
        if (meshVertexCount > 0)
        {
            meshVertices = new s32[meshVertexCount];
            file.Read(meshVertices, meshVertexCount * sizeof(s32));
        }

        printf("Lendo faces...\n");
        faceCount = file.ReadInt();
        printf("  - %d faces\n", faceCount);
        faces = new BSPFace[faceCount];
        file.Read(faces, faceCount * sizeof(BSPFace));
        for (s32 i = 0; i < faceCount; ++i)
        {
            LogInfo("Face %d: %d vertices\n", i, faces[i].firstVertex);
        }

        printf("Lendo texturas...\n");
        textureCount = file.ReadInt();
        printf("  - %d texturas\n", textureCount);
        textures = new BSPTexture[textureCount];
        file.Read(textures, textureCount * sizeof(BSPTexture));

        for (s32 i = 0; i < textureCount; ++i)
        {
            std::string name = textures[i].name;
            LogInfo("Texture %d: %s\n", i, name.c_str());

        
        }

        printf("Lendo lightmaps...\n");
        lightmapsCount = file.ReadInt();
        printf("  - %d lightmaps\n", lightmapsCount);

       
        file.Close();

        printf("BSX carregado !\n\n");
        return true;
    }

    bool Convert(const std::string &outputFile)
    {
        printf("Agrupando faces por material...\n");
        std::map<std::pair<u32, u32>, MaterialGroup> groups;

        int patchCount = 0, polyCount = 0, meshCount = 0;

        for (int i = 0; i < faceCount; i++)
        {
            BSPFace &face = faces[i];

            // Ignorar lightmaps inválidos
            s32 lmID = (face.lightmapID < 0) ? 0 : face.lightmapID;
            auto key = std::make_pair((u32)face.textureID, (u32)lmID);

            MaterialGroup &group = groups[key];
            if (group.vertices.empty())
            {
                group.textureID = face.textureID;
                group.lightmapID = lmID;
                if (face.textureID >= 0 && face.textureID < textureCount)
                {
                    group.textureName = textures[face.textureID].name;
                }
            }

            u32 baseIdx = (u32)group.vertices.size();

            switch (face.type)
            {
            case BSPFace::POLYGON:
                polyCount++;
                ProcessPolygon(face, group, baseIdx);
                break;

            case BSPFace::MESH:
                meshCount++;
                ProcessMesh(face, group, baseIdx);
                break;

            case BSPFace::PATCH:
                patchCount++;
                ProcessPatch(face, group, baseIdx);
                break;
            }
        }

        printf("Processadas:\n");
        printf("  - %d polígonos\n", polyCount);
        printf("  - %d meshes\n", meshCount);
        printf("  - %d patches\n", patchCount);
        printf("  - %zu grupos de material\n\n", groups.size());

        return SaveMap(outputFile, groups);
    }

private:
    void LoadEntities(FileStream &file)
    {
        s32 count;
        BSPVector pos;

        // 1) startingPositions -> spawnPoints
        count = file.ReadInt();
        for (s32 i = 0; i < count; ++i)
        {
            file.Read(&pos, sizeof(BSPVector));
            spawnPoints.push_back(Vec3(pos.x, pos.y, pos.z));
        }

        // 2) medikitPositions -> healthPacks
        count = file.ReadInt();
        for (s32 i = 0; i < count; ++i)
        {
            file.Read(&pos, sizeof(BSPVector));
            healthPacks.push_back(Vec3(pos.x, pos.y, pos.z));
        }

        // 3) foodPositions -> (podes ignorar ou usar como "extra health")
        count = file.ReadInt();
        for (s32 i = 0; i < count; ++i)
        {
            file.Read(&pos, sizeof(BSPVector));
            // opcional: healthPacks.push_back(Vec3(pos.x, pos.y, pos.z));
            // ou simplesmente ignorar (mas TEM de ser lido!)
        }

        // 4) armorPositions -> armor
        count = file.ReadInt();
        for (s32 i = 0; i < count; ++i)
        {
            file.Read(&pos, sizeof(BSPVector));
            armor.push_back(Vec3(pos.x, pos.y, pos.z));
        }

        // 5) bulletsPositions -> ammo
        count = file.ReadInt();
        for (s32 i = 0; i < count; ++i)
        {
            file.Read(&pos, sizeof(BSPVector));
            ammo.push_back(Vec3(pos.x, pos.y, pos.z));
        }

        // 6) grenadesPositions -> podes juntar a ammo ou ignorar
        count = file.ReadInt();
        for (s32 i = 0; i < count; ++i)
        {
            file.Read(&pos, sizeof(BSPVector));
            // ammo.push_back(Vec3(pos.x, pos.y, pos.z));
        }

        // 7) weaponPositions -> weapons
        count = file.ReadInt();
        for (s32 i = 0; i < count; ++i)
        {
            file.Read(&pos, sizeof(BSPVector));
            weapons.push_back(Vec3(pos.x, pos.y, pos.z));
        }
    }

    void ProcessPolygon(BSPFace &face, MaterialGroup &group, u32 baseIdx)
    {
        // Adicionar vértices
        for (s32 i = 0; i < face.vertexesCount; i++)
        {
            BSPVertex &v = vertices[face.firstVertex + i];
            DetailVertex dv(
                v.position.x, v.position.y, v.position.z,
                v.textureCoord.u, v.textureCoord.v,
                v.lightmapCoord.u, v.lightmapCoord.v);
            group.vertices.push_back(dv);
        }

        // Adicionar índices (fan triangulation)
        for (s32 i = 2; i < face.vertexesCount; i++)
        {
            group.indices.push_back(baseIdx);
            group.indices.push_back(baseIdx + i - 1);
            group.indices.push_back(baseIdx + i);
        }
    }

    void ProcessMesh(BSPFace &face, MaterialGroup &group, u32 baseIdx)
    {
        // Adicionar vértices
        for (s32 i = 0; i < face.vertexesCount; i++)
        {
            BSPVertex &v = vertices[face.firstVertex + i];
            DetailVertex dv(
                v.position.x, v.position.y, v.position.z,
                v.textureCoord.u, v.textureCoord.v,
                v.lightmapCoord.u, v.lightmapCoord.v);
            group.vertices.push_back(dv);
        }

        // Adicionar índices
        for (s32 i = 0; i < face.meshVertexesCount; i++)
        {
            s32 idx = meshVertices[face.firstMeshVertex + i];
            group.indices.push_back(baseIdx + idx);
        }
    }

    void ProcessPatch(BSPFace &face, MaterialGroup &group, u32 /*baseIdx*/)
    {
        s32 width = face.patchSize[0];
        s32 height = face.patchSize[1];
        s32 widthCount = (width - 1) / 2;
        s32 heightCount = (height - 1) / 2;

        for (s32 y = 0; y < heightCount; y++)
        {
            for (s32 x = 0; x < widthCount; x++)
            {
                BSPVertex controlPoints[9];

                for (s32 row = 0; row < 3; row++)
                {
                    for (s32 col = 0; col < 3; col++)
                    {
                        s32 idx = face.firstVertex +
                                  (y * 2 * width + x * 2) +
                                  row * width + col;
                        controlPoints[row * 3 + col] = vertices[idx];
                    }
                }

                // Base index é simplesmente o tamanho atual antes de adicionar os novos vértices
                u32 patchBaseIdx = (u32)group.vertices.size();

                PatchTessellator::Tessellate(
                    controlPoints,
                    3, // nível 3 = 9x9
                    group.vertices,
                    group.indices,
                    patchBaseIdx);
            }
        }
    }

    bool SaveMap(const std::string &filename,
                 const std::map<std::pair<u32, u32>, MaterialGroup> &groups)
    {
        FileStream file;
        if (!file.Open(filename, "wb"))
            return false;

        // HEADER
        file.WriteUInt(0x4D415032); // "MAP2"
        file.WriteUInt(1);          // versão

        // 1) TEXTURAS
        file.WriteUInt((u32)textureCount);
        for (s32 i = 0; i < textureCount; ++i)
        {
            std::string name =Utils::GetFileName( textures[i].name);
            
            file.WriteUTF(name.c_str());
            LogInfo("Texture %d: %s\n", i, name.c_str());

            
        }

        // 2) LIGHTMAPS
        file.WriteUInt((u32)lightmapsCount); // vindo do BSX/BSP

        // 3) SURFACES / BUFFERS
        file.WriteUInt((u32)groups.size());
        for (auto &p : groups)
        {
            const MaterialGroup &g = p.second;

            file.WriteUInt((u32)g.textureID);  // índice na tabela de texturas
            file.WriteUInt((u32)g.lightmapID); // índice 0..lightmapsCount-1

            file.WriteUInt((u32)g.vertices.size());
            file.Write(g.vertices.data(), g.vertices.size() * sizeof(DetailVertex));

            file.WriteUInt((u32)g.indices.size());
            file.Write(g.indices.data(), g.indices.size() * sizeof(u32));
        }

        file.Close();
        return true;
    }

    void WriteVec3Array(FileStream &file, const std::vector<Vec3> &arr)
    {
        file.WriteUInt((u32)arr.size());
        for (const Vec3 &v : arr)
        {
            file.WriteFloat(v.x);
            file.WriteFloat(v.y);
            file.WriteFloat(v.z);
        }
    }
};

// ============================================================================
// LOADER MAPMESH
// ============================================================================

// class MapMeshLoader {
// public:
//     static MapMesh* Load(const std::string& filename) {
//         FileStream file;
//         if (!file.Open(filename, "rb")) {
//             printf("ERRO: Não consegui abrir %s\n", filename.c_str());
//             return nullptr;
//         }

//         // Verificar header
//         u32 magic = file.ReadUInt();
//         u32 version = file.ReadUInt();

//         if (magic != 0x4D415032) {
//             printf("ERRO: Arquivo inválido (magic = 0x%X)\n", magic);
//             file.Close();
//             return nullptr;
//         }

//         if (version != 1) {
//             printf("ERRO: Versão não suportada: %u\n", version);
//             file.Close();
//             return nullptr;
//         }

//         printf("Carregando mapa: %s\n", filename.c_str());

//         MapMesh* mesh = new MapMesh(filename);

//         // Buffers
//         u32 bufferCount = file.ReadUInt();
//         printf("  - %u buffers\n", bufferCount);

//         for (u32 i = 0; i < bufferCount; i++) {
//             u32 texID = file.ReadUInt();
//             u32 lmID = file.ReadUInt();
//             std::string texName = file.ReadUTF();

//             DetailMeshBuffer* buffer = mesh->AddBuffer(texID);

//             u32 vertCount = file.ReadUInt();
//             buffer->vertices.resize(vertCount);
//             file.Read(buffer->vertices.data(), vertCount * sizeof(DetailVertex));

//             u32 idxCount = file.ReadUInt();
//             buffer->indices.resize(idxCount);
//             file.Read(buffer->indices.data(), idxCount * sizeof(u32));
//         }

//         // Entidades
//         ReadVec3Array(file, mesh->spawnPoints);
//         ReadVec3Array(file, mesh->healthPacks);
//         ReadVec3Array(file, mesh->weapons);

//         file.Close();

//         mesh->Build();
//         mesh->CalculateBoundingBox();

//         printf("Mapa carregado!\n");
//         printf("  - %zu spawn points\n", mesh->GetSpawnPoints().size());

//         return mesh;
//     }

// private:
//     static void ReadVec3Array(FileStream& file, std::vector<Vec3>& arr) {
//         u32 count = file.ReadUInt();
//         arr.resize(count);
//         for (u32 i = 0; i < count; i++) {
//             arr[i].x = file.ReadFloat();
//             arr[i].y = file.ReadFloat();
//             arr[i].z = file.ReadFloat();
//         }
//     }
// };

// ============================================================================
// MAIN
// ============================================================================

// int main(int argc, char *argv[])
// {
//     printf("╔════════════════════════════════════════╗\n");
//     printf("║   Conversor BSX/BSP → Formato MAP2    ║\n");
//     printf("╚════════════════════════════════════════╝\n\n");

//     if (argc < 3)
//     {
//         printf("Uso: %s <entrada.bsx> <saida.map>\n", argv[0]);
//         return 1;
//     }

//     std::string input = argv[1];
//     std::string output = argv[2];

//     BSXConverter converter;

//     if (!converter.Load(input))
//     {
//         return 1;
//     }

//     if (!converter.Convert(output))
//     {
//         return 1;
//     }

int main()

{

    std::string input = "/media/djoker/code/projects/cpp/Phoenix/assets/city.bsx";
    std::string output = "/media/djoker/code/projects/cpp/Phoenix/assets/city.map";

    BSXConverter converter;

    if (!converter.Load(input))
    {
        return 1;
    }

    if (!converter.Convert(output))
    {
        return 1;
    }
    printf("\n✓ Conversão completa!\n");
    printf("  Arquivo: %s\n", output.c_str());

    return 0;
}