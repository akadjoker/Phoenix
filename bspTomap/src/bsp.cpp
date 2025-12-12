
#include "bsp.hpp"


struct MapHeader
{
    u32 magic;         // 'M','A','P','2' 
    u32 version;       // 1
    u32 numTextures;
    u32 numLightmaps;
    u32 numSurfaces;
};

struct MapSurfaceHeader
{
    u32 textureIndex;   // índice em textures[]
    u32 lightmapIndex;  // índice em lightmaps[] ou 0xFFFFFFFF se não tiver
    u32 vertexCount;
    u32 indexCount;
};

struct MapVertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 uv0;  // diffuse
    Vec2 uv1;  // lightmap
};

// binary layout: 3f + 3f + 2f + 2f = 40 bytes


void BSP::loadTexture(FileStream &file)
{

    NumTextures = lumps[kTextures].length / sizeof(BSPTexture);
    Textures = new BSPTexture[NumTextures];
    file.Seek(lumps[kTextures].offset, SeekOrigin::Begin);
    file.Read(&Textures[0], lumps[kTextures].length);

    for (int i = 0; i < NumTextures; ++i)
    {

        char path[1024];
        snprintf(path, sizeof(path), "%s", Textures[i].strName);
        textures.push_back(path);
    }
}

void BSP::loadLightmap(FileStream &file)
{
    NumLightMaps = lumps[kLightmaps].length / sizeof(BSPLightmap);

    LightMaps = new BSPLightmap[NumLightMaps];
    file.Seek(lumps[kLightmaps].offset, SeekOrigin::Begin);
    file.Read(&LightMaps[0], lumps[kLightmaps].length);

    LogInfo(" %d", NumLightMaps);
    //lightmaps.reserve(NumLightMaps);
    for (int i = 0; i < NumLightMaps; ++i)
    {

        Pixmap pix(128, 128, 4);

        for (int y = 0; y < 128; ++y)
        {
            for (int x = 0; x < 128; ++x)
            {
                u8 r = LightMaps[i].imageBits[y][x][0];
                u8 g = LightMaps[i].imageBits[y][x][1];
                u8 b = LightMaps[i].imageBits[y][x][2];
                pix.SetPixel(x, y, r, g, b, 255);
            }
        }
        lightmaps.push_back(Utils::TextFormat("lightmap%d.png", i));
        pix.Save(Utils::TextFormat("lightmap%d.png", i));
    }
}

void BSP::loadVertex(FileStream &file)
{
    NumVertices = lumps[kVertices].length / sizeof(BSPVertex);

    Vertices = new BSPVertex[NumVertices];
    file.Seek(lumps[kVertices].offset, SeekOrigin::Begin);
    file.Read(&Vertices[0], lumps[kVertices].length);
}

void BSP::loadFaces(FileStream &file)
{
    NumFaces = lumps[kFaces].length / sizeof(BSPFace);

    Faces = new BSPFace[NumFaces];
    file.Seek(lumps[kFaces].offset, SeekOrigin::Begin);
    file.Read(&Faces[0], lumps[kFaces].length);
}

void BSP::loadIndex(FileStream &file)
{
    NumIndices = lumps[kIndices].length / sizeof(u16);

    Indices = new s32[NumIndices];
    file.Seek(lumps[kIndices].offset, SeekOrigin::Begin);
    file.Read(&Indices[0], lumps[kIndices].length);
}

void BSP::LoadEntities(FileStream &file)
{
    NumEntities = lumps[kEntities].length / sizeof(u8);
    Entities.reserve(lumps[kEntities].length + 2);
    Entities[lumps[kEntities].length + 1] = '\0';

    file.Seek(lumps[kEntities].offset, SeekOrigin::Begin);
    file.Read(&Entities[0], lumps[kEntities].length);

    if (Entities.empty())
        return;

    char *buffer = new char[lumps[kEntities].length + 1];
    std::copy(Entities.begin(), Entities.end(), buffer);

    LogInfo("Entities: %s", buffer);

    //  SaveFileText("entities.txt", buffer);
    delete[] buffer;

    // for (auto i = 0; i < lumps[kEntities].length; i++)
    // {
    //     printf("%c", Entities[i]);
    // }
}

