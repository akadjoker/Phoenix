#include "pch.h"
#include "Vertex.hpp"
#include "Driver.hpp"
#include "Frustum.hpp"
#include "Texture.hpp"
#include "Batch.hpp"
#include "Pixmap.hpp"
#include "glad/glad.h"
#include "Terrain.hpp"

PatchInfiniteTerrain::PatchInfiniteTerrain(const std::string &name)
    : Node3D(name),
      m_heightmap(nullptr),
      m_heights(nullptr),
      m_length(1024.0f),
      m_height(20.0f),
      m_grids(256),
      m_patches(16),
      m_material(nullptr)
{
    m_material = new Material();
}

PatchInfiniteTerrain::~PatchInfiniteTerrain()
{
    if (m_heightmap)
        delete m_heightmap;
    if (m_heights)
        delete[] m_heights;

    for (auto &pm : m_patchMeshes)
        delete pm.mesh;
    m_patchMeshes.clear();

    if (m_material)
        delete m_material;
}

bool PatchInfiniteTerrain::LoadHeightmap(const std::string &filename, float patchSize,
                                         float heightScale, int numPatches)
{
    m_heightmap = new Pixmap();
    if (!m_heightmap->Load(filename.c_str()))
    {
        LogError("[PatchInfiniteTerrain] Failed to load: %s", filename.c_str());
        delete m_heightmap;
        m_heightmap = nullptr;
        return false;
    }

    m_grids = m_heightmap->width;
    m_patches = numPatches;
    m_length = patchSize * numPatches;
    m_height = heightScale;
    m_gridLen = m_length / m_grids;
    m_invLen = 1.0f / m_length;

    // Cache heightmap data
    int grids2 = m_grids * m_grids;
    m_heights = new u8[grids2];
    for (int i = 0; i < grids2; i++)
        m_heights[i] = m_heightmap->pixels[i * m_heightmap->components];

    // Build reusable patch meshes
    BuildAllPatches();

    LogInfo("[PatchInfiniteTerrain] Loaded %dx%d heightmap, %d patches (%.1fm each), total=%.1fm",
            m_grids, m_grids, m_patches, patchSize, m_length);

    return true;
}

void PatchInfiniteTerrain::BuildAllPatches()
{
    // Limpar meshes existentes
    for (auto &pm : m_patchMeshes)
        delete pm.mesh;
    m_patchMeshes.clear();

    // Criar patches reutilizáveis (ex: 16x16 = 256 meshes)
    m_patchMeshes.reserve(m_patches * m_patches);

    for (int pz = 0; pz < m_patches; pz++)
    {
        for (int px = 0; px < m_patches; px++)
        {
            PatchMesh pm;
            pm.mesh = new MeshBuffer("TerrainPatch");
            pm.refX = px;
            pm.refZ = pz;

            BuildPatchMesh(pm.mesh, px, pz);

            m_patchMeshes.push_back(pm);
        }
    }

    LogInfo("[PatchInfiniteTerrain] Built %d reusable patch meshes", (int)m_patchMeshes.size());
}

void PatchInfiniteTerrain::BuildPatchMesh(MeshBuffer *mesh, int refPatchX, int refPatchZ)
{
    mesh->ClearVertices();
    mesh->ClearIndices();

    const int patchGrid = m_grids / m_patches;
    const float patchLen = m_length / m_patches;
    const float stepXY = m_length / m_grids;
    const float stepUV = 1.0f / m_grids;

    for (int ctY = 0; ctY < patchGrid; ctY++)
    {
        int y = ctY + refPatchZ * patchGrid;
        float yy = y * stepXY - refPatchZ * patchLen;
        int baseX = y * m_grids;
        int baseX2 = (y == m_patches * patchGrid - 1) ? 0 : baseX + m_grids;

        for (int ctX = 0; ctX <= patchGrid; ctX++)
        {
            int x = ctX + refPatchX * patchGrid;
            int heightX = (x == m_grids) ? 0 : x;
            float xx = x * stepXY - refPatchX * patchLen;

            float u = x * stepUV;
            float v = y * stepUV;

            // Vertex 1 (current row)
            float hh = m_heights[baseX + heightX] / 255.0f;
            mesh->AddVertex(xx, hh * m_height, yy, 0, 1, 0, u, v);

            // Vertex 2 (next row)
            v = (y + 1) * stepUV;
            hh = m_heights[baseX2 + heightX] / 255.0f;
            mesh->AddVertex(xx, hh * m_height, yy + stepXY, 0, 1, 0, u, v);
        }

        // Indices para triangle strip
        int rowVerts = (patchGrid + 1) * 2;
        int baseIdx = ctY * rowVerts;

        for (int i = 0; i < rowVerts - 2; i += 2)
        {
            u32 i0 = baseIdx + i;
            u32 i1 = baseIdx + i + 1;
            u32 i2 = baseIdx + i + 2;
            u32 i3 = baseIdx + i + 3;

            mesh->AddFace(i0, i1, i2);
            mesh->AddFace(i1, i3, i2);
        }
    }
    mesh->CalculateNormals(true);
    mesh->Build();
}

