#include "pch.h"
#include "Vertex.hpp"
#include "Driver.hpp"
#include "Frustum.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "Stream.hpp"
#include "Batch.hpp"
#include "Pixmap.hpp"
#include "glad/glad.h"
#include "Terrain.hpp"

const int BLOCK_WIDTH = 17; // 17 vértices = 16 quads
const int BLOCK_HEIGHT = 17;
const int QUADS_WIDE = BLOCK_WIDTH - 1;
const int QUADS_HIGH = BLOCK_HEIGHT - 1;

// ============================================================================
// Terrain Implementation
// ============================================================================

Terrain::Terrain(const std::string &name) : Node3D(name)
{
    material = new Material();
    m_heightData = nullptr;
}

Terrain::~Terrain()
{
    delete material;

    for (MeshBuffer *buffer : m_blocks)
    {
        delete buffer;
    }
    m_blocks.clear();

    delete[] m_heightData;
}

bool Terrain::LoadFromHeightmap(const std::string &heightmapPath,
                                float scaleX, float scaleY, float scaleZ,
                                float texScaleU, float texScaleV)
{

    Pixmap heightmap;
    if (!heightmap.Load(heightmapPath.c_str()))
    {
        LogError("[Terrain] Failed to load heightmap: %s", heightmapPath.c_str());
        return false;
    }

    Pixmap *grayscale = nullptr;
    const Pixmap *sourceMap = &heightmap;

    if (heightmap.components > 1)
    {
        grayscale = new Pixmap(heightmap.width, heightmap.height, 1);
        for (int y = 0; y < heightmap.height; y++)
        {
            for (int x = 0; x < heightmap.width; x++)
            {
                Color pixel = heightmap.GetPixelColor(x, y);
                u8 gray = (u8)((pixel.r + pixel.g + pixel.b) / 3);
                grayscale->SetPixel(x, y, gray, gray, gray, 255);
            }
        }
        sourceMap = grayscale;
    }

    m_heightmapWidth = sourceMap->width;
    m_heightmapHeight = sourceMap->height;
    m_scale = Vec3(scaleX, scaleY, scaleZ);

    m_heightData = new float[m_heightmapWidth * m_heightmapHeight];
    for (int i = 0; i < m_heightmapWidth * m_heightmapHeight; i++)
    {
        u8 value = sourceMap->pixels[i];
        m_heightData[i] = (float)value / 255.0f;
    }

    if (grayscale)
        delete grayscale;

    FilterHeightMap();

    int blocksWide = (m_heightmapWidth - 1) / QUADS_WIDE;
    int blocksHigh = (m_heightmapHeight - 1) / QUADS_HIGH;

    LogInfo("[Terrain] Creating %dx%d blocks (%d total)", blocksWide, blocksHigh, blocksWide * blocksHigh);

    m_boundBox.clear();
    // Criar blocos
    for (int bz = 0; bz < blocksHigh; bz++)
    {
        for (int bx = 0; bx < blocksWide; bx++)
        {
            MeshBuffer *block = AddBuffer();
            if (!GenerateBlock(block, bx, bz, texScaleU, texScaleV))
            {
                delete block;
                LogError("[Terrain] Failed to generate block (%d, %d)", bx, bz);
                return false;
            }
        }
    }

    LogInfo("[Terrain] Terrain loaded successfully: %d blocks", m_blocks.size());
    return true;
}

