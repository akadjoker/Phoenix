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

void Terrain::render(Shader *shader)
{
    if (m_blocks.empty())
        return;

    const Frustum *frustum = Driver::Instance().GetFrustum();
    // BoundingBox box = BoundingBox::Transform(getBoundingBox(), getWorldTransform());

    // if (!frustum->intersectsAABB(box))
    //     return;


    const Mat4 &mat = getWorldTransform(); 
    shader->SetModelMat4(mat.m);


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

void TerrainLod::CalculateNormals()
{
    meshBuffer->CalculateNormals(true);
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

void TerrainLod::update(float dt)
{
    Node3D::update(dt);

    if (PreRenderLODCalculations())
    {
        PreRenderIndicesCalculations();
    }
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
        else if (m_terrainData.patches[i].currentLOD == 3)
            batch->SetColor(128, 0, 128); // Roxo para LOD 3
        else if (m_terrainData.patches[i].currentLOD >= 4)
            batch->SetColor(255, 0, 0); // Vermelho para LOD 4 ou mais
        else
            batch->SetColor(100, 100, 100); // Cinza para não renderizado

        batch->Box(m_terrainData.patches[i].boundBox);
    }
}

TerrainLod::~TerrainLod()
{
    if (meshBuffer)
    {
        delete meshBuffer;
        meshBuffer = nullptr;
    }

    if (material)
    {
        delete material;
        material = nullptr;
    }

    if (m_terrainData.patches)
    {
        delete[] m_terrainData.patches;
        m_terrainData.patches = nullptr;
    }
}

void TerrainLod::CreatePatches()
{
    m_terrainData.patchCount = (m_terrainData.size - 1) / m_terrainData.calcPatchSize;

    if (m_terrainData.patchCount <= 0)
    {
        LogError("[TerrainLod] Invalid patch count: %d", m_terrainData.patchCount);
        return;
    }

    if (m_terrainData.patches)
    {
        delete[] m_terrainData.patches;
        m_terrainData.patches = nullptr;
    }

    const int totalPatches = m_terrainData.patchCount * m_terrainData.patchCount;
    m_terrainData.patches = new Patch[totalPatches];

    LogInfo("[TerrainLod] Created %d patches (%dx%d)",
            totalPatches, m_terrainData.patchCount, m_terrainData.patchCount);
}

void TerrainLod::CalculateDistanceThresholds(bool scaleChanged)
{

    if (!m_overrideDistanceThreshold)
    {
        m_terrainData.lodDistanceThreshold.clear();
        m_terrainData.lodDistanceThreshold.resize(m_terrainData.maxLOD);

        const float normalizedPatchSize = static_cast<float>(m_terrainData.patchSize) /
                                          static_cast<float>(m_terrainData.size - 1);

        // Tamanho real em world space (considerando a escala X e Z)
        const float patchWorldSizeX = normalizedPatchSize * m_terrainData.scale.x;
        const float patchWorldSizeZ = normalizedPatchSize * m_terrainData.scale.z;

       // const float patchWorldSize = std::max(patchWorldSizeX, patchWorldSizeZ);

        const float patchDiagonal = std::sqrt(patchWorldSizeX * patchWorldSizeX +
                                              patchWorldSizeZ * patchWorldSizeZ);

        LogInfo("[TerrainLod] Patch world size: %.2f x %.2f (diagonal: %.2f)",
                patchWorldSizeX, patchWorldSizeZ, patchDiagonal);

        for (int i = 0; i < m_terrainData.maxLOD; ++i)
        {
            // LOD 0 = detalhe máximo (perto)
            // LOD N = menos detalhe (longe)

            // Fórmula baseada na diagonal do patch
            // Cada LOD dobra a distância (progressão geométrica)
            float distance = patchDiagonal * 1.0f * std::pow(2.0f, static_cast<float>(i));

            m_terrainData.lodDistanceThreshold[i] = distance * distance;

            LogInfo("[TerrainLod] LOD %d threshold: %.2f units (squared: %.2f)",
                    i, distance, m_terrainData.lodDistanceThreshold[i]);
        }
    }
}

u32 TerrainLod::GetIndex(int patchX, int patchZ, int patchIndex, u32 vX, u32 vZ) const
{
    const Patch &patch = m_terrainData.patches[patchIndex];

    // Ajustar vértices nas bordas para prevenir T-junctions
    // Processar bordas verticais (esquerda/direita)
    if (vX == 0 && patch.left && patch.currentLOD < patch.left->currentLOD)
    {
        u32 step = 1 << patch.left->currentLOD;
        vZ = (vZ / step) * step; // Snap to coarser grid
    }
    else if (vX == static_cast<u32>(m_terrainData.calcPatchSize) &&
             patch.right && patch.currentLOD < patch.right->currentLOD)
    {
        u32 step = 1 << patch.right->currentLOD;
        vZ = (vZ / step) * step;
    }

    // Processar bordas horizontais (top/bottom)
    if (vZ == 0 && patch.top && patch.currentLOD < patch.top->currentLOD)
    {
        u32 step = 1 << patch.top->currentLOD;
        vX = (vX / step) * step;
    }
    else if (vZ == static_cast<u32>(m_terrainData.calcPatchSize) &&
             patch.bottom && patch.currentLOD < patch.bottom->currentLOD)
    {
        u32 step = 1 << patch.bottom->currentLOD;
        vX = (vX / step) * step;
    }

    // Clamp aos limites do patch
    vX = std::min(vX, static_cast<u32>(m_terrainData.calcPatchSize));
    vZ = std::min(vZ, static_cast<u32>(m_terrainData.calcPatchSize));

    return (vZ + (m_terrainData.calcPatchSize * patchZ)) * m_terrainData.size +
           (vX + (m_terrainData.calcPatchSize * patchX));
}

bool TerrainLod::PreRenderLODCalculations()
{
    const Camera *camera = Driver::Instance().GetCamera();
    const Frustum *frustum = Driver::Instance().GetFrustum();

    if (!camera || !frustum)
    {
        LogWarning("[TerrainLod] No camera or frustum available for LOD calculations.");
        return false;
    }

    Vec3 cameraRotation = camera->getEulerAngles();
    Vec3 cameraPosition = camera->getPosition();

    // Calcular deltas
    Vec3 posDelta = cameraPosition - m_oldCameraPosition;
    Vec3 rotDelta = cameraRotation - m_oldCameraRotation;
    float movementLength = posDelta.length();
    float rotationLength = std::abs(rotDelta.y);

    // Atualizar mesmo com movimento pequeno para suavizar transições
    // Remover o early return muito restritivo
    bool cameraMovedSignificantly = (movementLength >= m_cameraMovementDelta) ||
                                    (rotationLength >= m_cameraRotationDelta);

    m_oldCameraPosition = cameraPosition;
    m_oldCameraRotation = cameraRotation;

    bool lodChanged = false;
    const int count = m_terrainData.patchCount * m_terrainData.patchCount;

    for (int j = 0; j < count; ++j)
    {
        Patch &patch = m_terrainData.patches[j];
        int previousLOD = patch.currentLOD;

        if (frustum->intersectsAABB(patch.boundBox))
        {
            float distanceSq = (cameraPosition - patch.center).lengthSquared();

            // Determinar LOD baseado em distância
            int newLOD = 0;
            for (int i = m_terrainData.maxLOD - 1; i >= 0; --i)
            {
                if (distanceSq >= m_terrainData.lodDistanceThreshold[i])
                {
                    newLOD = i;
                    break;
                }
            }

            // HISTERESE: adicionar uma margem para evitar flickering
            // Apenas muda LOD se a distância mudou significativamente
            const float hysteresisMargin = 0.15f; // 15% de margem

            if (previousLOD != newLOD)
            {
                // Se está aumentando detalhe (LOD diminuindo)
                if (newLOD < previousLOD)
                {
                    float threshold = m_terrainData.lodDistanceThreshold[newLOD];
                    if (distanceSq < threshold * (1.0f - hysteresisMargin))
                    {
                        patch.currentLOD = newLOD;
                        lodChanged = true;
                    }
                    else
                    {
                        patch.currentLOD = previousLOD; // Manter LOD anterior
                    }
                }
                // Se está diminuindo detalhe (LOD aumentando)
                else
                {
                    float threshold = m_terrainData.lodDistanceThreshold[previousLOD];
                    if (distanceSq > threshold * (1.0f + hysteresisMargin))
                    {
                        patch.currentLOD = newLOD;
                        lodChanged = true;
                    }
                    else
                    {
                        patch.currentLOD = previousLOD; // Manter LOD anterior
                    }
                }
            }
            else
            {
                patch.currentLOD = newLOD;
            }
        }
        else
        {
            patch.currentLOD = -1; // Fora do frustum
            if (previousLOD != -1)
                lodChanged = true;
        }
    }

    // Retornar true se LOD mudou ou câmera moveu significativamente
    return lodChanged || cameraMovedSignificantly;
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

    // Converter para grayscale
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

    // Ajustar maxLOD baseado no tamanho do patch
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

    // Criar vértices
    const float tdSize = 1.0f / (m_terrainData.size - 1);

    for (int z = 0; z < m_terrainData.size; ++z)
    {
        for (int x = 0; x < m_terrainData.size; ++x)
        {
            int pixelIndex = z * sourceMap->width + x;
            u8 heightValue = sourceMap->pixels[pixelIndex * sourceMap->components];

            float height = (heightValue / 255.0f) * heightScale;

            float fx = x * tdSize;
            float fz = z * tdSize;

            Vec3 v = Vec3(fx, height, fz);
            v.x = (v.x * m_terrainData.scale.x) + m_terrainData.position.x;
            v.y = (v.y * m_terrainData.scale.y) + m_terrainData.position.y;
            v.z = (v.z * m_terrainData.scale.z) + m_terrainData.position.z;

            float tu = fx;
            float tv = fz;

            meshBuffer->AddVertex(v.x, v.y, v.z, 0, 1, 0, tu, tv);
        }
    }

    if (grayscale)
    {
        delete grayscale;
        grayscale = nullptr;
    }

    Smooth(smoothFactor);
    CalculateNormals();

    CalculateDistanceThresholds(false);

    // m_overrideDistanceThreshold = true;
    // m_terrainData.lodDistanceThreshold.resize(m_terrainData.maxLOD);
    // m_terrainData.lodDistanceThreshold[0] = 60.0f;   // 60^2
    // m_terrainData.lodDistanceThreshold[1] = 120.0f;  // 120^2
    // m_terrainData.lodDistanceThreshold[2] = 240.0f;  // 240^2
    // m_terrainData.lodDistanceThreshold[3] = 480.0f; // 480^2
    // m_terrainData.lodDistanceThreshold[4] = 960.0f; // 960^2

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

bool TerrainLod::ValidateTerrainData() const
{
    if (!meshBuffer)
    {
        LogError("[TerrainLod] MeshBuffer is null");
        return false;
    }

    if (!m_terrainData.patches)
    {
        LogError("[TerrainLod] Patches array is null");
        return false;
    }

    if (m_terrainData.patchCount <= 0)
    {
        LogError("[TerrainLod] Invalid patch count: %d", m_terrainData.patchCount);
        return false;
    }

    if (m_terrainData.size <= 0)
    {
        LogError("[TerrainLod] Invalid terrain size: %d", m_terrainData.size);
        return false;
    }

    if (meshBuffer->GetVertexCount() != m_terrainData.size * m_terrainData.size)
    {
        LogError("[TerrainLod] Vertex count mismatch: %d vs %d",
                 meshBuffer->GetVertexCount(),
                 m_terrainData.size * m_terrainData.size);
        return false;
    }

    return true;
}

void TerrainLod::render(Shader *shader)
{
    if (!ValidateTerrainData())
    {
        LogWarning("[TerrainLod] Cannot render: invalid terrain data");
        return;
    }

    if (m_indicesToRender == 0)
    {

        return;
    }
    const Mat4 &mat = getWorldTransform(); 
    shader->SetModelMat4(mat.m);

    ApplyMaterial();
    Driver::Instance().DrawMeshBuffer(meshBuffer, PrimitiveType::PT_TRIANGLES, m_indicesToRender);
}

float TerrainLod::GetHeight(float worldX, float worldZ) const
{
    if (!meshBuffer || meshBuffer->GetVertexCount() == 0)
    {
        LogWarning("[TerrainLod] Cannot get height: no mesh data");
        return 0.0f;
    }

    // Converter coordenadas world para local (0-1)
    float localX = (worldX - m_terrainData.position.x) / m_terrainData.scale.x;
    float localZ = (worldZ - m_terrainData.position.z) / m_terrainData.scale.z;

    // Clamp para limites do terreno
    localX = std::max(0.0f, std::min(1.0f, localX));
    localZ = std::max(0.0f, std::min(1.0f, localZ));

    // Converter para índices do grid
    float gridX = localX * (m_terrainData.size - 1);
    float gridZ = localZ * (m_terrainData.size - 1);

    // Índices inteiros do quad onde o ponto está
    int ix = static_cast<int>(gridX);
    int iz = static_cast<int>(gridZ);

    // Clamp aos limites válidos
    ix = std::min(ix, m_terrainData.size - 2);
    iz = std::min(iz, m_terrainData.size - 2);

    // Fração dentro do quad (0-1)
    float fx = gridX - ix;
    float fz = gridZ - iz;

    // Obter os 4 vértices do quad
    int index00 = iz * m_terrainData.size + ix;
    int index10 = iz * m_terrainData.size + (ix + 1);
    int index01 = (iz + 1) * m_terrainData.size + ix;
    int index11 = (iz + 1) * m_terrainData.size + (ix + 1);

    Vec3 v00 = meshBuffer->GetVertexPosition(index00);
    Vec3 v10 = meshBuffer->GetVertexPosition(index10);
    Vec3 v01 = meshBuffer->GetVertexPosition(index01);
    Vec3 v11 = meshBuffer->GetVertexPosition(index11);

    // Interpolação bilinear
    // Dividir quad em 2 triângulos e interpolar no triângulo correto
    float height;

    if (fx + fz <= 1.0f)
    {
        // Triângulo inferior-esquerdo (v00, v10, v01)
        height = v00.y + (v10.y - v00.y) * fx + (v01.y - v00.y) * fz;
    }
    else
    {
        // Triângulo superior-direito (v10, v11, v01)
        height = v11.y + (v01.y - v11.y) * (1.0f - fx) + (v10.y - v11.y) * (1.0f - fz);
    }

    return height;
}

// Sobrecarga para Vec3
float TerrainLod::GetHeight(const Vec3 &worldPos) const
{
    return GetHeight(worldPos.x, worldPos.z);
}

void TerrainLod::Smooth(int smoothFactor)
{
    if (smoothFactor <= 0 || !meshBuffer || meshBuffer->GetVertexCount() == 0)
        return;

    m_smoothFactor = smoothFactor;

    // Buffer temporário para as alturas
    std::vector<float> heights(m_terrainData.size * m_terrainData.size);

    // Copiar alturas atuais
    for (int i = 0; i < m_terrainData.size * m_terrainData.size; ++i)
    {
        heights[i] = meshBuffer->GetVertexPosition(i).y;
    }

    // Aplicar filtro de suavização várias vezes
    for (int iteration = 0; iteration < smoothFactor; ++iteration)
    {
        std::vector<float> smoothed = heights;

        for (int z = 1; z < m_terrainData.size - 1; ++z)
        {
            for (int x = 1; x < m_terrainData.size - 1; ++x)
            {
                int idx = z * m_terrainData.size + x;

                // Filtro gaussiano 3x3 simplificado
                float sum = 0.0f;
                float weight = 0.0f;

                // Centro (peso maior)
                sum += heights[idx] * 4.0f;
                weight += 4.0f;

                // Adjacentes (peso médio)
                sum += heights[idx - 1] * 2.0f;                  // Esquerda
                sum += heights[idx + 1] * 2.0f;                  // Direita
                sum += heights[idx - m_terrainData.size] * 2.0f; // Cima
                sum += heights[idx + m_terrainData.size] * 2.0f; // Baixo
                weight += 8.0f;

                // Diagonais (peso menor)
                sum += heights[idx - m_terrainData.size - 1]; // Top-left
                sum += heights[idx - m_terrainData.size + 1]; // Top-right
                sum += heights[idx + m_terrainData.size - 1]; // Bottom-left
                sum += heights[idx + m_terrainData.size + 1]; // Bottom-right
                weight += 4.0f;

                smoothed[idx] = sum / weight;
            }
        }

        heights = smoothed;
    }
 
    for (int z = 0; z < m_terrainData.size; ++z)
    {
        for (int x = 0; x < m_terrainData.size; ++x)
        {
            int idx = z * m_terrainData.size + x;
            Vec3 pos = meshBuffer->GetVertexPosition(idx);
            pos.y = heights[idx];
            meshBuffer->SetVertexPosition(idx, pos);
        }
    }

    LogInfo("[TerrainLod] Applied smoothing filter %d times", smoothFactor);
}

void TerrainLod::SmoothArea(float worldX, float worldZ, float radius, int iterations)
{
    if (!meshBuffer || meshBuffer->GetVertexCount() == 0)
        return;

    float localX = (worldX - m_terrainData.position.x) / m_terrainData.scale.x;
    float localZ = (worldZ - m_terrainData.position.z) / m_terrainData.scale.z;

    float centerGridX = localX * (m_terrainData.size - 1);
    float centerGridZ = localZ * (m_terrainData.size - 1);

    float radiusX = radius / m_terrainData.scale.x * (m_terrainData.size - 1);
    float radiusZ = radius / m_terrainData.scale.z * (m_terrainData.size - 1);
    float maxRadius = std::max(radiusX, radiusZ);

    int minX = std::max(1, static_cast<int>(centerGridX - maxRadius));
    int maxX = std::min(m_terrainData.size - 2, static_cast<int>(centerGridX + maxRadius));
    int minZ = std::max(1, static_cast<int>(centerGridZ - maxRadius));
    int maxZ = std::min(m_terrainData.size - 2, static_cast<int>(centerGridZ + maxRadius));
 
    std::vector<float> heights(m_terrainData.size * m_terrainData.size);

    for (int i = 0; i < m_terrainData.size * m_terrainData.size; ++i)
    {
        heights[i] = meshBuffer->GetVertexPosition(i).y;
    }

    for (int iter = 0; iter < iterations; ++iter)
    {
        std::vector<float> smoothed = heights;

        for (int z = minZ; z <= maxZ; ++z)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                float dx = (x - centerGridX) / radiusX;
                float dz = (z - centerGridZ) / radiusZ;
                float distSq = dx * dx + dz * dz;

                if (distSq <= 1.0f)
                {
                    int idx = z * m_terrainData.size + x;

                    // Filtro 3x3
                    float sum = heights[idx] * 4.0f;
                    float weight = 4.0f;

                    sum += heights[idx - 1] * 2.0f;
                    sum += heights[idx + 1] * 2.0f;
                    sum += heights[idx - m_terrainData.size] * 2.0f;
                    sum += heights[idx + m_terrainData.size] * 2.0f;
                    weight += 8.0f;

                    sum += heights[idx - m_terrainData.size - 1];
                    sum += heights[idx - m_terrainData.size + 1];
                    sum += heights[idx + m_terrainData.size - 1];
                    sum += heights[idx + m_terrainData.size + 1];
                    weight += 4.0f;

                    // Blend baseado em distância
                    float blendFactor = 1.0f - distSq;
                    smoothed[idx] = heights[idx] + (sum / weight - heights[idx]) * blendFactor;
                }
            }
        }

        heights = smoothed;
    }

    // Aplicar alturas suavizadas
    for (int z = minZ; z <= maxZ; ++z)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            float dx = (x - centerGridX) / radiusX;
            float dz = (z - centerGridZ) / radiusZ;
            float distSq = dx * dx + dz * dz;

            if (distSq <= 1.0f)
            {
                int idx = z * m_terrainData.size + x;
                Vec3 pos = meshBuffer->GetVertexPosition(idx);
                pos.y = heights[idx];
                meshBuffer->SetVertexPosition(idx, pos);
            }
        }
    }

    CalculateNormals();
    CalculatePatchData();
}
void TerrainLod::SetHeight(float worldX, float worldZ, float newHeight, float radius)
{
    if (!meshBuffer || meshBuffer->GetVertexCount() == 0)
    {
        LogWarning("[TerrainLod] Cannot set height: no mesh data");
        return;
    }

    // Converter coordenadas world para local
    float localX = (worldX - m_terrainData.position.x) / m_terrainData.scale.x;
    float localZ = (worldZ - m_terrainData.position.z) / m_terrainData.scale.z;

    // Converter para índices do grid
    float centerGridX = localX * (m_terrainData.size - 1);
    float centerGridZ = localZ * (m_terrainData.size - 1);

    // Calcular raio em unidades de grid
    float radiusX = radius / m_terrainData.scale.x * (m_terrainData.size - 1);
    float radiusZ = radius / m_terrainData.scale.z * (m_terrainData.size - 1);
    float maxRadius = std::max(radiusX, radiusZ);

    // Área de influência
    int minX = std::max(0, static_cast<int>(centerGridX - maxRadius));
    int maxX = std::min(m_terrainData.size - 1, static_cast<int>(centerGridX + maxRadius));
    int minZ = std::max(0, static_cast<int>(centerGridZ - maxRadius));
    int maxZ = std::min(m_terrainData.size - 1, static_cast<int>(centerGridZ + maxRadius));

    for (int z = minZ; z <= maxZ; ++z)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            // Distância do centro
            float dx = (x - centerGridX) / radiusX;
            float dz = (z - centerGridZ) / radiusZ;
            float distSq = dx * dx + dz * dz;

            if (distSq <= 1.0f)
            {
                // Peso baseado em distância (suave falloff)
                float weight = 1.0f - distSq; // Linear
                // Para falloff mais suave: float weight = cos(distSq * M_PI * 0.5f);

                int idx = z * m_terrainData.size + x;
                Vec3 pos = meshBuffer->GetVertexPosition(idx);

                // Interpolar entre altura atual e nova altura
                float currentHeight = (pos.y - m_terrainData.position.y) / m_terrainData.scale.y;
                float targetHeight = newHeight / m_terrainData.scale.y;
                float blendedHeight = currentHeight + (targetHeight - currentHeight) * weight;

                pos.y = (blendedHeight * m_terrainData.scale.y) + m_terrainData.position.y;
                meshBuffer->SetVertexPosition(idx, pos);
            }
        }
    }

    CalculateNormals();
    CalculatePatchData();
}