float PatchInfiniteTerrain::SampleHeight(int gridX, int gridZ) const
{
    if (!m_heights)
        return 0.0f;

 
    int x = gridX % m_grids;
    int z = gridZ % m_grids;
    if (x < 0)
        x += m_grids;
    if (z < 0)
        z += m_grids;

    int idx = z * m_grids + x;
    return (m_heights[idx] / 255.0f) * m_height;
}

bool PatchInfiniteTerrain::IsClipped(const Camera *cam, const Frustum *frustum, float pX, float pY, float pZ, float size) const
{
    Vec3 center(pX, pY, pZ);
     

    Vec3 min(pX - size, pY - size, pZ - size);
    Vec3 max(pX + size, pY + size, pZ + size);
    return !frustum->intersectsAABB(min, max);
}

void PatchInfiniteTerrain::render(Shader *shader, bool useMaterial)
{
    if (!m_heights)
        return;

    const Camera *camera = Driver::Instance().GetCamera();
    const Frustum *frustum = Driver::Instance().GetFrustum();

    if (!camera || !frustum)
    {
        LogWarning("[TileTerrain] No camera or frustum");
        return;
    }
    Vec3 camPos = camera->getPosition();

    const float halfLen = m_length * 0.5f;
    const float patchLen = m_length / m_patches;
    const float patchLen2 = patchLen * 0.5f;
    const float invPatchLen = 1.0f / patchLen;

    int px = (int)floorf((camPos.x + halfLen) * invPatchLen);
    int py = (int)floorf((camPos.z + halfLen) * invPatchLen);

    // Raio de visão DIRECCIONAL
    float farPlane = camera->getFar();
    int dd = (int)(farPlane * invPatchLen + 1);
    dd = std::min(dd, m_patches * 2); // Limitar

    // Raio menor atrás da câmera
    Vec3 camDir = camera->getDirection(); // Direção da câmera
    int ddFront = dd;                     // Patches à frente
    int ddBack = dd / 3;                  // Menos patches atrás (33%)
    int ddSide = (dd + ddBack) / 2;       // Médio aos lados

    // LogInfo("[TileTerrain] px=%d py=%d dd=%d", ddFront, ddBack, ddSide);

    // Frustum culling size
    float patchSizeT = patchLen2 * (M_SQRT2 * 1.1f);
    float patchSizeH = m_height * (M_SQRT2 * 1.1f);
    float patchSize = std::max(patchSizeT, patchSizeH);

    // Material
    if (useMaterial && m_material)
    {
        const u8 layers = m_material->GetLayers();
        for (u8 i = 0; i < layers; i++)
        {
            const Texture *tex = m_material->GetTexture(i);
            if (tex)
                tex->Bind(i);
        }
    }

    const Mat4 &baseTransform = getWorldTransform();

    // Render patches visíveis com distância direccional
    for (int yy = py - dd; yy <= py + dd; yy++)
    {
        for (int xx = px - dd; xx <= px + dd; xx++)
        {
            // Calcular distância relativa do patch à câmera
            int deltaX = xx - px;
            int deltaZ = yy - py;

            // Determinar se está à frente, atrás ou aos lados
            // Projetar no vetor de direção da câmera
            float projFwd = deltaX * camDir.x + deltaZ * camDir.z;
            float projSide = abs(deltaX * camDir.z - deltaZ * camDir.x);

            // Skip patches baseado na direção
            if (projFwd > ddFront)
                continue; // Muito à frente
            if (projFwd < -ddBack)
                continue; // Muito atrás
            if (projSide > ddSide)
                continue; // Muito ao lado

            float posX = xx * patchLen - halfLen;
            float posZ = yy * patchLen - halfLen;

            if (IsClipped(camera, frustum, posX + patchLen2, 0, posZ + patchLen2, patchSize))
                continue;

            // Calcular índice do patch reutilizável (wrapping)
            int yyR = yy % m_patches;
            if (yyR < 0)
                yyR += m_patches;
            int xxR = xx % m_patches;
            if (xxR < 0)
                xxR += m_patches;

            int patchIdx = yyR * m_patches + xxR;
            MeshBuffer *mesh = m_patchMeshes[patchIdx].mesh;

            // Transform do patch
            Mat4 patchTransform = baseTransform;
            patchTransform.m[12] = baseTransform.m[12] + posX;
            patchTransform.m[14] = baseTransform.m[14] + posZ;

            shader->SetModelMat4(patchTransform.m);

            Driver::Instance().DrawMeshBuffer(
                mesh,
                PrimitiveType::PT_TRIANGLES,
                mesh->GetIndexCount());
            // Driver::Instance().DrawMeshBuffer(
            //     mesh,
            //     PrimitiveType::PT_TRIANGLE_STRIP,
            //     mesh->GetIndexCount());
        }
    }
}