void Terrain::FilterHeightMap()
{

    if (!m_heightData)
        return;
    float *pResult = new float[m_heightmapWidth * m_heightmapHeight];
    if (!pResult)
        return;

    memcpy(pResult, m_heightData, m_heightmapWidth * m_heightmapHeight * sizeof(float));

    float Value;

    // Filtro box 3x3 nos valores internos
    for (int z = 1; z < m_heightmapHeight - 1; z++)
    {
        for (int x = 1; x < m_heightmapWidth - 1; x++)
        {

            Value = m_heightData[(x - 1) + (z - 1) * m_heightmapWidth];
            Value += m_heightData[(x) + (z - 1) * m_heightmapWidth];
            Value += m_heightData[(x + 1) + (z - 1) * m_heightmapWidth];

            Value += m_heightData[(x - 1) + (z)*m_heightmapWidth];
            Value += m_heightData[(x) + (z)*m_heightmapWidth];
            Value += m_heightData[(x + 1) + (z)*m_heightmapWidth];

            Value += m_heightData[(x - 1) + (z + 1) * m_heightmapWidth];
            Value += m_heightData[(x) + (z + 1) * m_heightmapWidth];
            Value += m_heightData[(x + 1) + (z + 1) * m_heightmapWidth];

            // Store the result
            pResult[x + z * m_heightmapWidth] = Value / 9.0f;
        }
    }

    delete[] m_heightData;

    m_heightData = pResult;
}

float Terrain::GetHeight(int x, int z) const
{
    if (x < 0 || z < 0 || x >= m_heightmapWidth || z >= m_heightmapHeight)
        return 0.0f;

    return m_heightData[x + z * m_heightmapWidth];
}

Vec3 Terrain::CalculateNormal(int x, int z) const
{
    // Obter coordenadas dos vizinhos (com clamp)
    int x1 = std::min(x + 1, m_heightmapWidth - 1);
    int x0 = std::max(x - 1, 0);
    int z1 = std::min(z + 1, m_heightmapHeight - 1);
    int z0 = std::max(z - 1, 0);

    // Obter alturas normalizadas [0-1]
    float hL = GetHeight(x0, z);
    float hR = GetHeight(x1, z);
    float hD = GetHeight(x, z0);
    float hU = GetHeight(x, z1);

    // Calcular as distâncias reais entre os pontos
    float dx = (float)(x1 - x0) * m_scale.x;
    float dz = (float)(z1 - z0) * m_scale.z;

    // Calcular diferenças de altura escaladas
    float dhX = (hR - hL) * m_scale.y;
    float dhZ = (hU - hD) * m_scale.y;

    // Construir vetores tangentes
    Vec3 tangentX(dx, dhX, 0.0f);
    Vec3 tangentZ(0.0f, dhZ, dz);

    // Calcular normal (cross product)
    Vec3 normal = Vec3::Cross(tangentZ, tangentX);

    return Vec3::Normalize(normal);
}

void Terrain::Build()
{
    for (auto *block : m_blocks)
        block->Build();
}

MeshBuffer *Terrain::AddBuffer()
{
    MeshBuffer *buffer = new MeshBuffer();
    buffer->m_boundBox.clear();
    m_blocks.push_back(buffer);
    return buffer;
}

MeshBuffer *Terrain::GetBlock(int blockX, int blockZ) const
{
    int blocksWide = (m_heightmapWidth - 1) / QUADS_WIDE;
    int blocksHigh = (m_heightmapHeight - 1) / QUADS_HIGH;

    if (blockX < 0 || blockZ < 0 || blockX >= blocksWide || blockZ >= blocksHigh)
        return nullptr;

    return m_blocks[blockX + blockZ * blocksWide];
}

MeshBuffer *Terrain::GetBlock(u32 index) const
{
    if (index >= m_blocks.size())
        return nullptr;
    return m_blocks[index];
}

void Terrain::ApplyMaterial()
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

void Terrain::render()
{
    if (m_blocks.empty())
        return;

    const Frustum *frustum = Driver::Instance().GetFrustum();
    // BoundingBox box = BoundingBox::Transform(getBoundingBox(), getWorldTransform());

    // if (!frustum->intersectsAABB(box))
    //     return;

    ApplyMaterial();

    for (auto *block : m_blocks)
    {
        BoundingBox blockBox = BoundingBox::Transform(block->GetBoundingBox(), getWorldTransform());

        if (!frustum->intersectsAABB(blockBox))
            continue;

        Driver::Instance().DrawMeshBuffer(block, PrimitiveType::PT_TRIANGLE_STRIP, block->GetIndexCount());
    }
}