void TerrainLod::ModifyHeight(float worldX, float worldZ, float deltaHeight, float radius)
{
    if (!meshBuffer || meshBuffer->GetVertexCount() == 0)
    {
        LogWarning("[TerrainLod] Cannot modify height: no mesh data");
        return;
    }

    float localX = (worldX - m_terrainData.position.x) / m_terrainData.scale.x;
    float localZ = (worldZ - m_terrainData.position.z) / m_terrainData.scale.z;

    float centerGridX = localX * (m_terrainData.size - 1);
    float centerGridZ = localZ * (m_terrainData.size - 1);

    float radiusX = radius / m_terrainData.scale.x * (m_terrainData.size - 1);
    float radiusZ = radius / m_terrainData.scale.z * (m_terrainData.size - 1);
    float maxRadius = std::max(radiusX, radiusZ);

    int minX = std::max(0, static_cast<int>(centerGridX - maxRadius));
    int maxX = std::min(m_terrainData.size - 1, static_cast<int>(centerGridX + maxRadius));
    int minZ = std::max(0, static_cast<int>(centerGridZ - maxRadius));
    int maxZ = std::min(m_terrainData.size - 1, static_cast<int>(centerGridZ + maxRadius));

    for (int z = minZ; z <= maxZ; ++z)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            float dx = (x - centerGridX) / radiusX;
            float dz = (z - centerGridZ) / radiusZ;
            float distSq = dx * dx + dz * dz;

            if (distSq <= 1.0f)
            {
                // Falloff suave usando cosine
                float weight = std::cos(distSq * 3.14159f * 0.5f);

                int idx = z * m_terrainData.size + x;
                Vec3 pos = meshBuffer->GetVertexPosition(idx);

                // Aplicar delta com weight
                pos.y += (deltaHeight * weight * m_terrainData.scale.y);

                meshBuffer->SetVertexPosition(idx, pos);
            }
        }
    }

    CalculateNormals();
    CalculatePatchData();
}