float PatchInfiniteTerrain::GetHeightAt(float worldX, float worldZ) const
{
    if (!m_heights)
        return 0.0f;

    float frac = worldX * m_invLen + 0.5f;
    float x = frac - floorf(frac);
    x *= m_grids;
    int xx = (int)x;
    x -= xx;
    int xx2 = xx + 1;
    if (xx2 >= m_grids)
        xx2 = 0;

    frac = worldZ * m_invLen + 0.5f;
    float z = frac - floorf(frac);
    z *= m_grids;
    int zz = (int)z;
    z -= zz;
    int zz2 = zz + 1;
    if (zz2 >= m_grids)
        zz2 = 0;

    // Bilinear interpolation
    float heightK = m_height / 255.0f;
    float h00 = heightK * m_heights[zz * m_grids + xx];
    float h10 = heightK * m_heights[zz * m_grids + xx2];
    float h01 = heightK * m_heights[zz2 * m_grids + xx];
    float h11 = heightK * m_heights[zz2 * m_grids + xx2];

    // Interpolação triangular (mais precisa)
    float h0;
    if (x + z < 1.0f)
        h0 = h00 + (h10 - h00) * x + (h01 - h00) * z;
    else
        h0 = h11 + (h11 - h10) * (z - 1) + (h11 - h01) * (x - 1);

    return h0;
}


 

InfiniteTerrain::InfiniteTerrain(const std::string &name)
    : Node3D(name), m_patchesPerSide(8), m_verticesPerPatch(65), m_patchWorldSize(512.0f), m_material(nullptr), m_currentFrame(0)
{
    m_material = new Material();
}

InfiniteTerrain::~InfiniteTerrain()
{
    if (m_baseData.heights)
    {
        delete[] m_baseData.heights;
        m_baseData.heights = nullptr;
    }

    for (auto &pair : m_patchCache)
        delete pair.second;
    m_patchCache.clear();

    if (m_material)
    {
        delete m_material;
        m_material = nullptr;
    }
}

void InfiniteTerrain::SetPatchConfig(int patchesPerSide, int verticesPerPatch, float patchSize)
{
    m_patchesPerSide = patchesPerSide;
    m_verticesPerPatch = verticesPerPatch;
    m_patchWorldSize = patchSize;
}

