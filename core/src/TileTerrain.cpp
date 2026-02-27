#include "pch.h"
#include "Vertex.hpp"
#include "Driver.hpp"
#include "Frustum.hpp"
#include "Texture.hpp"
#include "Batch.hpp"
#include "Pixmap.hpp"
#include "glad/glad.h"
#include "Terrain.hpp"

TiledTerrain::TiledTerrain(int tilesInTextureSide,
                           float patchLength, int tilesPerPatch,
                           u8 defaultTile,u8 border, const std::string &name)
    : Node3D(name), m_mapWidth(0), m_mapHeight(0), m_tileMap(nullptr), m_patchBuffer(nullptr), m_tilesInTextureSide(8), m_patchSideLength(16.0f), m_tilesInPatchSide(4), m_defaultTile(0), m_border(border)
{
    material = new Material();
    Initialize(tilesInTextureSide, patchLength, tilesPerPatch, defaultTile);
}

TiledTerrain::~TiledTerrain()
{
    delete material;
    if (m_tileMap)
    {
        delete[] m_tileMap;
        m_tileMap = nullptr;
    }

    if (m_patchBuffer)
    {
        delete m_patchBuffer;
        m_patchBuffer = nullptr;
    }

    m_mapWidth = 0;
    m_mapHeight = 0;
}

void TiledTerrain::Initialize(int tilesInTextureSide,
                              float patchLength, int tilesPerPatch,
                              u8 defaultTile)
{

    m_tilesInTextureSide = tilesInTextureSide;
    m_patchSideLength = patchLength;
    m_tilesInPatchSide = tilesPerPatch;
    m_defaultTile = defaultTile;

    // Create patch buffer
    if (m_patchBuffer == nullptr)
    {
        m_patchBuffer = new MeshBuffer("TerrainPatch");

        m_patchBuffer->SetDynamicVertexBuffer(true);
        m_patchBuffer->SetDynamicIndexBuffer(true);
    }
}

void TiledTerrain::LoadTilemap(Pixmap *tileMapImage)
{
    if (!tileMapImage || !tileMapImage->IsValid())
        return;

    LoadTilemap(tileMapImage->width, tileMapImage->height, tileMapImage->pixels);
}

void TiledTerrain::LoadTilemap(u32 width, u32 height, const u8 *data)
{
    // Free existing
    if (m_tileMap)
    {
        delete[] m_tileMap;
        m_tileMap = nullptr;
    }

    m_mapWidth = width;
    m_mapHeight = height;

    u32 size = m_mapWidth * m_mapHeight;
    m_tileMap = new u8[size];
    memcpy(m_tileMap, data, size);
    LogWarning("Loaded tilemap: %ux%u", m_mapWidth, m_mapHeight);
}

void TiledTerrain::SetTileAt(u32 x, u32 z, u8 tile)
{
    if (x >= m_mapWidth || z >= m_mapHeight)
        return;

    m_tileMap[z * m_mapWidth + x] = tile;
}

u8 TiledTerrain::GetTileAt(u32 x, u32 z) const
{
    if (x >= m_mapWidth || z >= m_mapHeight)
        return m_defaultTile;

    return m_tileMap[z * m_mapWidth + x];
}

u8 TiledTerrain::GetTileAtWrapped(int x, int z) const
{
    if (!m_tileMap)
        return m_defaultTile;

    // Out of bounds - return default or wrap
    if (x < 0 || x >= (int)m_mapWidth ||
        z < 0 || z >= (int)m_mapHeight)
    {
        return m_defaultTile;
    }

    return m_tileMap[z * m_mapWidth + x];
}