void BSP::loadModels(FileStream &file)
{
    NumModels = lumps[kModels].length / sizeof(BSPModel);

    Models = new BSPModel[NumModels];
    file.Seek(lumps[kModels].offset, SeekOrigin::Begin);
    file.Read(&Models[0], lumps[kModels].length);
}

bool BSP::loadFromFile(const std::string &filePath)
{
    FileStream file;
    if (!file.Open(filePath.c_str()))
        return false;

    file.Read(&header, sizeof(BSPHeader));

    LogInfo("BSP version: %d", header.version);
    // LogInfo("BSP ID: %d", header.strID);

    file.Read(&lumps, sizeof(BSPLump) * kMaxLumps);

    loadTexture(file);
    loadLightmap(file);
    loadVertex(file);
    loadFaces(file);
    loadIndex(file);
    LoadEntities(file);
    loadModels(file);

    BuildSurfaces();
    MergeSurfacesByMaterial();

    return true;
}

bool BSP::saveToFile(const std::string& filePath)
{
    // Garante que já tens Surfaces construídas e merged
    if (Surfaces.empty())
        BuildSurfaces();
    if (mergedSurfaces.empty())
        MergeSurfacesByMaterial();

    FileStream out(filePath, "wb");
    if (!out.IsOpen())
        return false;

 // HEADER
    out.WriteUInt(0x4D415032); // "MAP2"
    out.WriteUInt(1);          // versão

        // 1) TEXTURAS
    u32 numTextures  = static_cast<u32>(textures.size());
  
 
    out.WriteUInt(numTextures);
    for (const std::string& texName : textures)
    {
    
        out.WriteUTF(texName.c_str());
    }

 // 2) LIGHTMAPS
    out.WriteUInt(NumLightMaps);

    out.WriteUInt((u32)mergedSurfaces.size());

    
    for (const BSPSurface& s : mergedSurfaces)
    {
        u32 textureIndex  = static_cast<u32>(s.textureID);
        u32 lightmapIndex = static_cast<u32>(s.lightmapID);
                          

        u32 vertexCount   = static_cast<u32>(s.vertices.size());
        u32 indexCount    = static_cast<u32>(s.indices.size());

        
        out.WriteUInt(textureIndex);
        out.WriteUInt(lightmapIndex);


        // Vertices
        out.WriteUInt(vertexCount);
        
        for (u32 i = 0; i < vertexCount; ++i)
        {
            const Vec3& p  = s.vertices[i];
           // const Vec3& n  = s.normals[i];
          //  const Color& c = s.colors[i];
            const Vec2& uv0 = s.uv0[i];
            const Vec2& uv1 = s.uv1[i];

            out.WriteFloat(p.x);
            out.WriteFloat(p.y);
            out.WriteFloat(p.z);

            // out.WriteByte(c.r);
            // out.WriteByte(c.g);
            // out.WriteByte(c.b);
            // out.WriteByte(c.a);
            
            // out.WriteFloat(n.x);
            // out.WriteFloat(n.y);
            // out.WriteFloat(n.z);
            
            out.WriteFloat(uv0.x);
            out.WriteFloat(uv0.y);
            
            out.WriteFloat(uv1.x);
            out.WriteFloat(uv1.y);
        }
        
        // Índices (u32)
        out.WriteUInt(indexCount);
        for (u32 i = 0; i < indexCount/3; ++i)
        {
            u32 idx0 = s.indices[i * 3 + 0];
            u32 idx1 = s.indices[i * 3 + 1];
            u32 idx2 = s.indices[i * 3 + 2];
            
            out.WriteUInt(idx0);
            out.WriteUInt(idx1);
            out.WriteUInt(idx2);
        }
    }

    out.Close();
    return true;
}




BSP::BSP()
{
}

BSP::~BSP()

{
    delete[] Textures;
    Textures = 0;
    delete[] LightMaps;
    LightMaps = 0;
    delete[] Vertices;
    Vertices = 0;
    delete[] Faces;
    Faces = 0;
    delete[] Models;
    Models = 0;
    delete[] Planes;
    Planes = 0;
    delete[] Nodes;
    Nodes = 0;
    delete[] Leafs;
    Leafs = 0;
    delete[] LeafFaces;
    LeafFaces = 0;
    delete[] MeshVerts;
    MeshVerts = 0;
    delete[] Brushes;
    Brushes = 0;
    delete[] Indices;
    Indices = 0;
}