bool InfiniteTerrain::LoadBaseHeightmap(const std::string &filename, float heightScale)
{
    Pixmap heightmap;
    if (!heightmap.Load(filename.c_str()))
    {
        LogError("[InfiniteTerrain] Failed to load heightmap: %s", filename.c_str());
        return false;
    }

    m_baseData.width = heightmap.width;
    m_baseData.height = heightmap.height;
    m_baseData.heightScale = heightScale;

    u32 size = m_baseData.width * m_baseData.height;
    m_baseData.heights = new float[size];

    for (u32 i = 0; i < size; i++)
    {
        u8 heightValue = heightmap.pixels[i * heightmap.components];
        m_baseData.heights[i] = heightValue / 255.0f;
    }

    LogInfo("[InfiniteTerrain] Loaded base heightmap: %ux%u, scale=%.1fm",
            m_baseData.width, m_baseData.height, heightScale);

    return true;
}

// FIXED: Use world coords as unique key
long long InfiniteTerrain::GetPatchKey(int patchX, int patchZ) const
{
    return ((long long)patchX << 32) | (unsigned int)patchZ;
}

float InfiniteTerrain::SampleHeightFromBase(float u, float v) const
{
    if (!m_baseData.heights)
        return 0.0f;

    // Wrap UVs (0-1 repeating)
    u = u - floorf(u);
    v = v - floorf(v);

    float x = u * (m_baseData.width - 1);
    float z = v * (m_baseData.height - 1);

    int x0 = (int)x;
    int z0 = (int)z;
    int x1 = (x0 + 1) % m_baseData.width;
    int z1 = (z0 + 1) % m_baseData.height;

    float fx = x - x0;
    float fz = z - z0;

    // Bilinear interpolation
    float h00 = m_baseData.heights[z0 * m_baseData.width + x0];
    float h10 = m_baseData.heights[z0 * m_baseData.width + x1];
    float h01 = m_baseData.heights[z1 * m_baseData.width + x0];
    float h11 = m_baseData.heights[z1 * m_baseData.width + x1];

    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;

    return (h0 * (1.0f - fz) + h1 * fz) * m_baseData.heightScale;
}

Vec3 InfiniteTerrain::CalculateNormal(float u, float v) const
{
    const float delta = 0.001f;

    float hC = SampleHeightFromBase(u, v);
    float hR = SampleHeightFromBase(u + delta, v);
    float hU = SampleHeightFromBase(u, v + delta);

    float worldDelta = delta * m_patchWorldSize * m_patchesPerSide;

    Vec3 tangentX(worldDelta, hR - hC, 0.0f);
    Vec3 tangentZ(0.0f, hU - hC, worldDelta);

    Vec3 normal = tangentX.cross(tangentZ);
    normal.normalize();

    return normal;
}

