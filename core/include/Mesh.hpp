#pragma once
#include "Config.hpp"
#include "Object.hpp"
#include "GraphicsTypes.hpp"
#include "LoadTypes.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class VertexBuffer;
class IndexBuffer;
class VertexArray;
class Mesh;
class MeshBuffer;
class MeshManager;
class Stream;
class Texture;
class Driver;
class RenderBatch;
class MeshWriter;
class MeshLoader;
class Animator;
class Pixmap;
class Terrain;
 

constexpr u32 MESH_MAGIC = 0x4D455348; // "MESH"
constexpr u32 MESH_VERSION = 100;      // 1.00

constexpr u32 BUFFER_FLAG_SKINNED = 1 << 0;  // Tem skinning data
constexpr u32 BUFFER_FLAG_TANGENTS = 1 << 1; // Tem tangents
constexpr u32 BUFFER_FLAG_COLORS = 1 << 2;   // Tem vertex colors

constexpr u32 CHUNK_MATS = 0x4D415453; // "MATS" - Materials
constexpr u32 CHUNK_BUFF = 0x42554646; // "BUFF" - Buffer
constexpr u32 CHUNK_VRTS = 0x56525453; // "VRTS" - Vertices
constexpr u32 CHUNK_IDXS = 0x49445853; // "IDXS" - Indices
constexpr u32 CHUNK_SKEL = 0x534B454C; // "SKEL" - Skeleton
constexpr u32 CHUNK_SKIN = 0x534B494E; // "SKIN" - Skinning data
constexpr u32 CHUNK_ANIM = 0x414E494D; // "ANIM" - Reserved

constexpr u32 ANIM_MAGIC = 0x414E494D; // "ANIM"
constexpr u32 ANIM_VERSION = 100;      // v1.00

// Chunk IDs
constexpr u32 ANIM_CHUNK_INFO = 0x494E464F; // "INFO" - Animation info
constexpr u32 ANIM_CHUNK_CHAN = 0x4348414E; // "CHAN" - Channel (per bone)
constexpr u32 ANIM_CHUNK_KEYS = 0x4B455953; // "KEYS" - Keyframes

struct ChunkHeader
{
    u32 id;
    u32 length;
};

const u32 MAX_TEXTURES = 6;

struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct VertexSkin
{
    u8 boneIDs[4];
    float weights[4];
};

struct Bone
{
    std::string name;
    bool hasAnimation;
    s32 parentIndex; // -1 = root
    Mat4 transform;
    Mat4 localPose;
    Mat4 inverseBindPose;
    Bone *parent{nullptr};

    Bone();

    Mat4 GetGlobalTransform() const;
    Mat4 GetLocalTransform() const;
};

class Material
{
    Texture *m_texture[MAX_TEXTURES];
    u8 m_layers;
    std::string m_name;
    Vec3 m_diffuse;
    Vec3 m_specular;
    float m_shininess;

public:
    Material();

    void SetTexture(u32 index, Texture *texture);
    Texture *GetTexture(u32 index) const;

    u8 GetLayers() const { return m_layers; }

    void SetName(const std::string &name) { m_name = name; }
    const std::string &GetName() const { return m_name; }

    void SetDiffuse(const Vec3 &diffuse) { m_diffuse = diffuse; }
    const Vec3 &GetDiffuse() const { return m_diffuse; }

    void SetSpecular(const Vec3 &specular) { m_specular = specular; }
    const Vec3 &GetSpecular() const { return m_specular; }

    void SetShininess(float shininess) { m_shininess = shininess; }
    float GetShininess() const { return m_shininess; }
};

class MeshBuffer
{
private:
    std::vector<Vertex> vertices;
    std::vector<Vertex> m_skinnedVertices;

    std::vector<VertexSkin> m_skinData;
    std::vector<u32> indices;
    bool m_isSkinned = false;
    bool m_vdirty;
    bool m_idirty;
    VertexArray *buffer;
    VertexBuffer *vb;
    IndexBuffer *ib;
    BoundingBox m_boundBox;
    u32 m_material{0};
    friend class Mesh;
    friend class MeshManager;
    friend class Driver;
    friend class MeshReader;
    friend class MeshWriter;
    friend class Terrain;

public:
    MeshBuffer();
    ~MeshBuffer();

    void Clear();

    u32 AddVertex(const Vertex &v);
    u32 AddVertex(float x, float y, float z, float u, float v);
    u32 AddVertex(float x, float y, float z, float nx, float ny, float nz, float u, float v);
    u32 AddIndex(u32 index);
    u32 AddFace(u32 i0, u32 i1, u32 i2);