void BSP::BuildSurfaces()
{
    Surfaces.clear();
    Surfaces.reserve(NumFaces);

    // Primeiro: construir todas as superfícies individuais
    for (int i = 0; i < NumFaces; i++)
    {
        BSPFace face = Faces[i];

        s32 textureID = face.textureID;

        if (textureID < 0 || textureID >= NumTextures)
        {
            LogWarning("Invalid textureID %d for face %d", textureID, i);
            continue;
        }

        Surfaces.push_back(BSPSurface());
        BSPSurface &surface = Surfaces.back();
        surface.textureID = textureID;

        if (face.lightmapID >= 0 && face.lightmapID < NumLightMaps)
        {
            surface.lightmapID = face.lightmapID;
        }
        else
        {
            surface.lightmapID = -1;
        }

        // Carregar vértices

        for (s32 v = face.startVertIndex; v < face.startVertIndex + face.numOfVerts; v++)
        {
            if (v >= NumVertices)
            {
                LogWarning("Vertex index %d out of bounds", v);
                continue;
            }

            BSPVertex vertex = Vertices[v];
            surface.vertices.push_back((Vec3){vertex.vPosition.x * scale,
                                              vertex.vPosition.z * scale,
                                              vertex.vPosition.y * scale});
            surface.normals.push_back((Vec3){vertex.vNormal.x * scale, vertex.vNormal.z * scale, vertex.vNormal.y * scale});

            surface.uv0.push_back(vertex.vTextureCoord);
            surface.uv1.push_back(vertex.vLightmapCoord);
        }

        if (face.type == 1) // Polygon
        {
            ProcessPolygonFace(face, surface);
        }
        else if (face.type == 2) // Patch
        {
            ProcessBezierPatch(face, surface);
        }
        else if (face.type == 3) // Mesh
        {
            ProcessMeshFace(face, surface);
        }
        else
        {
            LogInfo("Face type %d not supported, treating as simple mesh", face.type);
        }
        //  surface.init();
    }
}

void BSP::MergeSurfacesByMaterial()
{
    if (Surfaces.empty())
        return;

    // Criar um mapa para agrupar por textureID e lightmapID
    std::map<std::pair<int, int>, std::vector<int>> materialGroups;

    // Agrupar superfícies por material
    for (size_t i = 0; i < Surfaces.size(); i++)
    {
        std::pair<int, int> materialKey = {Surfaces[i].textureID,
                                           Surfaces[i].lightmapID};
        materialGroups[materialKey].push_back(i);
    }

    // Limpar superfícies merged anteriores
    mergedSurfaces.clear();
    mergedSurfaces.reserve(materialGroups.size());

    // Merge para cada grupo de material
    for (const auto &group : materialGroups)
    {
        BSPSurface mergedSurface;
        mergedSurface.textureID = group.first.first;   // textureID
        mergedSurface.lightmapID = group.first.second; // lightmapID

        // Calcular tamanho total necessário para otimização
        size_t totalVertices = 0;
        size_t totalIndices = 0;
        for (int surfaceIndex : group.second)
        {
            totalVertices += Surfaces[surfaceIndex].vertices.size();
            totalIndices += Surfaces[surfaceIndex].indices.size();
        }

        // Reservar espaço
        mergedSurface.vertices.reserve(totalVertices);
        mergedSurface.normals.reserve(totalVertices);
        mergedSurface.uv0.reserve(totalVertices);
        mergedSurface.uv1.reserve(totalVertices);
        mergedSurface.indices.reserve(totalIndices);
        mergedSurface.colors.reserve(totalVertices);

        // Merge todas as superfícies do grupo
        for (int surfaceIndex : group.second)
        {
            const BSPSurface &surface = Surfaces[surfaceIndex];
            int vertexOffset = mergedSurface.vertices.size();

            // Adicionar vértices
            for (size_t i = 0; i < surface.vertices.size(); i++)
            {
                mergedSurface.vertices.push_back(surface.vertices[i]);
                mergedSurface.normals.push_back(surface.normals[i]);
                mergedSurface.uv0.push_back(surface.uv0[i]);
                mergedSurface.uv1.push_back(surface.uv1[i]);
                mergedSurface.colors.push_back(surface.colors[i]);
            }

            for (size_t i = 0; i < surface.indices.size(); i++)
            {
                mergedSurface.indices.push_back(surface.indices[i] + vertexOffset);
            }
        }

        mergedSurfaces.push_back(mergedSurface);

        LogInfo("Merged %d surfaces with texture %d, lightmap %d (%d "
                "vertices, %d indices)",
                (int)group.second.size(), mergedSurface.textureID,
                mergedSurface.lightmapID, (int)mergedSurface.vertices.size(),
                (int)mergedSurface.indices.size());
    }
}