void InfiniteTerrain::BuildPatchMeshLOD(int patchX, int patchZ, int lod, PatchMesh *patch)
{
    if (!patch->meshes[lod])
        patch->meshes[lod] = new MeshBuffer("Patch_LOD" + std::to_string(lod));
    
    MeshBuffer *mesh = patch->meshes[lod];
    mesh->ClearVertices();
    mesh->ClearIndices();
    
    int step = 1 << lod;
    int resolution = (m_verticesPerPatch - 1) / step + 1;
    
    float patchWorldX = patchX * m_patchWorldSize;
    float patchWorldZ = patchZ * m_patchWorldSize;
    
    int patchGrid = (m_verticesPerPatch - 1);
    int totalGrids = m_patchesPerSide * patchGrid;
    float stepUV = 1.0f / (float)(totalGrids - 1);
    
    float minHeight = FLT_MAX;
    float maxHeight = -FLT_MAX;
    
    // Build main grid vertices
    for (int z = 0; z < resolution; z++)
    {
        for (int x = 0; x < resolution; x++)
        {
            int globalX = x * step + patchX * patchGrid;
            int globalZ = z * step + patchZ * patchGrid;
            
            int wrappedX = globalX % totalGrids;
            int wrappedZ = globalZ % totalGrids;
            
            float u = wrappedX * stepUV;
            float v = wrappedZ * stepUV;
            
            float localX = (x * step) / (float)patchGrid;
            float localZ = (z * step) / (float)patchGrid;
            float worldX = patchWorldX + localX * m_patchWorldSize;
            float worldZ = patchWorldZ + localZ * m_patchWorldSize;
            
            float height = SampleHeightFromBase(u, v);
            Vec3 normal = CalculateNormal(u, v);
            
            minHeight = std::min(minHeight, height);
            maxHeight = std::max(maxHeight, height);
            
            mesh->AddVertex(worldX, height, worldZ, normal.x, normal.y, normal.z, u, v);
        }
    }
    
    int mainVertexCount = resolution * resolution;
    float skirtDepth =     m_baseData.heightScale * 0.1f;
    
    // Add SKIRT vertices (duplicates of edges, moved down)
    // Left edge (x=0)
    for (int z = 0; z < resolution; z++)
    {
      
        float worldX = patchWorldX;
        float worldZ = patchWorldZ + (z * step) / (float)patchGrid * m_patchWorldSize;
        
        int wrappedX = (patchX * patchGrid) % totalGrids;
        int wrappedZ = (z * step + patchZ * patchGrid) % totalGrids;
        float u = wrappedX * stepUV;
        float v = wrappedZ * stepUV;
        float height = SampleHeightFromBase(u, v) - skirtDepth;
        
        mesh->AddVertex(worldX, height, worldZ, 0, -1, 0, u, v);
    }
    
    int leftSkirtStart = mainVertexCount;
    
    // Right edge (x=resolution-1)
    for (int z = 0; z < resolution; z++)
    {
        float worldX = patchWorldX + m_patchWorldSize;
        float worldZ = patchWorldZ + (z * step) / (float)patchGrid * m_patchWorldSize;
        
        int wrappedX = ((resolution - 1) * step + patchX * patchGrid) % totalGrids;
        int wrappedZ = (z * step + patchZ * patchGrid) % totalGrids;
        float u = wrappedX * stepUV;
        float v = wrappedZ * stepUV;
        float height = SampleHeightFromBase(u, v) - skirtDepth;
        
        mesh->AddVertex(worldX, height, worldZ, 0, -1, 0, u, v);
    }
    
    int rightSkirtStart = leftSkirtStart + resolution;
    
    // Top edge (z=0)
    for (int x = 0; x < resolution; x++)
    {
        float worldX = patchWorldX + (x * step) / (float)patchGrid * m_patchWorldSize;
        float worldZ = patchWorldZ;
        
        int wrappedX = (x * step + patchX * patchGrid) % totalGrids;
        int wrappedZ = (patchZ * patchGrid) % totalGrids;
        float u = wrappedX * stepUV;
        float v = wrappedZ * stepUV;
        float height = SampleHeightFromBase(u, v) - skirtDepth;
        
        mesh->AddVertex(worldX, height, worldZ, 0, -1, 0, u, v);
    }
    
    int topSkirtStart = rightSkirtStart + resolution;
    
    // Bottom edge (z=resolution-1)
    for (int x = 0; x < resolution; x++)
    {
        float worldX = patchWorldX + (x * step) / (float)patchGrid * m_patchWorldSize;
        float worldZ = patchWorldZ + m_patchWorldSize;
        
        int wrappedX = (x * step + patchX * patchGrid) % totalGrids;
        int wrappedZ = ((resolution - 1) * step + patchZ * patchGrid) % totalGrids;
        float u = wrappedX * stepUV;
        float v = wrappedZ * stepUV;
        float height = SampleHeightFromBase(u, v) - skirtDepth;
        
        mesh->AddVertex(worldX, height, worldZ, 0, -1, 0, u, v);
    }
    
    int bottomSkirtStart = topSkirtStart + resolution;
    
    // Build main grid indices
    for (int z = 0; z < resolution - 1; z++)
    {
        for (int x = 0; x < resolution - 1; x++)
        {
            u32 i0 = z * resolution + x;
            u32 i1 = z * resolution + (x + 1);
            u32 i2 = (z + 1) * resolution + (x + 1);
            u32 i3 = (z + 1) * resolution + x;
            
            mesh->AddFace(i0, i2, i1);
            mesh->AddFace(i0, i3, i2);
        }
    }
    
    // Connect skirts - Left
    for (int z = 0; z < resolution - 1; z++)
    {
        u32 top = z * resolution;
        u32 bottom = (z + 1) * resolution;
        u32 skirtTop = leftSkirtStart + z;
        u32 skirtBottom = leftSkirtStart + z + 1;
        
        mesh->AddFace(top, skirtTop, bottom);
        mesh->AddFace(skirtTop, skirtBottom, bottom);
    }
    
    // Connect skirts - Right
    for (int z = 0; z < resolution - 1; z++)
    {
        u32 top = z * resolution + (resolution - 1);
        u32 bottom = (z + 1) * resolution + (resolution - 1);
        u32 skirtTop = rightSkirtStart + z;
        u32 skirtBottom = rightSkirtStart + z + 1;
        
        mesh->AddFace(top, bottom, skirtTop);
        mesh->AddFace(skirtTop, bottom, skirtBottom);
    }
    
    // Connect skirts - Top
    for (int x = 0; x < resolution - 1; x++)
    {
        u32 left = x;
        u32 right = x + 1;
        u32 skirtLeft = topSkirtStart + x;
        u32 skirtRight = topSkirtStart + x + 1;
        
        mesh->AddFace(left, right, skirtLeft);
        mesh->AddFace(skirtLeft, right, skirtRight);
    }
    
    // Connect skirts - Bottom
    for (int x = 0; x < resolution - 1; x++)
    {
        u32 left = (resolution - 1) * resolution + x;
        u32 right = (resolution - 1) * resolution + x + 1;
        u32 skirtLeft = bottomSkirtStart + x;
        u32 skirtRight = bottomSkirtStart + x + 1;
        
        mesh->AddFace(left, skirtLeft, right);
        mesh->AddFace(skirtLeft, skirtRight, right);
    }
    
    patch->bounds.min = Vec3(patchWorldX, minHeight, patchWorldZ);
    patch->bounds.max = Vec3(patchWorldX + m_patchWorldSize, maxHeight, patchWorldZ + m_patchWorldSize);
    
    mesh->CalculateBoundingBox();
    
    mesh->Build();
}

 


 