    u32 GetVertexCount() const { return vertices.size(); }
    u32 GetIndexCount() const { return indices.size(); }

    void SetMaterial(u32 material);
    u32 GetMaterial() const { return m_material; }

    void Build();

    void Render();
    void Render(PrimitiveType type, u32 count);
    void Debug(RenderBatch *batch);

    void UpdateSkinning(Mesh *mesh);

    void RemoveDuplicateVertices(float threshold);
    void Optimize();

    void CalculateNormals(bool smooth = true);
    void CalculateTangents();
    void CalculateBoundingBox();

    void GeneratePlanarUVsAuto(float resolution);
    void GeneratePlanarUVsAxis(float resolutionS, float resolutionT, int axis, const Vec3 &offset);
    void GeneratePlanarUVsAdvanced(float resolutionS, float resolutionT, const Vec3 &offset, bool autoDetect);

    void GeneratePlanarUVs(const Vec3 &axis = Vec3(0, 1, 0));
    void GenerateBoxUVs();
    void GenerateSphericalUVs();
    void ScaleUVs(const Vec2 &scale);
    void OffsetUVs(const Vec2 &offset);

    void Transform(const Mat4 &matrix);
    void TransformPositions(const Mat4 &matrix);
    void TransformNormals(const Mat3 &normalMatrix);
    void Translate(const Vec3 &offset);
    void Rotate(const Quat &rotation);
    void Rotate(const Vec3 &axis, float angle);
    void Scale(const Vec3 &scale);
    void Scale(float uniformScale);

    BoundingBox GetBoundingBox() { return m_boundBox; }
    const BoundingBox &GetBoundingBox() const { return m_boundBox; }

    void Reverse();
    void FlipNormals();
    Vec3 Center();
    void Merge(const MeshBuffer &other);

    bool IsSkinned() const { return m_isSkinned; }

    const VertexSkin *GetSkinData() const { return m_skinData.data(); }
    void SetSkinData(const std::vector<VertexSkin> &data)
    {
        m_skinData = data;
        m_isSkinned = !data.empty();
    }
    const Vertex *GetVertices() const { return vertices.data(); }
    const u32 *GetIndices() const { return indices.data(); }
};

struct AnimationKeyframe
{
    float time;
    Vec3 position;
    Quat rotation;

    AnimationKeyframe() : time(0.0f), position(), rotation() {}
};

struct AnimationChannel
{
    std::string boneName;
    u32 boneIndex; // Index no mesh
    std::vector<AnimationKeyframe> keyframes;
};

class Animation
{
public:
    bool Load(const std::string &filename);
    void Update(float deltaTime);
    void BindToMesh(Mesh *mesh);
    float GetDuration() const { return m_duration; }
    float GetTicksPerSecond() const { return m_ticksPerSecond; }
    const std::string &GetName() const { return m_name; }

    AnimationChannel *GetChannel(u32 index) { return &m_channels[index]; }

    AnimationChannel *FindChannel(const std::string &name);

    Vec3 InterpolatePosition(const AnimationChannel &channel, float time);
    Quat InterpolateRotation(const AnimationChannel &channel, float time);

    u32 GetChannelCount() const { return m_channels.size(); }

    bool operator==(const Animation &other) const { return m_name == other.m_name; }
    bool operator!=(const Animation &other) const { return !(*this == other); }

private:
    std::string m_name;
    float m_duration;
    float m_ticksPerSecond;
    Mesh *m_mesh = nullptr;
    float m_currentTime;
    friend class Animator;
    friend class AnimationLayer;

    void Sample(float time);

    std::vector<AnimationChannel> m_channels;
};

enum class PlayMode
{
    Once,
    Loop,
    PingPong
};

class AnimationLayer
{
public:
    AnimationLayer(Mesh *mesh);
    ~AnimationLayer();

    void AddAnimation(const std::string &name, Animation *anim);
    Animation *GetAnimation(const std::string &name);
    Animation *LoadAnimation(const std::string &name, const std::string &filename);

    // Controle de playback (animação única)
    void Play(const std::string &animName, PlayMode mode = PlayMode::Loop,
              float blendTime = 0.3f);
    void PlayOneShot(const std::string &animName, const std::string &returnTo, float blendTime = 0.3f, PlayMode toMode = PlayMode::Loop);
    void CrossFade(const std::string &toAnim, float duration);
    void Stop(float blendOutTime = 0.3f);
    void Pause();
    void Resume();