void Terrain::renderDebug(RenderBatch *batch)
{

    const Frustum *frustum = Driver::Instance().GetFrustum();
    if (!frustum->intersectsAABB(m_boundBox))
        return;

    batch->SetColor(255, 0, 0);
    batch->Box(getBoundingBox());
    batch->SetColor(0, 255, 0);
    for (auto *block : m_blocks)
    {
        if (!frustum->intersectsAABB(block->GetBoundingBox()))
            continue;
        batch->Box(block->GetBoundingBox());
    }
}

bool Terrain::GenerateBlock(MeshBuffer *block, int blockX, int blockZ,
                            float texScaleU, float texScaleV)
{
    int startX = blockX * QUADS_WIDE;
    int startZ = blockZ * QUADS_HIGH;

    // int vertexCount = BLOCK_WIDTH * BLOCK_HEIGHT;

    // ========================================================================
    // Gerar Vértices
    // ========================================================================
    for (int vz = 0; vz < BLOCK_HEIGHT; vz++)
    {
        for (int vx = 0; vx < BLOCK_WIDTH; vx++)
        {
            int worldX = startX + vx;
            int worldZ = startZ + vz;

            float px = (float)worldX * m_scale.x;
            float py = GetHeight(worldX, worldZ) * m_scale.y;
            float pz = (float)worldZ * m_scale.z;

            float u = ((float)worldX / (m_heightmapWidth - 1)) * texScaleU;
            float v = ((float)worldZ / (m_heightmapHeight - 1)) * texScaleV;

            Vec3 normal = CalculateNormal(worldX, worldZ);

            block->AddVertex(px, py, pz, normal.x, normal.y, normal.z, u, v);
        }
    }

    m_boundBox.merge(block->GetBoundingBox());

    u32 Counter = 0;

    // Calculate the indices for the terrain block tri-strip
    for (int vz = 0; vz < BLOCK_HEIGHT - 1; vz++)
    {
        // Is this an odd or even row?
        if ((vz % 2) == 0) // EVEN ROW
        {
            for (int vx = 0; vx < BLOCK_WIDTH; vx++)
            {
                // Force insert winding order switch degenerate?
                if (vx == 0 && vz > 0)
                {
                    block->AddIndex((u32)(vx + vz * BLOCK_WIDTH));
                    Counter++;
                }

                // Insert next two indices
                block->AddIndex((u32)(vx + vz * BLOCK_WIDTH));
                block->AddIndex((u32)((vx + vz * BLOCK_WIDTH) + BLOCK_WIDTH));
                Counter += 2;

            } // Next Index Column

        } // End if even row
        else // ODD ROW
        {
            for (int vx = BLOCK_WIDTH - 1; vx >= 0; vx--)
            {
                // Force insert winding order switch degenerate?
                if (vx == (BLOCK_WIDTH - 1))
                {
                    block->AddIndex((u32)(vx + vz * BLOCK_WIDTH));
                    Counter++;
                }

                // Insert next two indices
                block->AddIndex((u32)(vx + vz * BLOCK_WIDTH));
                block->AddIndex((u32)((vx + vz * BLOCK_WIDTH) + BLOCK_WIDTH));
                Counter += 2;

            } // Next Index Column

        } // End if odd row

    } // Next Index Row

    block->Build();

    // LogInfo("[Terrain] Block (%d, %d): %d vertices, %d indices",
    //         blockX, blockZ, block->GetVertexCount(), block->GetIndexCount());

    return true;
}

// ============================================================================

TerrainLod::TerrainLod(const std::string &name, int maxLOD, PatchSize patchSize, const Vec3 &position, const Vec3 &scale)
    : Node3D(name),
      m_terrainData(static_cast<int>(patchSize), maxLOD, position, scale),
      m_verticesToRender(0),
      m_indicesToRender(0),
  
      m_overrideDistanceThreshold(false),
      m_textureScale(1.0f),
      m_smoothFactor(0)
{
    
    material = new Material();
    m_oldCameraPosition = Vec3::Zero;
    m_oldCameraRotation = Vec3::Zero;
    m_cameraMovementDelta = 0.2f;
    m_cameraRotationDelta = 0.0010f;
    meshBuffer = new MeshBuffer();
 
}