// Tipo 1: Polygon - face plana triangulada
bool BSP::ProcessPolygonFace(const BSPFace &face, BSPSurface &surface)
{
    if (face.numOfIndices % 3 != 0)
    {
        LogWarning("Polygon face has invalid number of indices: %d",
                   face.numOfIndices);
        return false;
    }

    for (s32 j = 0; j < face.numOfIndices; j += 3)
    {
        if (face.startIndex + j + 2 >= NumIndices)
        {
            LogWarning("Polygon index out of bounds");
            break;
        }

        surface.indices.push_back(Indices[face.startIndex + j + 0]);
        surface.indices.push_back(Indices[face.startIndex + j + 1]);
        surface.indices.push_back(Indices[face.startIndex + j + 2]);
    }

    return true;
}

// Tipo 3: Mesh -
bool BSP::ProcessMeshFace(const BSPFace &face, BSPSurface &surface)
{
    if (face.numOfIndices % 3 != 0)
    {
        LogWarning("Mesh face has invalid number of indices: %d",
                   face.numOfIndices);
        return false;
    }

    // Para meshes, os índices são absolutos (não relativos à face)
    for (s32 j = 0; j < face.numOfIndices; j += 3)
    {
        if (face.startIndex + j + 2 >= NumIndices)
        {
            LogWarning("Mesh index out of bounds");
            break;
        }

        // Ajustar índices para serem relativos aos vértices desta superfície
        int idx0 = Indices[face.startIndex + j + 0] - face.startVertIndex;
        int idx1 = Indices[face.startIndex + j + 1] - face.startVertIndex;
        int idx2 = Indices[face.startIndex + j + 2] - face.startVertIndex;

        // Validar se os índices estão dentro do range
        if (idx0 >= 0 && idx0 < face.numOfVerts && idx1 >= 0 && idx1 < face.numOfVerts && idx2 >= 0 && idx2 < face.numOfVerts)
        {
            surface.indices.push_back(idx0);
            surface.indices.push_back(idx1);
            surface.indices.push_back(idx2);
        }
        else
        {
            LogWarning("Mesh indices out of vertex range");
        }
    }

    return true;
}

// Tipo 4: Billboard - quad sempre virado para a câmera
bool BSP::ProcessBillboardFace(const BSPFace &face, BSPSurface &surface)
{
    if (face.numOfVerts != 4)
    {
        LogWarning("Billboard must have exactly 4 vertices, has %d",
                   face.numOfVerts);
        return false;
    }

    // Billboard é um quad, precisa de 2 triângulos (6 índices)
    // Assumindo que os vértices estão em ordem: bottom-left, bottom-right,
    // top-right, top-left
    surface.indices.push_back(0); // bottom-left
    surface.indices.push_back(1); // bottom-right
    surface.indices.push_back(2); // top-right

    surface.indices.push_back(0); // bottom-left
    surface.indices.push_back(2); // top-right
    surface.indices.push_back(3); // top-left

    // Marcar como billboard para tratamento especial no shader
    // surface.isBillboard = true;

    return true;
}