void InfiniteTerrain::CleanOldPatches()
{
    if (m_patchCache.size() <= MAX_CACHED_PATCHES)
        return;

    std::vector<std::pair<long long, u32>> ages;
    ages.reserve(m_patchCache.size());

    for (const auto &pair : m_patchCache)
        ages.push_back({pair.first, pair.second->lastAccessFrame});

    std::sort(ages.begin(), ages.end(),
              [](const auto &a, const auto &b)
              { return a.second < b.second; });

    int toRemove = (int)m_patchCache.size() - MAX_CACHED_PATCHES;
    for (int i = 0; i < toRemove; i++)
    {
        auto it = m_patchCache.find(ages[i].first);
        if (it != m_patchCache.end())
        {
            delete it->second;
            m_patchCache.erase(it);
        }
    }
}

int InfiniteTerrain::CalculateLODWithHysteresis(float distSq, int patchX, int patchZ) const
{
    const Camera *camera = Driver::Instance().GetCamera();
    if (!camera) return 0;
    
    float farPlane = camera->getFar();
    float dist = sqrtf(distSq);
    
    // Ranges baseados no far plane
    float lod0Max = farPlane * 0.40f;
    float lod1Max = farPlane * 0.65f;
    float lod2Max = farPlane * 0.80f;
    
    // HYSTERESIS: Grande margem para evitar flip-flop
    const float HYSTERESIS = 100.0f; // 100 metros de margem
    
    // Verificar LOD anterior
    long long key = GetPatchKey(patchX, patchZ);
    auto it = m_patchCache.find(key);
    int prevLOD = -1;
    
    if (it != m_patchCache.end())
    {
        for (int i = 0; i < MAX_LOD_LEVELS; i++)
        {
            if (it->second->meshes[i])
            {
                prevLOD = i;
                break;
            }
        }
    }
    
    // Calcular novo LOD
    int newLOD;
    if (dist < lod0Max)
        newLOD = 0;
    else if (dist < lod1Max)
        newLOD = 1;
    else if (dist < lod2Max)
        newLOD = 2;
    else
        newLOD = 3;
    
    // Se já tem LOD, só muda se a diferença for grande
    if (prevLOD >= 0)
    {
        // Ficando mais perto (melhor qualidade)
        if (newLOD < prevLOD)
        {
            float threshold = (prevLOD == 1) ? lod0Max :
                            (prevLOD == 2) ? lod1Max : lod2Max;
            
            if (dist > threshold - HYSTERESIS)
                newLOD = prevLOD; // Mantém LOD atual
        }
        // Ficando mais longe (pior qualidade)
        else if (newLOD > prevLOD)
        {
            float threshold = (newLOD == 1) ? lod0Max :
                            (newLOD == 2) ? lod1Max : lod2Max;
            
            if (dist < threshold + HYSTERESIS)
                newLOD = prevLOD; // Mantém LOD atual
        }
    }
    
    return newLOD;
} 
InfiniteTerrain::PatchMesh *InfiniteTerrain::GetOrCreatePatch(int patchX, int patchZ, int lod)
{
    lod = std::max(0, std::min(lod, MAX_LOD_LEVELS - 1));

    long long key = GetPatchKey(patchX, patchZ);

    auto it = m_patchCache.find(key);
    if (it != m_patchCache.end())
    {
        it->second->lastAccessFrame = m_currentFrame;

        if (it->second->meshes[lod] == nullptr)
            BuildPatchMeshLOD(patchX, patchZ, lod, it->second);

        return it->second;
    }

    // Create new patch
    PatchMesh *patch = new PatchMesh();
    patch->lastAccessFrame = m_currentFrame;
    patch->worldX = patchX;
    patch->worldZ = patchZ;

    BuildPatchMeshLOD(patchX, patchZ, lod, patch);

    m_patchCache[key] = patch;

    if (m_patchCache.size() > MAX_CACHED_PATCHES)
        CleanOldPatches();

    return patch;
}