TerrainLod::~TerrainLod()
{

    delete meshBuffer;
    delete material;
 
    if (m_terrainData.patches)
        delete[] m_terrainData.patches;
}

void TerrainLod::SetPosition(const Vec3 &pos)
{
    m_terrainData.position = pos;
    ApplyTransformation();
    
}

void TerrainLod::SetScale(const Vec3 &scl)
{
    m_terrainData.scale = scl;
    ApplyTransformation();
    CalculateNormals();
    
}

void TerrainLod::ApplyTransformation()
{
    
    // CalculateDistanceThresholds();
    CalculatePatchData();
}

void TerrainLod::PreRenderIndicesCalculations()
{
     
 
    m_indicesToRender = 0;
    meshBuffer->ClearIndices();

    int index = 0;

    const int count = m_terrainData.patchCount;

    for (int i = 0; i < count; ++i)
    {
        for (int j = 0; j < count; ++j)
        {
            if (m_terrainData.patches[index].currentLOD >= 0)
            {
                int x = 0;
                int z = 0;
                const int step = 1 << m_terrainData.patches[index].currentLOD;

                while (z < m_terrainData.calcPatchSize)
                {
                    const u32 index11 = GetIndex(j, i, index, x, z);
                    const u32 index21 = GetIndex(j, i, index, x + step, z);
                    const u32 index12 = GetIndex(j, i, index, x, z + step);
                    const u32 index22 = GetIndex(j, i, index, x + step, z + step);

          
                    meshBuffer->AddIndex(index22);
                    meshBuffer->AddIndex(index11);
                    meshBuffer->AddIndex(index12);
                    meshBuffer->AddIndex(index22);
                    meshBuffer->AddIndex(index21);
                    meshBuffer->AddIndex(index11);

                    m_indicesToRender += 6;

                    x += step;
                    if (x >= m_terrainData.calcPatchSize)
                    {
                        x = 0;
                        z += step;
                    }
                }
            }
            ++index;
        }
    }
    //    LogInfo("[TerrainLod] Total indices to render: %d", m_indicesToRender);
 
    meshBuffer->Build();
}

void TerrainLod::ApplyMaterial()
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

u32 TerrainLod::GetIndex(int patchX, int patchZ, int patchIndex, u32 vX, u32 vZ) const
{
    // Ajustar para evitar cracks entre patches de diferentes LODs
    const Patch &patch = m_terrainData.patches[patchIndex];

    // Borda superior
    if (vZ == 0 && patch.top && patch.currentLOD < patch.top->currentLOD)
    {
        u32 step = 1 << patch.top->currentLOD;
        vX -= vX % step;
    }
    // Borda inferior
    else if (vZ == static_cast<u32>(m_terrainData.calcPatchSize) &&
             patch.bottom && patch.currentLOD < patch.bottom->currentLOD)
    {
        u32 step = 1 << patch.bottom->currentLOD;
        vX -= vX % step;
    }

    // Borda esquerda
    if (vX == 0 && patch.left && patch.currentLOD < patch.left->currentLOD)
    {
        u32 step = 1 << patch.left->currentLOD;
        vZ -= vZ % step;
    }
    // Borda direita
    else if (vX == static_cast<u32>(m_terrainData.calcPatchSize) &&
             patch.right && patch.currentLOD < patch.right->currentLOD)
    {
        u32 step = 1 << patch.right->currentLOD;
        vZ -= vZ % step;
    }

    if (vZ >= static_cast<u32>(m_terrainData.patchSize))
        vZ = m_terrainData.calcPatchSize;
    if (vX >= static_cast<u32>(m_terrainData.patchSize))
        vX = m_terrainData.calcPatchSize;

    return (vZ + (m_terrainData.calcPatchSize * patchZ)) * m_terrainData.size +
           (vX + (m_terrainData.calcPatchSize * patchX));
}