bool BSP::ProcessBezierPatch(const BSPFace &face, BSPSurface &surface)
{

    int controlWidth = face.size[0];
    int controlHeight = face.size[1];

    if (controlWidth == 0 || controlHeight == 0)
        return false;

    int biquadWidth = (controlWidth - 1) / 2;
    int biquadHeight = (controlHeight - 1) / 2;

    std::vector<Vertex2TCoords> controlPoint;
    controlPoint.reserve(controlWidth * controlHeight);

    for (int i = 0; i < controlWidth * controlHeight; ++i)
    {
        controlPoint.push_back(Vertices[face.startVertIndex + i]);
    }

    Bezier.Patch = new BSPSurface();

    for (int j = 0; j < biquadHeight; ++j)
    {
        for (int k = 0; k < biquadWidth; ++k)
        {
            const s32 inx = j * controlWidth * 2 + k * 2;

            // setup bezier control points for this patch
            Bezier.control[0] = controlPoint[inx + 0];
            Bezier.control[1] = controlPoint[inx + 1];
            Bezier.control[2] = controlPoint[inx + 2];
            Bezier.control[3] = controlPoint[inx + controlWidth + 0];
            Bezier.control[4] = controlPoint[inx + controlWidth + 1];
            Bezier.control[5] = controlPoint[inx + controlWidth + 2];
            Bezier.control[6] = controlPoint[inx + controlWidth * 2 + 0];
            Bezier.control[7] = controlPoint[inx + controlWidth * 2 + 1];
            Bezier.control[8] = controlPoint[inx + controlWidth * 2 + 2];

            Bezier.tesselate(5, scale);
        }
    }

    const u32 bsize = Bezier.Patch->vertices.size();
    const u32 msize = surface.vertices.size();

    surface.vertices.reserve(msize + bsize);
    surface.normals.reserve(msize + bsize);
    surface.uv0.reserve(msize + bsize);
    surface.uv1.reserve(msize + bsize);
    surface.colors.reserve(msize + bsize);

    for (u32 i = 0; i != bsize; ++i)
    {
        surface.vertices.push_back(Bezier.Patch->vertices[i]);
        surface.normals.push_back(Bezier.Patch->normals[i]);
        surface.uv0.push_back(Bezier.Patch->uv0[i]);
        surface.uv1.push_back(Bezier.Patch->uv1[i]);
        surface.colors.push_back(Bezier.Patch->colors[i]);
    }

    surface.indices.reserve(surface.indices.size() + Bezier.Patch->indices.size());

    for (u32 i = 0; i != Bezier.Patch->indices.size(); ++i)
    {
        surface.indices.push_back(msize + Bezier.Patch->indices[i]);
    }

    delete Bezier.Patch;
    Bezier.Patch = nullptr;

    return true;
}

void BSP::SBezier::tesselate(s32 level, float scale)
{
    // Calculate how many vertices across/down there are
    s32 j, k;

    column[0].reserve(level + 1);
    column[1].reserve(level + 1);
    column[2].reserve(level + 1);

    const double w = 0.0 + (1.0 / (double)level);

    // Tesselate along the columns
    for (j = 0; j <= level; ++j)
    {
        const double f = w * (double)j;

        column[0][j] = Interpolated_quadratic(control[0], control[3], control[6], f);
        column[1][j] = Interpolated_quadratic(control[1], control[4], control[7], f);
        column[2][j] = Interpolated_quadratic(control[2], control[5], control[8], f);
    }

    const u32 idx = Patch->vertices.size();
    Patch->vertices.reserve(idx + level * level);
    Patch->normals.reserve(idx + level * level);
    Patch->uv0.reserve(idx + level * level);
    Patch->uv1.reserve(idx + level * level);
    Patch->colors.reserve(idx + level * level);
    

    Vertex2TCoords v;
    Vertex2TCoords f;
    for (j = 0; j <= level; ++j)
    {
        for (k = 0; k <= level; ++k)
        {
            f = Interpolated_quadratic(column[0][j], column[1][j], column[2][j], w * (double)k);
            f.copy(v);

            Vec3 pos;

            pos.x = v.position.x * scale;
            pos.y = v.position.y * scale;
            pos.z = v.position.z * scale;


            Vec3 norm;

            norm.x = v.normal.x * scale;
            norm.y = v.normal.y * scale;
            norm.z = v.normal.z * scale;

            Color col;

            col.r = v.color.r;
            col.g = v.color.g;
            col.b = v.color.b;
            col.a = v.color.a;

            Patch->colors.push_back(col);
            Patch->vertices.push_back(pos);
            Patch->normals.push_back(norm);
            Patch->uv0.push_back(v.uv0);
            Patch->uv1.push_back(v.uv1);
        }
    }

    Patch->indices.reserve(Patch->indices.size() + 6 * level * level);

    for (j = 0; j < level; ++j)
    {
        for (k = 0; k < level; ++k)
        {
            const s32 inx = idx + (k * (level + 1)) + j;

            Patch->indices.push_back(inx + 0);
            Patch->indices.push_back(inx + (level + 1) + 0);
            Patch->indices.push_back(inx + (level + 1) + 1);

            Patch->indices.push_back(inx + 0);
            Patch->indices.push_back(inx + (level + 1) + 1);
            Patch->indices.push_back(inx + 1);
        }
    }
}