bool InfiniteTerrain::IsPatchVisible(int patchX, int patchZ, const Frustum *frustum) const
{
    float patchWorldX = patchX * m_patchWorldSize;
    float patchWorldZ = patchZ * m_patchWorldSize;

    // AABB mais preciso que sphere
    Vec3 min(
        patchWorldX,
        0.0f,   
        patchWorldZ
    );
    
    Vec3 max(
        patchWorldX + m_patchWorldSize,
        m_baseData.heightScale,  // Altura máxima possível
        patchWorldZ + m_patchWorldSize
    );

    return frustum->intersectsAABB(min, max);
}

void InfiniteTerrain::render(Shader *shader, bool useMaterial)
{
    if (!m_baseData.heights)
        return;

    const Camera *camera = Driver::Instance().GetCamera();
    const Frustum *frustum = Driver::Instance().GetFrustum();

    if (!camera || !frustum)
        return;

    m_currentFrame++;

    Vec3 camPos = camera->getPosition();
    Vec3 camDir = camera->getDirection();

    int centerPatchX = (int)floorf(camPos.x / m_patchWorldSize);
    int centerPatchZ = (int)floorf(camPos.z / m_patchWorldSize);

    float farPlane = camera->getFar();
    int viewRadius = (int)(farPlane / m_patchWorldSize) + 2;
    viewRadius = std::min(viewRadius, 32);

    // Directional rendering
    int ddFront = viewRadius;
    int ddBack = viewRadius / 3;
    int ddSide = (viewRadius + ddBack) / 2;

    const Mat4 &mat = getWorldTransform();
    shader->SetModelMat4(mat.m);

    if (useMaterial && m_material)
    {
        const u8 layers = m_material->GetLayers();
        for (u8 i = 0; i < layers; i++)
        {
            const Texture *tex = m_material->GetTexture(i);
            if (tex)
                tex->Bind(i);
        }
    }

    int rendered = 0;
    int lodCounts[MAX_LOD_LEVELS] = {0};

    for (int pz = centerPatchZ - viewRadius; pz <= centerPatchZ + viewRadius; pz++)
    {
        for (int px = centerPatchX - viewRadius; px <= centerPatchX + viewRadius; px++)
        {
            // Directional culling
            int deltaX = px - centerPatchX;
            int deltaZ = pz - centerPatchZ;

            float projFwd = deltaX * camDir.x + deltaZ * camDir.z;
            float projSide = abs(deltaX * camDir.z - deltaZ * camDir.x);

            if (projFwd > ddFront || projFwd < -ddBack || projSide > ddSide)
                continue;

            // Frustum culling
            if (!IsPatchVisible(px, pz, frustum))
                continue;

            float patchCenterX = px * m_patchWorldSize + m_patchWorldSize * 0.5f;
            float patchCenterZ = pz * m_patchWorldSize + m_patchWorldSize * 0.5f;

            float dx = patchCenterX - camPos.x;
            float dz = patchCenterZ - camPos.z;
            float distSq = dx * dx + dz * dz;

            int lod =  CalculateLODWithHysteresis(distSq, px, pz);
            

            PatchMesh *patch = GetOrCreatePatch(px, pz, lod);

            if (patch && patch->meshes[lod])
            {
                MeshBuffer *mesh = patch->meshes[lod];
                Driver::Instance().DrawMeshBuffer(
                    mesh,
                    PrimitiveType::PT_TRIANGLES,
                    mesh->GetIndexCount());
                rendered++;
                lodCounts[lod]++;
            }
        }
    }

    if (m_currentFrame % 60 == 0)
    {
        LogInfo("[InfiniteTerrain] Rendered:%d | LOD:[%d,%d,%d,%d] | Cache:%zu",
                rendered, lodCounts[0], lodCounts[1], lodCounts[2], lodCounts[3], m_patchCache.size());
    }
}