void TerrainLod::Flatten(float worldX, float worldZ, float targetHeight, float radius, float strength)
{
    if (!meshBuffer || meshBuffer->GetVertexCount() == 0)
        return;

    float localX = (worldX - m_terrainData.position.x) / m_terrainData.scale.x;
    float localZ = (worldZ - m_terrainData.position.z) / m_terrainData.scale.z;

    float centerGridX = localX * (m_terrainData.size - 1);
    float centerGridZ = localZ * (m_terrainData.size - 1);

    float radiusX = radius / m_terrainData.scale.x * (m_terrainData.size - 1);
    float radiusZ = radius / m_terrainData.scale.z * (m_terrainData.size - 1);
    float maxRadius = std::max(radiusX, radiusZ);

    int minX = std::max(0, static_cast<int>(centerGridX - maxRadius));
    int maxX = std::min(m_terrainData.size - 1, static_cast<int>(centerGridX + maxRadius));
    int minZ = std::max(0, static_cast<int>(centerGridZ - maxRadius));
    int maxZ = std::min(m_terrainData.size - 1, static_cast<int>(centerGridZ + maxRadius));

    for (int z = minZ; z <= maxZ; ++z)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            float dx = (x - centerGridX) / radiusX;
            float dz = (z - centerGridZ) / radiusZ;
            float distSq = dx * dx + dz * dz;

            if (distSq <= 1.0f)
            {
                float weight = (1.0f - distSq) * strength;

                int idx = z * m_terrainData.size + x;
                Vec3 pos = meshBuffer->GetVertexPosition(idx);

                float currentHeight = (pos.y - m_terrainData.position.y) / m_terrainData.scale.y;
                float normalizedTarget = targetHeight / m_terrainData.scale.y;
                float newHeight = currentHeight + (normalizedTarget - currentHeight) * weight;

                pos.y = (newHeight * m_terrainData.scale.y) + m_terrainData.position.y;
                meshBuffer->SetVertexPosition(idx, pos);
            }
        }
    }

    CalculateNormals();
    CalculatePatchData();
}