void TerrainLod::Smooth(int smoothFactor)
{
}

void TerrainLod::CalculateNormals()
{
    meshBuffer->CalculateNormals(true);
}

void TerrainLod::CreatePatches()
{
    m_terrainData.patchCount = (m_terrainData.size - 1) / m_terrainData.calcPatchSize;

    if (m_terrainData.patches)
        delete[] m_terrainData.patches;

    m_terrainData.patches = new Patch[m_terrainData.patchCount * m_terrainData.patchCount];
}

void TerrainLod::CalculatePatchData()
{
    m_boundBox.clear();

    if (meshBuffer->GetVertexCount() > 0)
    {
        
        m_boundBox.reset(meshBuffer->GetVertexPosition(0));
    }

    for (int x = 0; x < m_terrainData.patchCount; ++x)
    {
        for (int z = 0; z < m_terrainData.patchCount; ++z)
        {
            int index = x * m_terrainData.patchCount + z;
            Patch &patch = m_terrainData.patches[index];

            int xstart = x * m_terrainData.calcPatchSize;
            int xend = xstart + m_terrainData.calcPatchSize;
            int zstart = z * m_terrainData.calcPatchSize;
            int zend = zstart + m_terrainData.calcPatchSize;

            patch.boundBox.reset(meshBuffer->GetVertexPosition((xstart * m_terrainData.size + zstart)));

            for (int xx = xstart; xx <= xend; ++xx)
                for (int zz = zstart; zz <= zend; ++zz)
                    patch.boundBox.addPoint(meshBuffer->GetVertexPosition(xx * m_terrainData.size + zz));

            m_boundBox.merge(patch.boundBox);
            patch.center = patch.boundBox.getCenter();

            // Atribuir vizinhos
            patch.top = (x > 0) ? &m_terrainData.patches[(x - 1) * m_terrainData.patchCount + z] : nullptr;
            patch.bottom = (x < m_terrainData.patchCount - 1) ? &m_terrainData.patches[(x + 1) * m_terrainData.patchCount + z] : nullptr;
            patch.left = (z > 0) ? &m_terrainData.patches[x * m_terrainData.patchCount + z - 1] : nullptr;
            patch.right = (z < m_terrainData.patchCount - 1) ? &m_terrainData.patches[x * m_terrainData.patchCount + z + 1] : nullptr;
        }
    }

    m_terrainData.center = m_boundBox.getCenter();
}
void TerrainLod::CalculateDistanceThresholds(bool scaleChanged)
{
    if (!m_overrideDistanceThreshold)
    {
        m_terrainData.lodDistanceThreshold.clear();

        // float size = m_terrainData.patchSize * m_terrainData.patchSize;

        //  float patchWorldSize = m_terrainData.patchSize * Max(m_terrainData.scale.x, m_terrainData.scale.z);

        const double size = (m_terrainData.patchSize * m_terrainData.patchSize); // * (m_terrainData.scale.x * m_terrainData.scale.z);
        m_terrainData.lodDistanceThreshold.reserve(m_terrainData.maxLOD);

        for (int i = 0; i < m_terrainData.maxLOD; ++i)
        {
            // LOD 0 = mais próximo (mais detalhe)
            // LOD N = mais longe (menos detalhe)
            double distance = size * ((i + 1 + i / 2) * (i + 1 + i / 2));

            m_terrainData.lodDistanceThreshold[i] = static_cast<float>(distance);
            m_terrainData.lodDistanceThreshold[i] *= 0.5f;

            LogInfo("[TerrainLod] LOD %d  (distance: %.2f)", i, distance);
        }
    }
}

 


 