void TiledTerrain::ApplyMaterial()
{
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

void TiledTerrain::render(Shader *shader, bool useMaterial)
{
    if (!m_patchBuffer || !m_tileMap)
    {
        LogError("Terrain is not initialized");
        return;
    }

    // Tamanho do tile individual
    float tileSize = m_patchSideLength / m_tilesInPatchSide;
    
    // Tamanho world com border incluído
    float borderSize = m_border * tileSize * m_tilesInPatchSide;  // border em world units
    float worldWidth = m_mapWidth * tileSize;
    float worldHeight = m_mapHeight * tileSize;

    
    m_boundBox.min = Vec3(-borderSize, -0.1f, -borderSize);
    m_boundBox.max = Vec3(worldWidth + borderSize, 0.1f, worldHeight + borderSize);

    const Camera *camera = Driver::Instance().GetCamera();
    const Frustum *frustum = Driver::Instance().GetFrustum();

    if (!camera || !frustum)
    {
        LogWarning("[TileTerrain] No camera or frustum");
        return;
    }

    const Mat4 &mat = getWorldTransform();
    shader->SetModelMat4(mat.m);

    if (useMaterial)
        ApplyMaterial();

    m_patchBuffer->ClearVertices();
    m_patchBuffer->ClearIndices();

    int patchCountX = (m_mapWidth + m_tilesInPatchSide - 1) / m_tilesInPatchSide;
    int patchCountZ = (m_mapHeight + m_tilesInPatchSide - 1) / m_tilesInPatchSide;
    
    // Loop com border
    for (int pz = -m_border; pz < patchCountZ + m_border; pz++)
    {
        for (int px = -m_border; px < patchCountX + m_border; px++)
        {
            if (!IsPatchVisible(px, pz, frustum))
                continue;
            RenderPatch(px, pz);
        }
    }


    // Vec3 camPos = camera->getPosition();

    // const float invPatchLen = 1.0f / m_patchSideLength;
    // int centerX = (int)(camPos.x * invPatchLen);
    // int centerZ = (int)(camPos.z * invPatchLen);

    // // Calculate visible radius
    // float farPlane = camera->getFar();
    // int radius = (int)(farPlane * invPatchLen) + 1;

    // // Render visible patches
    // for (int pz = centerZ - radius; pz <= centerZ + radius; pz++)
    // {
    //     for (int px = centerX - radius; px <= centerX + radius; px++)
    //     {
    //         if (!IsPatchVisible(px, pz, frustum))
    //             continue;

    //         RenderPatch(px, pz);
    //     }
    // }
}

void TiledTerrain::RenderPatch(int patchX, int patchZ)
{

    // Clear and rebuild patch geometry
    m_patchBuffer->ClearVertices();
    m_patchBuffer->ClearIndices();

    const float step = m_patchSideLength / m_tilesInPatchSide;
    const float stepUV = 1.0f / m_tilesInTextureSide;

    int startX = patchX * m_tilesInPatchSide;
    int startZ = patchZ * m_tilesInPatchSide;

    float patchWorldX = patchX * m_patchSideLength;
    float patchWorldZ = patchZ * m_patchSideLength;

    for (int tz = 0; tz < m_tilesInPatchSide; tz++)
    {
        for (int tx = 0; tx < m_tilesInPatchSide; tx++)
        {
            float x0 = patchWorldX + tx * step;
            float x1 = x0 + step;
            float z0 = patchWorldZ + tz * step;
            float z1 = z0 + step;

            int mapX = startX + tx;
            int mapZ = startZ + tz;

            // Detectar se está na borda
            bool isBorder = (mapX < 0 || mapX >= (int)m_mapWidth ||
                             mapZ < 0 || mapZ >= (int)m_mapHeight);

            u8 tile;
            if (isBorder)
            {
                tile = m_defaultTile;  
            }
            else
            {
                tile = GetTileAtWrapped(mapX, mapZ);
            }

            float uvs[4][2];
            GetTileUVs(tile, stepUV, uvs);

            u32 v0 = m_patchBuffer->AddVertex(x0, 0, z0, 0, 1, 0, uvs[0][0], uvs[0][1]);
            u32 v1 = m_patchBuffer->AddVertex(x0, 0, z1, 0, 1, 0, uvs[1][0], uvs[1][1]);
            u32 v2 = m_patchBuffer->AddVertex(x1, 0, z1, 0, 1, 0, uvs[2][0], uvs[2][1]);
            u32 v3 = m_patchBuffer->AddVertex(x1, 0, z0, 0, 1, 0, uvs[3][0], uvs[3][1]);

            m_patchBuffer->AddFace(v0, v1, v2);
            m_patchBuffer->AddFace(v0, v2, v3);
        }
    }

    m_patchBuffer->Build();
    Driver::Instance().DrawMeshBuffer(m_patchBuffer, PrimitiveType::PT_TRIANGLES, m_patchBuffer->GetIndexCount());
}

bool TiledTerrain::IsPatchVisible(int patchX, int patchZ, const Frustum *frustum) const
{
    float patchWorldX = patchX * m_patchSideLength;
    float patchWorldZ = patchZ * m_patchSideLength;

    Vec3 min(patchWorldX, -0.1f, patchWorldZ);
    Vec3 max(patchWorldX + m_patchSideLength,
             0.1f,
             patchWorldZ + m_patchSideLength);

    return frustum->intersectsAABB(min, max);
}

void TiledTerrain::GetTileUVs(u8 tile, float stepUV, float uvs[4][2]) const
{
    // Extract tile coordinates (6 bits = 3+3)
    int tileU = (tile & 7);        // 3 bits X (0-7)
    int tileV = ((tile >> 3) & 7); // 3 bits Y (0-7)
    int rotation = (tile >> 6);    // 2 bits rotation (0-3)

    float u0 = tileU * stepUV;
    float u1 = u0 + stepUV;
    float v0 = tileV * stepUV;
    float v1 = v0 + stepUV;

    // Apply rotation to UVs
    switch (rotation)
    {
    case 0: // 0° (normal)
        uvs[0][0] = u0;
        uvs[0][1] = v0;
        uvs[1][0] = u0;
        uvs[1][1] = v1;
        uvs[2][0] = u1;
        uvs[2][1] = v1;
        uvs[3][0] = u1;
        uvs[3][1] = v0;
        break;

    case 1: // 90° clockwise
        uvs[0][0] = u1;
        uvs[0][1] = v0;
        uvs[1][0] = u0;
        uvs[1][1] = v0;
        uvs[2][0] = u0;
        uvs[2][1] = v1;
        uvs[3][0] = u1;
        uvs[3][1] = v1;
        break;

    case 2: // 180°
        uvs[0][0] = u1;
        uvs[0][1] = v1;
        uvs[1][0] = u1;
        uvs[1][1] = v0;
        uvs[2][0] = u0;
        uvs[2][1] = v0;
        uvs[3][0] = u0;
        uvs[3][1] = v1;
        break;

    case 3: // 270° clockwise
        uvs[0][0] = u0;
        uvs[0][1] = v1;
        uvs[1][0] = u1;
        uvs[1][1] = v1;
        uvs[2][0] = u1;
        uvs[2][1] = v0;
        uvs[3][0] = u0;
        uvs[3][1] = v0;
        break;
    }
}