void InfiniteTerrain::debug(RenderBatch *batch)
{
    const Camera *camera = Driver::Instance().GetCamera();
    if (!camera)
        return;

    Vec3 camPos = camera->getPosition();
    int centerPatchX = (int)floorf(camPos.x / m_patchWorldSize);
    int centerPatchZ = (int)floorf(camPos.z / m_patchWorldSize);

    int viewRadius = 8;

    for (int pz = centerPatchZ - viewRadius; pz <= centerPatchZ + viewRadius; pz++)
    {
        for (int px = centerPatchX - viewRadius; px <= centerPatchX + viewRadius; px++)
        {
            long long key = GetPatchKey(px, pz);
            auto it = m_patchCache.find(key);

            BoundingBox box;
            if (it != m_patchCache.end())
            {
                box = it->second->bounds;

                int cachedLOD = -1;
                for (int i = 0; i < MAX_LOD_LEVELS; i++)
                {
                    if (it->second->meshes[i])
                        cachedLOD = i;
                }

                switch (cachedLOD)
                {
                case 0:
                    batch->SetColor(0, 255, 0);
                    break;
                case 1:
                    batch->SetColor(255, 255, 0);
                    break;
                case 2:
                    batch->SetColor(255, 128, 0);
                    break;
                case 3:
                    batch->SetColor(255, 0, 0);
                    break;
                default:
                    batch->SetColor(128, 128, 128);
                    break;
                }
            }
            else
            {
                float patchWorldX = px * m_patchWorldSize;
                float patchWorldZ = pz * m_patchWorldSize;
                box.min = Vec3(patchWorldX, 0, patchWorldZ);
                box.max = Vec3(patchWorldX + m_patchWorldSize, m_baseData.heightScale, patchWorldZ + m_patchWorldSize);
                batch->SetColor(100, 100, 100);
            }

            batch->Box(box);
        }
    }
}

float InfiniteTerrain::GetHeightAt(float worldX, float worldZ) const
{
    if (!m_baseData.heights)
        return 0.0f;

    float totalWorldSize = m_patchesPerSide * m_patchWorldSize;
    float u = worldX / totalWorldSize;
    float v = worldZ / totalWorldSize;

    return SampleHeightFromBase(u, v);
}

Vec3 InfiniteTerrain::GetNormalAt(float worldX, float worldZ) const
{
    float totalWorldSize = m_patchesPerSide * m_patchWorldSize;
    float u = worldX / totalWorldSize;
    float v = worldZ / totalWorldSize;

    return CalculateNormal(u, v);
}