bool TerrainLod::PreRenderLODCalculations()
{
    // Frustum* frustum = camera->GetFrustum();

    const Camera *camera = Driver::Instance().GetCamera();
    const Frustum *frustum = Driver::Instance().GetFrustum();
    // if (!frustum->intersectsAABB(m_boundBox))
    //      return;

    if (!camera || !frustum)
    {
        LogWarning("[TerrainLod] No camera or frustum available for LOD calculations.");
        return false;
    }

    Vec3 cameraRotation = camera->getEulerAngles();
    Vec3 cameraPosition = camera->getPosition();

    Vec3 posDelta = cameraPosition - m_oldCameraPosition;
    Vec3 rotDelta = cameraRotation - m_oldCameraRotation;
    float length = posDelta.length();
    float rotLength = std::abs(rotDelta.y);

   // if ((length !=  0.0f) || (rotLength != 0.0f))
   //     LogInfo("Camera  Delta Length: %.4f  Rotation Delta Y: %.4f", length, rotLength);

    if ( length < m_cameraMovementDelta &&  rotLength < m_cameraRotationDelta)
    {
         return false;
    }


    m_oldCameraPosition = cameraPosition;
    m_oldCameraRotation = cameraRotation;

    const int count = m_terrainData.patchCount * m_terrainData.patchCount;

    for (int j = 0; j < count; ++j)
    {

        if (frustum->intersectsAABB(m_terrainData.patches[j].boundBox))
        {
            float distanceSq = (cameraPosition - m_terrainData.patches[j].center).lengthSquared();

            m_terrainData.patches[j].currentLOD = 0;
            for (int i = m_terrainData.maxLOD - 1; i > 0; --i)
            {
                //     LogInfo("DistanceSq to patch %d: %f > %f", i, distanceSq, m_terrainData.lodDistanceThreshold[i]);
                if (distanceSq >= m_terrainData.lodDistanceThreshold[i])
                {
                    m_terrainData.patches[j].currentLOD = i;
                    break;
                }
            }
        }
        else
        {
            m_terrainData.patches[j].currentLOD = -1;
        }
    }

    return true;
}

void TerrainLod::update(float dt)
{
    Node3D::update(dt);

    if (PreRenderLODCalculations())
    {
        PreRenderIndicesCalculations();
    }
}

void TerrainLod::render()
{
    if (!meshBuffer )
    {
        return;
        LogWarning("[TerrainLod] No buffer to render.");
    }

    

    ApplyMaterial();
    Driver::Instance().DrawMeshBuffer(meshBuffer, PrimitiveType::PT_TRIANGLES, m_indicesToRender);
}

void TerrainLod::debug(RenderBatch *batch)
{

    int count = m_terrainData.patchCount * m_terrainData.patchCount;

    for (int i = 0; i < count; ++i)
    {
     
        if (m_terrainData.patches[i].currentLOD == 0)
            batch->SetColor(0, 255, 0); // Verde para LOD 0
        else if (m_terrainData.patches[i].currentLOD == 1)
            batch->SetColor(255, 255, 0); // Amarelo para LOD 1
        else if (m_terrainData.patches[i].currentLOD == 2)
            batch->SetColor(255, 165, 0); // Laranja para LOD 2
        else if (m_terrainData.patches[i].currentLOD >= 3)
            batch->SetColor(255, 0, 0); // Vermelho para LOD 3 ou mais
        else
            batch->SetColor(100, 100, 100); // Cinza para não renderizado

        batch->Box(m_terrainData.patches[i].boundBox);
    }


}

float TerrainLod::GetHeight(float x, float z) const
{

    return 0.0f;
}