    // Update
    void Update(float deltaTime);

    // Getters
    bool IsPlaying(const std::string &animName) const;
    float GetCurrentTime() const { return m_currentTime; }
    const std::string &GetCurrentAnimation() const { return m_currentAnimName; }

    // Settings
    void SetSpeed(float speed) { m_globalSpeed = speed; }
    void SetDefaultBlendTime(float time) { m_defaultBlendTime = time; }

private:
    Mesh *m_mesh;
    std::map<std::string, Animation *> m_animations;

    // Animação única atual
    std::string m_currentAnimName;
    std::string m_previousAnimName;
    Animation *m_currentAnim;
    Animation *m_previousAnim;
    Animation *m_playTo;

    float m_currentTime;
    float m_currentTimeBlend;
    float m_globalSpeed;
    bool m_isPaused;

    // Blending
    bool m_isBlending;
    float m_blendTime;
    float m_blendDuration;

    // OneShot
    std::string m_returnToAnim;
    bool m_shouldReturn;

    PlayMode m_currentMode;
    PlayMode m_toReturnMode;
    float m_defaultBlendTime;
    bool m_isPingPongReverse;

    // Métodos privados
    void UpdateBlending(float deltaTime);
    void UpdateLayers(float deltaTime);
    bool CheckAnimationEnd();
};

class Animator
{

    std::vector<AnimationLayer *> layers;
    Mesh *m_mesh;

public:
    Animator(Mesh *mesh);
    ~Animator();
    void Update(float deltaTime);

    AnimationLayer *AddLayer();
    AnimationLayer *GetLayer(u32 index);


};


class Visual : public Object
{

protected:
    std::vector<Material *> materials;
    BoundingBox m_boundBox;    

    friend class MeshBuffer;
    friend class MeshManager;
    friend class Driver;
    friend class MeshReader;
public:
    Visual(const std::string &name = "Visual");
    ~Visual();
    Material *AddMaterial(const std::string &name);
    Material *GetMaterial(u32 index) const { return materials[index]; }
    void SetTexture(u32 layer, Texture *texture);
    u32 GetMaterialCount() const { return materials.size(); }

    
    const BoundingBox &GetBoundingBox() const { return m_boundBox; }
    BoundingBox &GetBoundingBox() { return m_boundBox; }
    
    
    virtual void Clear() {};
    virtual void Build() {};
    virtual void Render() {};
    virtual void CalculateBoundingBox() {};
    virtual void Debug(RenderBatch *batch) {(void) batch;};
};

class Mesh : public Visual
{
public:
    Mesh(const std::string &name = "Mesh");
    ~Mesh();

    void Clear() override;
    void Build() override;
    void Render() override;
    void CalculateBoundingBox() override;
    void Debug(RenderBatch *batch) override;


    MeshBuffer *AddBuffer(u32 material = 0);
    void CalculateNormals();

    
    
    size_t GetBufferCount() const { return buffers.size(); }
    MeshBuffer *GetBuffer(size_t index) const { return buffers[index]; }
    
    bool SetMaterial(u32 material);
    bool SetBufferMaterial(u32 index, u32 material);


    void OptimizeBuffers();

    bool HasSkeleton() const { return !m_bones.empty(); }
    bool IsSkinned() const { return !m_bones.empty(); }
    u32 GetBoneCount() const { return m_bones.size(); }
    Bone *GetBone(u32 index) const;
    Bone *AddBone(const std::string &name);

    std::vector<Bone *> &GetBones() { return m_bones; }
    const std::vector<Bone *> &GetBones() const { return m_bones; }

    void UpdateSkinning();



    void CalculateBoneMatrices();
    const std::vector<Mat4> &GetBoneMatrices() const { return m_boneMatrices; }
    Bone *FindBone(const std::string &name);

    u32 FindBoneIndex(const std::string &name);
    Mat4 GetBoneMatrix(u32 index) const;
    Mat4 GetBoneBindPoseMatrix(u32 index) const;

    void SetBoneTransform(u32 index, const Vec3 &position, const Quat &rotation);
    void SetBoneStatic(u32 index);
    void ResetBones();


private:
    std::vector<Bone *> m_bones;
    std::vector<Mat4> m_boneMatrices;
    std::vector<MeshBuffer *> buffers;
   

    friend class MeshBuffer;
    friend class MeshManager;
    friend class Driver;

    void SortByMaterial();
};