bool RayIntersectTriangle(const Vec3 &rayOrigin, const Vec3 &rayDir,
                          const Vec3 &v0, const Vec3 &v1, const Vec3 &v2,
                          float &t, float &u, float &v)
{
    const float EPSILON = 0.0000001f;

    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    Vec3 h = rayDir.cross(edge2);
    float a = edge1.dot(h);

    if (a > -EPSILON && a < EPSILON)
        return false; // Ray paralelo ao triângulo

    float f = 1.0f / a;
    Vec3 s = rayOrigin - v0;
    u = f * s.dot(h);

    if (u < 0.0f || u > 1.0f)
        return false;

    Vec3 q = s.cross(edge1);
    v = f * rayDir.dot(q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * edge2.dot(q);

    return t > EPSILON;
}

TerrainRaycastHit TerrainLod::Raycast(const Ray &ray, float maxDistance) const
{
    TerrainRaycastHit result;

    if (!meshBuffer || meshBuffer->GetVertexCount() == 0)
    {
        LogWarning("[TerrainLod] Cannot raycast: no mesh data");
        return result;
    }

    // Primeiro verificar se o ray intersecta a bounding box do terreno
    float tMin, tMax;
    if (!ray.intersectAABB(getBoundingBox(), tMin, tMax))
    {
        return result;
    }

    float closestDistance = maxDistance;
 

    // Testar todos os patches visíveis
    const int patchCount = m_terrainData.patchCount * m_terrainData.patchCount;

    for (int patchIdx = 0; patchIdx < patchCount; ++patchIdx)
    {
        const Patch &patch = m_terrainData.patches[patchIdx];

        if (patch.currentLOD < 0)
            continue;

        if (!ray.intersectAABB(patch.boundBox, tMin, tMax))
            continue;

        int patchZ = patchIdx / m_terrainData.patchCount;
        int patchX = patchIdx % m_terrainData.patchCount;

        int step = 1 << patch.currentLOD; // LOD step

        int xStart = patchX * m_terrainData.calcPatchSize;
        int zStart = patchZ * m_terrainData.calcPatchSize;

        for (int z = 0; z < m_terrainData.calcPatchSize; z += step)
        {
            for (int x = 0; x < m_terrainData.calcPatchSize; x += step)
            {
                int gx = xStart + x;
                int gz = zStart + z;

                // Garantir que não excede limites
                if (gx + step >= m_terrainData.size || gz + step >= m_terrainData.size)
                    continue;

                // Índices dos 4 vértices do quad
                int idx00 = gz * m_terrainData.size + gx;
                int idx10 = gz * m_terrainData.size + (gx + step);
                int idx01 = (gz + step) * m_terrainData.size + gx;
                int idx11 = (gz + step) * m_terrainData.size + (gx + step);

                Vec3 v00 = meshBuffer->GetVertexPosition(idx00);
                Vec3 v10 = meshBuffer->GetVertexPosition(idx10);
                Vec3 v01 = meshBuffer->GetVertexPosition(idx01);
                Vec3 v11 = meshBuffer->GetVertexPosition(idx11);

                float t, u, v;

                // Triângulo 1: (v00, v10, v01)
                if (RayIntersectTriangle(ray.origin, ray.direction, v00, v10, v01, t, u, v))
                {
                    if (t < closestDistance && t > 0.0f)
                    {
                        closestDistance = t;
                        result.hit = true;
                        result.position = ray.origin + ray.direction * t;
                        result.distance = t;
                        result.gridX = gx;
                        result.gridZ = gz;

                        // Calcular normal do triângulo
                        Vec3 edge1 = v10 - v00;
                        Vec3 edge2 = v01 - v00;
                        result.normal = edge1.cross(edge2);
                        result.normal.normalize();

       
                    }
                }

                // Triângulo 2: (v10, v11, v01)
                if (RayIntersectTriangle(ray.origin, ray.direction, v10, v11, v01, t, u, v))
                {
                    if (t < closestDistance && t > 0.0f)
                    {
                        closestDistance = t;
                        result.hit = true;
                        result.position = ray.origin + ray.direction * t;
                        result.distance = t;
                        result.gridX = gx;
                        result.gridZ = gz;

                        // Calcular normal do triângulo
                        Vec3 edge1 = v11 - v10;
                        Vec3 edge2 = v01 - v10;
                        result.normal = edge1.cross(edge2);
                        result.normal.normalize();

              
                    }
                }
            }
        }
    }

    return result;
}