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
// TerrainBlock Implementation
// ============================================================================

TerrainBlock::TerrainBlock()
{
    buffer = new VertexArray();
    vb = nullptr;
    ib = nullptr;
    m_boundBox.clear();
    m_vdirty = true;
    m_idirty = true;
}

TerrainBlock::~TerrainBlock()
{
    delete buffer;
}

void TerrainBlock::Build()
{
    if (!vb)
    {
        vb = buffer->AddVertexBuffer(sizeof(TerrainVertex), m_vertices.size(), false);

        auto *decl = buffer->GetVertexDeclaration();

        decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
        decl->AddElement(0, 3 * sizeof(float), VET_FLOAT3, VES_NORMAL);
        decl->AddElement(0, 6 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    }

    if (!ib)
    {
        ib = buffer->CreateIndexBuffer(m_indices.size(), false, false);
    }

    if (m_vdirty)
    {
        vb->SetData(m_vertices.data());
    }

    if (m_idirty)
    {
        ib->SetData(m_indices.data());
    }

    m_idirty = false;
    m_vdirty = false;
    buffer->Build();
}

void TerrainBlock::Render()
{
    if (!buffer || m_indices.empty())
        return;

    buffer->Render(PT_TRIANGLE_STRIP, m_indices.size());
}

// ============================================================================
// Terrain Implementation
// ============================================================================

Terrain::Terrain(const std::string &name) : Visual(name)
{
    material = new Material();
    materials.push_back(material);
    m_heightData = nullptr;
}

Terrain::~Terrain()
{

    delete[] m_heightData;
    for (auto *block : m_blocks)
        delete block;
    m_blocks.clear();
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
            TerrainBlock *block = new TerrainBlock();
            if (!GenerateBlock(block, bx, bz, texScaleU, texScaleV))
            {
                delete block;
                LogError("[Terrain] Failed to generate block (%d, %d)", bx, bz);
                return false;
            }
            m_boundBox.merge(block->m_boundBox);
            m_blocks.push_back(block);
        }
    }

    LogInfo("[Terrain] Terrain loaded successfully: %d blocks", m_blocks.size());
    return true;
}


void Terrain::FilterHeightMap()
{

    if (!m_heightData) return;
    float * pResult = new float[m_heightmapWidth * m_heightmapHeight];
    if (!pResult) return;
    
    memcpy( pResult, m_heightData, m_heightmapWidth * m_heightmapHeight * sizeof(float) );

     float Value;

    // Filtro box 3x3 nos valores internos
    for (int z = 1; z < m_heightmapHeight - 1; z++)
    {
        for (int x = 1; x < m_heightmapWidth - 1; x++)
        {
           

            Value  = m_heightData[ (x - 1) + (z - 1) * m_heightmapWidth ]; 
            Value += m_heightData[ (x    ) + (z - 1) * m_heightmapWidth ]; 
            Value += m_heightData[ (x + 1) + (z - 1) * m_heightmapWidth ]; 

            Value += m_heightData[ (x - 1) + (z    ) * m_heightmapWidth ]; 
            Value += m_heightData[ (x    ) + (z    ) * m_heightmapWidth ]; 
            Value += m_heightData[ (x + 1) + (z    ) * m_heightmapWidth ]; 

            Value += m_heightData[ (x - 1) + (z + 1) * m_heightmapWidth ]; 
            Value += m_heightData[ (x    ) + (z + 1) * m_heightmapWidth ]; 
            Value += m_heightData[ (x + 1) + (z + 1) * m_heightmapWidth ]; 

            // Store the result
            pResult[ x + z * m_heightmapWidth ] = Value / 9.0f;
        }
    }

    delete [] m_heightData;

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

TerrainBlock *Terrain::GetBlock(int x, int z) const
{
    if (x < 0 || z < 0 || x >= m_heightmapWidth || z >= m_heightmapHeight)
        return nullptr;
    return m_blocks[x + z * m_heightmapWidth];
}

TerrainBlock *Terrain::GetBlock(u32 index) const
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

void Terrain::Render()
{
    if (m_blocks.empty())
        return;

 

    const Frustum *frustum = Driver::Instance().GetFrustum();
    if (!frustum->intersectsAABB(m_boundBox))
        return;
        
    ApplyMaterial(); 


    for (auto *block : m_blocks)
    {
        if (!frustum->intersectsAABB(block->m_boundBox))
            continue;
        block->Render();
 
    }      
    
     
}

void Terrain::Debug(RenderBatch *batch)
{
  

    const Frustum *frustum = Driver::Instance().GetFrustum();
    if (!frustum->intersectsAABB(m_boundBox))
        return;
        
     


    batch->SetColor(255, 0, 0);
    batch->Box(m_boundBox);
    batch->SetColor(0, 255, 0);
    for (auto *block : m_blocks)
    {
        if (!frustum->intersectsAABB(block->m_boundBox))
            continue;
        batch->Box(block->m_boundBox);
     
    }

   
}

bool Terrain::GenerateBlock(TerrainBlock* block, int blockX, int blockZ,
                            float texScaleU, float texScaleV)
{
    int startX = blockX * QUADS_WIDE;
    int startZ = blockZ * QUADS_HIGH;

    int vertexCount = BLOCK_WIDTH * BLOCK_HEIGHT;
    
    block->m_vertices.reserve(vertexCount);
    block->m_boundBox.clear();

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

            TerrainVertex vert;
            vert.x = px; vert.y = py; vert.z = pz;
            vert.nx = normal.x; vert.ny = normal.y; vert.nz = normal.z;
            vert.u = u; vert.v = v;
            
            block->m_vertices.push_back(vert);
            block->m_boundBox.expand(px, py, pz);
        }
    }

 
    block->m_indices.clear();
    u32 Counter = 0;
    
    // Calculate the indices for the terrain block tri-strip
    for (int vz = 0; vz < BLOCK_HEIGHT - 1; vz++)
    {
        // Is this an odd or even row?
        if ((vz % 2) == 0)  // EVEN ROW
        {
            for (int vx = 0; vx < BLOCK_WIDTH; vx++)
            {
                // Force insert winding order switch degenerate?
                if (vx == 0 && vz > 0)
                {
                    block->m_indices.push_back((u32)(vx + vz * BLOCK_WIDTH));
                    Counter++;
                }

                // Insert next two indices
                block->m_indices.push_back((u32)(vx + vz * BLOCK_WIDTH));
                block->m_indices.push_back((u32)((vx + vz * BLOCK_WIDTH) + BLOCK_WIDTH));
                Counter += 2;

            } // Next Index Column
            
        } // End if even row
        else  // ODD ROW
        {
            for (int vx = BLOCK_WIDTH - 1; vx >= 0; vx--)
            {
                // Force insert winding order switch degenerate?
                if (vx == (BLOCK_WIDTH - 1))
                {
                    block->m_indices.push_back((u32)(vx + vz * BLOCK_WIDTH));
                    Counter++;
                }

                // Insert next two indices
                block->m_indices.push_back((u32)(vx + vz * BLOCK_WIDTH));
                block->m_indices.push_back((u32)((vx + vz * BLOCK_WIDTH) + BLOCK_WIDTH));
                Counter += 2;

            } // Next Index Column

        } // End if odd row
    
    } // Next Index Row

    block->Build();
    
    LogInfo("[Terrain] Block (%d, %d): %d vertices, %d indices",
            blockX, blockZ, block->m_vertices.size(), block->m_indices.size());
    
    return true;
}