class MeshLoader
{
public:
    virtual ~MeshLoader() = default;
    virtual bool Load(Stream *stream, Mesh *mesh) = 0;
    virtual std::vector<std::string> GetExtensions() const = 0;
    virtual bool CanLoad(const std::string &filename) const;
    virtual bool CanLoad(Stream *stream) const { return false; }

protected:
    bool HasExtension(const std::string &filename, const std::string &ext) const;
    std::string m_basePath;
};

struct OBJMaterial
{
    std::string name;
    Vec3 diffuse;
    Vec3 specular;
    float shininess;
    std::string texture;
};

class OBJMeshLoader : public MeshLoader
{
public:
    virtual bool Load(Stream *stream, Mesh *mesh) override;
    virtual std::vector<std::string> GetExtensions() const override;

private:
    struct TempVertex
    {
        float x, y, z;
    };
    struct TempTexCoord
    {
        float u, v;
    };
    struct TempNormal
    {
        float nx, ny, nz;
    };

    struct FaceIndex
    {
        int v, vt, vn;
        bool operator==(const FaceIndex &other) const
        {
            return v == other.v && vt == other.vt && vn == other.vn;
        }
    };

    struct FaceIndexHash
    {
        size_t operator()(const FaceIndex &fi) const
        {
            return ((size_t)fi.v << 32) ^ ((size_t)fi.vt << 16) ^ (size_t)fi.vn;
        }
    };

    void ParseLine(const std::string &line, MeshBuffer *buffer,
                   std::vector<TempVertex> &positions,
                   std::vector<TempTexCoord> &texcoords,
                   std::vector<TempNormal> &normals,
                   std::unordered_map<FaceIndex, u32, FaceIndexHash> &vertexCache,
                   bool &hasNormals);

    void ParseFace(const std::string &line, MeshBuffer *buffer,
                   const std::vector<TempVertex> &positions,
                   const std::vector<TempTexCoord> &texcoords,
                   const std::vector<TempNormal> &normals,
                   std::unordered_map<FaceIndex, u32, FaceIndexHash> &vertexCache);

    bool LoadMTL(const std::string &mtlPath);
    u32 FindMaterialID(const std::string &name);
    std::string GetDirectory(const std::string &filepath);

    std::vector<OBJMaterial> m_materials;
    std::map<std::string, u32> m_materialMap;
};

class Loader3DS : public MeshLoader
{
public:
    virtual bool Load(Stream *stream, Mesh *mesh) override;
    virtual std::vector<std::string> GetExtensions() const override;

private:
    struct Chunk
    {
        u16 id;
        u32 length;
        u32 bytesRead;
    };

    struct Material3DS
    {
        std::string name;
        float ambient[3];
        float diffuse[3];
        float specular[3];
        std::string textureName;
        u32 materialID;
    };

    struct Object3DS
    {
        std::string name;
        std::vector<float> vertices;        // x,y,z
        std::vector<u16> indices;           // triangulos
        std::vector<float> texCoords;       // u,v
        std::vector<std::string> materials; // material por face
        std::vector<u32> smoothGroups;      // smooth groups
    };

    Stream *m_stream;
    std::vector<Material3DS> m_materials;
    std::vector<Object3DS> m_objects;

    Chunk ReadChunk();
    void SkipChunk(const Chunk &chunk);

    void ProcessMainChunk();
    void ProcessEditChunk(const Chunk &parent);
    void ProcessObjectChunk(const Chunk &parent);
    void ProcessMaterialChunk(const Chunk &parent);
    void ProcessMeshChunk(Object3DS &obj, const Chunk &parent);

    std::string ReadString();
    void ReadColorChunk(float *color);
    float ReadPercentage();

    void BuildMesh(Mesh *mesh);
    u32 FindMaterialID(const std::string &name);
};

class MeshWriter
{
public:
    bool Save(const Mesh *mesh, const std::string &filename);

private:
    Stream *m_stream;

    void BeginChunk(u32 chunkId, long *posOut);
    void EndChunk(long startPos);
    void WriteCString(const std::string &str);

    void WriteMaterialsChunk(const Mesh *mesh);
    void WriteSkeletonChunk(const Mesh *mesh);
    void WriteBufferChunk(const MeshBuffer *buffer);
    void WriteVerticesChunk(const MeshBuffer *buffer);
    void WriteIndicesChunk(const MeshBuffer *buffer);
    void WriteSkinChunk(const MeshBuffer *buffer);
};