inline double Clamp(double val, double min, double max)
{
    return val < min ? min : (val > max ? max : val);
}

inline float GetValue(unsigned char val)
{
    return val / 255.0f;
}

inline unsigned char ToByte(float val)
{
    return (unsigned char)Clamp(val * 255.0f, 0.0f, 255.0f);
}

static Vec3 Interpolated_vec3_quadratic(const Vec3 &v1, const Vec3 &v2,
                                        const Vec3 &v3, double d)
{

    const double inv = 1.0f - d;
    const double mul0 = inv * inv;
    const double mul1 = 2.0f * d * inv;
    const double mul2 = d * d;

    Vec3 v;
    v.x = (double)(v1.x * mul0 + v2.x * mul1 + v3.x * mul2);
    v.y = (double)(v1.y * mul0 + v2.y * mul1 + v3.y * mul2);
    v.z = (double)(v1.z * mul0 + v2.z * mul1 + v3.z * mul2);

    return v;
}

static Vec2 Interpolated_vec2_quadratic(const Vec2 &v1, const Vec2 &v2,
                                        const Vec2 &v3, double d)
{

    const double inv = 1.0f - d;
    const double mul0 = inv * inv;
    const double mul1 = 2.0f * d * inv;
    const double mul2 = d * d;

    Vec2 v;
    v.x = (double)(v1.x * mul0 + v2.x * mul1 + v3.x * mul2);
    v.y = (double)(v1.y * mul0 + v2.y * mul1 + v3.y * mul2);

    return v;
}

 

static Color Interpolated_color_quadratic(const Color& c0, const Color& c1, const Color& c2, float d)
{
    d = Clamp(d, 0.f, 1.f);
    const float inv = 1.f - d;
    const float mul0 = inv * inv;
    const float mul1 = 2.f * d * inv;
    const float mul2 = d * d;

    float a = GetValue(c0.a) * mul0 + GetValue(c1.a) * mul1 + GetValue(c2.a) * mul2;
    float r = GetValue(c0.r) * mul0 + GetValue(c1.r) * mul1 + GetValue(c2.r) * mul2;
    float g = GetValue(c0.g) * mul0 + GetValue(c1.g) * mul1 + GetValue(c2.g) * mul2;
    float b = GetValue(c0.b) * mul0 + GetValue(c1.b) * mul1 + GetValue(c2.b) * mul2;

    Color result;
    result.a =  ToByte(a);
    result.r = ToByte(r);
    result.g = ToByte(g);
    result.b = ToByte(b);

    return result;
}

BSP::Vertex2TCoords BSP::SBezier::Interpolated_quadratic(BSP::Vertex2TCoords p0,
                                                         BSP::Vertex2TCoords p1,
                                                         BSP::Vertex2TCoords p2, double f)
{
    BSP::Vertex2TCoords result;
    result.position = Interpolated_vec3_quadratic(p0.position, p1.position, p2.position, f);
    result.normal = Interpolated_vec3_quadratic(p0.normal, p1.normal, p2.normal, f);
    result.uv0 = Interpolated_vec2_quadratic(p0.uv0, p1.uv0, p2.uv0, f);
    result.uv1 = Interpolated_vec2_quadratic(p0.uv1, p1.uv1, p2.uv1, f);
    result.color = Interpolated_color_quadratic(p0.color, p1.color, p2.color, f);

    return result;
}