bool TerrainLod::LoadHeightMap(const std::string &filename,
                               float heightScale,
                               int smoothFactor)
{
    const u32 startTime = SDL_GetTicks();

    Pixmap heightmap;
    if (!heightmap.Load(filename.c_str()))
    {
        LogError("[TerrainLod] Failed to load heightmap: %s", filename.c_str());
        return false;
    }

    // Converter para grayscale se necessário
    Pixmap *grayscale = nullptr;
    const Pixmap *sourceMap = &heightmap;

    if (heightmap.components > 1)
    {
        grayscale = new Pixmap(heightmap.width, heightmap.height, 1);
        for (int y = 0; y < heightmap.height; y++)
        {
            for (int x = 0; x < heightmap.width; x++)
            {
                Color pixel = heightmap.GetPixelColor(x, y);
                u8 gray = (u8)((pixel.r + pixel.g + pixel.b) / 3);
                grayscale->SetPixel(x, y, gray, gray, gray, 255);
            }
        }
        sourceMap = grayscale;
    }

    m_terrainData.size = sourceMap->width;

    if (sourceMap->width != sourceMap->height)
    {
        LogWarning("[TerrainLod] Heightmap is not square: %dx%d",
                   sourceMap->width, sourceMap->height);
    }

    switch (m_terrainData.patchSize)
    {
    case PATCH_9:
        m_terrainData.maxLOD = std::min(m_terrainData.maxLOD, 3);
        break;
    case PATCH_17:
        m_terrainData.maxLOD = std::min(m_terrainData.maxLOD, 4);
        break;
    case PATCH_33:
        m_terrainData.maxLOD = std::min(m_terrainData.maxLOD, 5);
        break;
    case PATCH_65:
        m_terrainData.maxLOD = std::min(m_terrainData.maxLOD, 6);
        break;
    case PATCH_129:
        m_terrainData.maxLOD = std::min(m_terrainData.maxLOD, 7);
        break;
    }



    const float tdSize = 1.0f / (m_terrainData.size - 1);

    for (int z = 0; z < m_terrainData.size; ++z)
    {
        for (int x = 0; x < m_terrainData.size; ++x)
        {
            // Ler altura do pixel
            int pixelIndex = z * sourceMap->width + x;
            u8 heightValue = sourceMap->pixels[pixelIndex * sourceMap->components];

            // Normalizar 0-255 para 0.0-1.0 e aplicar escala
            float height = (heightValue / 255.0f) * heightScale;

            // Coordenadas normalizadas
            float fx = x * tdSize;
            float fz = z * tdSize;

            Vec3 v = Vec3(fx, height, fz);
            v.x = (v.x * m_terrainData.scale.x) + m_terrainData.position.x;
            v.y = (v.y * m_terrainData.scale.y) + m_terrainData.position.y;
            v.z = (v.z * m_terrainData.scale.z) + m_terrainData.position.z;

            // UVs para textura
            float tu = fx;
            float tv = fz;


            meshBuffer->AddVertex(v.x, v.y, v.z, 0, 1, 0, tu, tv);
        }
    }

    // Cleanup
    if (grayscale)
    {
        delete grayscale;
    }

    Smooth(smoothFactor);
    CalculateNormals();

    // Processar terreno
    // CalculateDistanceThresholds();
    m_terrainData.lodDistanceThreshold.reserve(10);
    m_terrainData.lodDistanceThreshold[0] = 60.0f;
    m_terrainData.lodDistanceThreshold[1] = 120.0f;
    m_terrainData.lodDistanceThreshold[2] = 240.0f;
    m_terrainData.lodDistanceThreshold[3] = 480.0f;
    m_terrainData.lodDistanceThreshold[4] = 960.0f;

    CreatePatches();
    CalculatePatchData();

    // Criar buffers
    int maxIndices = m_terrainData.patchCount * m_terrainData.patchCount *
                     m_terrainData.calcPatchSize * m_terrainData.calcPatchSize * 6;



    meshBuffer->CreateIndexBuffer(maxIndices, true);
    PreRenderIndicesCalculations();

 


    const u32 endTime = SDL_GetTicks();

    LogInfo("[TerrainLod] Heightmap loaded in %d ms: size=%d, vertices=%d, patches=%d",
            endTime - startTime,
            m_terrainData.size,
            meshBuffer->GetVertexCount(),
            m_terrainData.patchCount * m_terrainData.patchCount);

    return true;
}