class MeshReader
{
public:
    bool Load(const std::string &filename, Mesh *mesh);

private:
    Stream *m_stream;

    ChunkHeader ReadChunkHeader();
    void SkipChunk(const ChunkHeader &header);
    std::string ReadCString();

    void ReadMaterialsChunk(Mesh *mesh, const ChunkHeader &header);
    void ReadSkeletonChunk(Mesh *mesh, const ChunkHeader &header);
    void ReadBufferChunk(Mesh *mesh, const ChunkHeader &header);
    void ReadVerticesChunk(MeshBuffer *buffer, const ChunkHeader &header);
    void ReadIndicesChunk(MeshBuffer *buffer, const ChunkHeader &header);
    void ReadSkinChunk(MeshBuffer *buffer, const ChunkHeader &header);
};

class AnimReader
{
public:
    struct Channel
    {
        std::string boneName;                     // Nome do bone (ex: "mixamorig:LeftArm")
        std::vector<AnimationKeyframe> keyframes; // Keyframes ao longo do tempo
    };

    struct FrameAnimation
    {
        std::string name;              // Nome da animação (ex: "idle", "walk", "run")
        float duration;                // Duração total em segundos (ex: 2.5s)
        float ticksPerSecond;          // FPS/ticks (ex: 25.0, 30.0, 60.0)
        std::vector<Channel> channels; // 1 channel por bone animado

        FrameAnimation()
            : name("unnamed"), duration(0.0f), ticksPerSecond(25.0f)
        {
        }
    };

    FrameAnimation *Load(const std::string &filename);

private:
    Stream *m_stream;
    bool ReadInfoChunk(FrameAnimation &info);
    bool ReadChannelChunk(Channel &channel);
};

class MeshManager
{
public:
    static MeshManager &Instance();

    Mesh *Get(const std::string &name);
    void Add(const std::string &name, Mesh *mesh);
    bool Exists(const std::string &name) const;


    

    Mesh *Create(const std::string &name);

    Mesh *CreateCube(const std::string &name, float size = 1.0f);
    Mesh *CreatePlane(const std::string &name, float width = 1.0f, float height = 1.0f, int detailX = 1, int detailY = 1, float tilesH = 1.0f, float tilesV = 1.0f);
    Mesh *CreateSphere(const std::string &name, float radius = 1.0f, int segments = 32, int rings = 16);
    Mesh *CreateCylinder(const std::string &name, float radius = 1.0f, float height = 2.0f, int segments = 32, bool caps = true);
    Mesh *CreateCone(const std::string &name, float radius = 1.0f, float height = 2.0f, int segments = 32);
    Mesh *CreateQuad(const std::string &name, const Vec3 &face,
                     float size = 1.0f, float tilesU = 1.0f, float tilesV = 1.0f);
    Mesh *CreateHillPlane(
        const std::string &name,
        float width, float height,
        int segmentsX, int segmentsY,
        float hillHeight = 0.0f,
        float hillCountX = 1.0f,
        float hillCountY = 1.0f,
        float tilesU = 1.0f,
        float tilesV = 1.0f);

    Mesh *CreateTerrainFromHeightmap(
        const std::string &name,
        const std::string &heightmapPath,
        float width, float height, float maxHeight,
        int detailX = 128, int detailY = 128,
        float tilesU = 1.0f, float tilesV = 1.0f);

    Mesh *CreateTerrainFromPixmap(
        const std::string &name,
        const Pixmap *heightmap,
        float width, float height, float maxHeight,
        int detailX = 128, int detailY = 128,
        float tilesU = 1.0f, float tilesV = 1.0f);

    Terrain *CreateTerrain(const std::string &name,const std::string& heightmapPath,
                          float scaleX, float scaleY, float scaleZ,
                          float texScaleU = 1.0f, float texScaleV = 1.0f);
    Terrain *GetTerrain(const std::string &name);
    

  
    void UnloadAll();

    Mesh *Load(const std::string &name, const std::string &filename);
    bool Save(const std::string &filename, const Mesh *mesh);

    void RegisterImporter(MeshLoader *loader);
    Mesh *Import(const std::string &name, const std::string &filename);
    Mesh *ImportFromStream(const std::string &name, Stream *stream, const std::string &extension);

    MeshManager();
    ~MeshManager();

private:
    std::unordered_map<std::string, Mesh *> m_meshes;
    std::unordered_map<std::string, Terrain *> m_terrains;
    std::vector<MeshLoader *> m_loaders;
};

