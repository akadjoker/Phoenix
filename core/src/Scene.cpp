#include "pch.h"
#include "Scene.hpp"
#include "Node3D.hpp"
#include "GameObject.hpp"
#include "Camera.hpp"
#include "Frustum.hpp"
#include "Terrain.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "Color.hpp"
#include "Batch.hpp"

Camera *Scene::createCamera(const std::string &name)
{
    Camera *camera = new Camera(name);
    m_cameras.push_back(camera);
    ActiveCamera = camera;
    return camera;
}

bool Scene::Pick(float x, float y, Node3D **out)
{
    const u32 width = Device::Instance().GetWidth();
    const u32 height = Device::Instance().GetHeight();

    bool state = false;

    Ray ray = ActiveCamera->screenPointToRay(x, y, width, height);
    for (u32 i = 0; i < m_objects.size(); i++)
    {
        if (!m_objects[i]->isPickable())
        {
            continue;
        }
        if (m_objects[i]->pick(ray))
        {
            *out = m_objects[i];
            state = true;
        }
    }
    return state;
}

bool Scene::Pick(RenderType renderPass, float x, float y, Node3D **out)
{
    const u32 width = Device::Instance().GetWidth();
    const u32 height = Device::Instance().GetHeight();
    Ray ray = ActiveCamera->screenPointToRay(x, y, width, height);
    bool state = false;
    LogInfo("Picking  origin: (%f, %f, %f) direction: (%f, %f, %f)", ray.origin.x, ray.origin.y, ray.origin.z, ray.direction.x, ray.direction.y, ray.direction.z);
    for (u32 i = 0; i < m_objects.size(); i++)
    {
        if (m_objects[i]->getRenderType() == renderPass)
        {
            if (m_objects[i]->pick(ray))
            {
                *out = m_objects[i];
                state = true;
            }
        }
    }
    return state;
}

Scene::Scene()
{
    m_frustum = Driver::Instance().GetFrustum();
    m_current_camera = 0;
    m_ready = false;
}

Scene::~Scene()
{

    m_frustum = nullptr;
}

void Scene::Clear()
{
    m_ready = false;
    m_total = 0;
    m_visible = 0;
    m_render_solids.clear();
    m_render_trasparent.clear();
    m_render_special.clear();
    m_render_skyes.clear();
    m_render_lights.clear();
    m_render_mirrors.clear();
    m_render_waters.clear();
    m_render_terrains.clear();

    removeObjects();
    for (u32 i = 0; i < m_cameras.size(); i++)
    {
        delete m_cameras[i];
    }
    m_cameras.clear();
}

void Scene::removeObjects()
{
    for (auto *o : m_objects)
        delete o;

    m_objects.clear();
}

void Scene::Release()
{
    Clear();
    OnDestroy();
}

void Scene::Debug(RenderBatch *batch)
{
    OnDebug(batch);
    // for (auto *m : m_meshes)
    // {
    //     batch->SetColor(255, 0, 0);
    //     for (u32 i = 0; i < m->getSubMeshCount(); i++)
    //     {
    //         const SubMesh *sm = m->getSubMesh(i);
    //         batch->Box(sm->getBoundingBox());
    //     }

    //     batch->SetColor(0, 255, 0);
    //     batch->Box(m->getBoundingBox());
    // }

    batch->SetColor(255, 0, 255);

    for (u32 i = 0; i < m_objects.size(); i++)
    {
        // if (m_objects[i]->isShowBoxes())
        //  batch->Box(m_objects[i]->getTransformedBoundingBox());
    }

    for (u32 i = 0; i < m_objects.size(); i++)
    {
        //  m_objects[i]->debug(batch);
        if (m_objects[i]->isShowBoxes())
            batch->Box(m_objects[i]->getTransformedBoundingBox());
    }
}

Node3D *Scene::createNode3D(const std::string &name, Node3D *parent)
{
    Node3D *node = new Node3D(name);
    if (parent)
    {
        node->setParent(parent);
    } else 
    {
        m_objects.push_back(node);
        m_needRebuildLists = true;

    }
    return node;
}

Terrain *Scene::createTerrain(const std::string &name, const std::string &heightmapPath, float scaleX, float scaleY, float scaleZ, float texScaleU, float texScaleV)
{
    Terrain *node = new Terrain(name);
    if (!node->LoadFromHeightmap(heightmapPath, scaleX, scaleY, scaleZ, texScaleU, texScaleV))
    {
        delete node;
        return nullptr;
    }
    node->setRenderType(RenderType::Terrain);
    m_objects.push_back(node);
    m_needRebuildLists = true;
    return node;
}

TerrainLod *Scene::createTerrainLod(const std::string &name, const std::string &heightmapPath, int maxLOD, PatchSize patchSize, const Vec3 &position, const Vec3 &scale, float heightScale, int smoothFactor)
{
    TerrainLod *node = new TerrainLod(name, maxLOD, patchSize, position, scale);
    node->setRenderType(RenderType::Terrain);
    node->LoadHeightMap(heightmapPath);
    m_objects.push_back(node);
    m_needRebuildLists = true;
    return node;
}

GameObject *Scene::createGameObject(const std::string &name, Node3D *parent)
{
    GameObject *node = new GameObject(name);
    node->setRenderType(RenderType::Solid);
    if (parent)
    {
        node->setParent(parent);
    } else
    {

        m_objects.push_back(node);
        m_needRebuildLists = true;
    }
    node->setActive(true);
    node->awake();
    return node;
}

GameObject *Scene::getGameObjectByName(const std::string &name) const
{
    for (u32 i = 0; i < m_objects.size(); i++)
    {
        if (m_objects[i]->getName() == name)
        {
            return (GameObject *)m_objects[i];
        }
    }
    return nullptr;
}

void Scene::setActiveCamera(Camera *camera)
{
    ActiveCamera = camera;
    Driver::Instance().SetCamera(camera);
}

const Mat4 &Scene::getViewMatrix() const
{
    return m_view;
}

const Mat4 &Scene::getProjectionMatrix() const
{
    return m_proj;
}

bool Scene::Save(const std::string &filename)
{
    SceneParser parser;
    return parser.Save(filename, *this);
}

bool Scene::Load(const std::string &filename)
{
    SceneParser parser;
    return parser.Parse(filename, *this);
}

bool Scene::Init()
{
    if (OnCreate())
    {
        m_ready = true;
        return true;
    }
    LogWarning("[Scene] Failed to create scene");
    return false;
}

bool Scene::isVisible(Node3D *node)
{
    return true;//(m_frustum->intersectsAABB(node->getTransformedBoundingBox()) && node->isActive());
}

void Scene::renderAll(Shader *shader)
{
    if (!m_ready)
    {
        LogWarning("[Scene] Scene is not ready");
        return;
    }
    rebuildRenderLists();

    // Render all passes in order
    renderPass(shader, RenderType::Terrain);
    renderPass(shader, RenderType::Light);
    renderPass(shader, RenderType::Solid);
    renderPass(shader, RenderType::Mirror);
    renderPass(shader, RenderType::Water);
    renderPass(shader, RenderType::Trasparent);
    renderPass(shader, RenderType::Special);
    renderPass(shader, RenderType::Sky);
}

void Scene::SetCamera(Camera *camera)
{
    if (!m_ready || !camera)
    {
        LogWarning("[Scene] Scene is not ready");
        return;
    }
    Driver::Instance().SetCamera(camera);
    ActiveCamera = camera;
    m_view = ActiveCamera->getViewMatrix();
    m_proj = ActiveCamera->getProjectionMatrix();
    m_frustum = Driver::Instance().GetFrustum();
    m_frustum->extractFromCamera(m_view, m_proj);
    camWorldPos = ActiveCamera->getPosition();
    m_needRebuildLists = true;
}

void Scene::rebuildRenderLists()
{
    if (!m_needRebuildLists)
        return;

    m_total = m_objects.size();
    m_visible = 0;

  
    m_render_solids.clear();
    m_render_trasparent.clear();
    m_render_special.clear();
    m_render_skyes.clear();
    m_render_lights.clear();
    m_render_mirrors.clear();
    m_render_waters.clear();
    m_render_terrains.clear();

    // Cull and sort
    for (Node3D *object : m_objects)
    {
        if (!object->isActive())
            continue;

        RenderType type = object->getRenderType();

        // Sky, Water e Mirror não fazem frustum culling
        bool skipCulling = (type == RenderType::Sky ||
                            type == RenderType::Water ||
                            type == RenderType::Mirror);

        if (!skipCulling && !m_frustum->intersectsAABB(object->getTransformedBoundingBox()))
            continue;

        m_visible++;

        switch (type)
        {
        case RenderType::Sky:
            m_render_skyes.push_back(object);
            break;
        case RenderType::Terrain:
            m_render_terrains.push_back(object);
            break;
        case RenderType::Solid:
            m_render_solids.push_back(object);
            break;
        case RenderType::Light:
            m_render_lights.push_back(object);
            break;
        case RenderType::Trasparent:
            m_render_trasparent.push_back(object);
            break;
        case RenderType::Special:
            m_render_special.push_back(object);
            break;
        case RenderType::Water:
            m_render_waters.push_back(object);
            break;
        case RenderType::Mirror:
            m_render_mirrors.push_back(object);
            break;
        }
    }

    // // Sort transparent back-to-front (IMPORTANTE para alpha blending correto)
    // std::sort(m_render_trasparent.begin(), m_render_trasparent.end(),
    //           [this](Node3D *a, Node3D *b)
    //           {
    //               float distA = (a->getPosition() - camWorldPos).lengthSquared();
    //               float distB = (b->getPosition() - camWorldPos).lengthSquared();
    //               return distA > distB; // Back to front
    //           });

    // // Sort solid front-to-back (reduz overdraw e melhora early-z)
    // std::sort(m_render_solids.begin(), m_render_solids.end(),
    //           [this](Node3D *a, Node3D *b)
    //           {
    //               float distA = (a->getPosition() - camWorldPos).lengthSquared();
    //               float distB = (b->getPosition() - camWorldPos).lengthSquared();
    //               return distA < distB; // Front to back
    //           });

    m_needRebuildLists = false;
}

void Scene::renderPass(Shader *shader, RenderType renderPass,bool useMaterial)
{
 
    rebuildRenderLists();

    const std::vector<Node3D *> *renderList = nullptr;
    bool useSpecialDepth = false;

    // Seleciona a lista correta
    switch (renderPass)
    {
    case RenderType::Sky:
        renderList = &m_render_skyes;
        useSpecialDepth = true;
        break;
    case RenderType::Terrain:
        renderList = &m_render_terrains;
        break;
    case RenderType::Light:
        renderList = &m_render_lights;
        break;
    case RenderType::Solid:
        renderList = &m_render_solids;
        break;
    case RenderType::Trasparent:
        renderList = &m_render_trasparent;
        break;
    case RenderType::Special:
        renderList = &m_render_special;
        break;
    case RenderType::Water:
        renderList = &m_render_waters;
        break;
    case RenderType::Mirror:
        renderList = &m_render_mirrors;
        break;
    default:
        return;
    }

    if (!renderList || renderList->empty())
        return;


    if(!useMaterial)
     {
        Driver::Instance().BindTexture(0,GL_TEXTURE_2D, TextureManager::Instance().GetDefault()->GetHandle());
     }    
  
    if (useSpecialDepth)
        glDepthFunc(GL_LEQUAL);

 
    for (Node3D *object : *renderList)
    {
       
        object->render(shader,useMaterial);
    }

 
    if (useSpecialDepth)
        glDepthFunc(GL_LESS);
}

void Scene::Render()
{
    OnRender();
}

void Scene::Update(float dt)
{
    if (!m_ready)
    {
        LogWarning("[Scene] Scene is not ready");
        return;
    }

    OnUpdate(dt);

    if (ActiveCamera)
    {
        ActiveCamera->update(dt);
    }

    const std::vector<Node3D *> *updateLists[] =
        {
            &m_render_terrains,
            &m_render_lights,
            &m_render_skyes,
            &m_render_solids,
            &m_render_trasparent,
            &m_render_special,
            &m_render_waters,
            &m_render_mirrors};

    for (const auto *list : updateLists)
    {
        for (Node3D *object : *list)
        {
            object->update(dt);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------

SceneParser::SceneParser()
    : m_currentLine(0), m_lineIndex(0)
{
}

SceneParser::~SceneParser()
{
}

void SceneParser::SetError(const std::string &error)
{
    std::ostringstream oss;
    oss << "Line " << m_currentLine << ": " << error;
    m_error = oss.str();
}

static std::string Trim(const std::string &str)
{
    size_t start = 0;
    size_t end = str.length();

    while (start < end && std::isspace(str[start]))
        start++;

    while (end > start && std::isspace(str[end - 1]))
        end--;

    return str.substr(start, end - start);
}

static std::vector<std::string> Split(const std::string &str, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter))
    {
        result.push_back(Trim(item));
    }

    return result;
}


static bool ParseVector3(const std::string &value, Vec3 &vec)
{
    std::vector<std::string> parts = Split(value, ' ');
    if (parts.size() != 3)
        return false;

    vec.x = static_cast<f32>(std::atof(parts[0].c_str()));
    vec.y = static_cast<f32>(std::atof(parts[1].c_str()));
    vec.z = static_cast<f32>(std::atof(parts[2].c_str()));

    return true;
}

static bool ParseVector4(const std::string &value, Vec4 &vec)
{
    std::vector<std::string> parts = Split(value, ' ');
    if (parts.size() != 4)
        return false;

    vec.x = static_cast<f32>(std::atof(parts[0].c_str()));
    vec.y = static_cast<f32>(std::atof(parts[1].c_str()));
    vec.z = static_cast<f32>(std::atof(parts[2].c_str()));
    vec.w = static_cast<f32>(std::atof(parts[3].c_str()));

    return true;
}

static bool ParseVector2(const std::string &value, Vec2 &vec)
{
    std::vector<std::string> parts = Split(value, ' ');
    if (parts.size() != 2)
        return false;

    vec.x = static_cast<f32>(std::atof(parts[0].c_str()));
    vec.y = static_cast<f32>(std::atof(parts[1].c_str()));

    return true;
}

static bool StartsWith(const std::string &str, const std::string &prefix)
{
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
}

std::string SceneParser::GetNextLine()
{
    if (m_lineIndex >= m_lines.size())
        return "";

    m_currentLine = m_lineIndex + 1;
    return m_lines[m_lineIndex++];
}

std::string SceneParser::PeekNextLine()
{
    SkipEmptyLines();
    if (m_lineIndex >= m_lines.size())
        return "";
    return m_lines[m_lineIndex];
}

bool SceneParser::HasMoreLines() const
{
    return m_lineIndex < m_lines.size();
}

void SceneParser::SkipEmptyLines()
{
    while (m_lineIndex < m_lines.size())
    {
        std::string line = Trim(m_lines[m_lineIndex]);
        if (line.empty() || StartsWith(line, "//"))
            m_lineIndex++;
        else
            break;
    }
}

bool SceneParser::Parse(const std::string &filename, Scene &outScene)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        m_error = "Failed to open file: " + filename;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return ParseFromString(buffer.str(), outScene);
}

bool SceneParser::ParseFromString(const std::string &content, Scene &outScene)
{
    m_error.clear();
    m_currentLine = 0;
    m_lineIndex = 0;
    m_lines.clear();
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line))
    {
        m_lines.push_back(line);
    }

    return ParseScene(outScene);
}

bool SceneParser::ParseScene(Scene &scene)
{
    SkipEmptyLines();

    std::string line = GetNextLine();
    if (Trim(line) != "scene")
    {
        SetError("Expected 'scene' keyword");
        return false;
    }

    line = GetNextLine();
    if (Trim(line) != "{")
    {
        SetError("Expected '{' after 'scene'");
        return false;
    }

    while (HasMoreLines())
    {
        SkipEmptyLines();
        std::string peek = PeekNextLine();
        std::string trimmed = Trim(peek);

        if (trimmed == "}")
        {
            GetNextLine(); // consume '}'
            break;
        }

        std::string key, value;
        if (ParseProperty(peek, key, value))
        {
            GetNextLine(); // consume the line
            if (key == "name")
            {
                // scene.name = value;
            }
        }
        else if (trimmed == "objects")
        {
            if (!ParseObjects(scene))
                return false;
        }
        else if (trimmed == "meshes")
        {
            // if (!ParseMeshes(scene))
            return false;
        }
        else
        {
            GetNextLine(); // skip unknown line
        }
    }

    return true;
}

bool SceneParser::ParseProperty(const std::string &line, std::string &key, std::string &value)
{
    std::string trimmed = Trim(line);
    size_t pos = trimmed.find(' ');

    if (pos == std::string::npos)
        return false;

    key = trimmed.substr(0, pos);
    value = Trim(trimmed.substr(pos + 1));

    // Remove quotes
    if (value.length() >= 2 && value[0] == '"' && value[value.length() - 1] == '"')
    {
        value = value.substr(1, value.length() - 2);
    }

    return true;
}

bool SceneParser::ParseObjects(Scene &scene)
{
    std::string line = GetNextLine();
    if (Trim(line) != "objects")
    {
        SetError("Expected 'objects' keyword");
        return false;
    }

    line = GetNextLine();
    if (Trim(line) != "{")
    {
        SetError("Expected '{' after 'objects'");
        return false;
    }

    while (HasMoreLines())
    {
        SkipEmptyLines();
        std::string peek = PeekNextLine();
        std::string trimmed = Trim(peek);

        if (trimmed == "}")
        {
            GetNextLine(); // consume '}'
            break;
        }

        if (trimmed == "object")
        {
            Serialize obj;
            if (!ParseObject(obj))
                return false;
            // LogInfo("Object: %s", obj.GetString("name").c_str());
            scene.OnSerialize(obj);
        }
        else
        {
            GetNextLine(); // skip unknown line
        }
    }

    return true;
}

static bool TryParseBool(const std::string &v, bool &out)
{
    if (v == "true" || v == "1")
    {
        out = true;
        return true;
    }
    if (v == "false" || v == "0")
    {
        out = false;
        return true;
    }
    return false;
}

static bool TryParseInt(const std::string &v, int &out)
{
    if (v.empty())
        return false;

    char *endPtr = nullptr;
    long val = std::strtol(v.c_str(), &endPtr, 10);

    if (endPtr == v.c_str() || *endPtr != '\0')
        return false;

    out = static_cast<int>(val);
    return true;
}

static bool TryParseFloat(const std::string &v, float &out)
{
    if (v.empty())
        return false;

    char *endPtr = nullptr;
    float val = std::strtof(v.c_str(), &endPtr);

    if (endPtr == v.c_str() || *endPtr != '\0')
        return false;

    out = val;
    return true;
}

bool SceneParser::ParseObject(Serialize &obj)
{
    // "object"
    std::string line = GetNextLine();
    if (Trim(line) != "object")
    {
        SetError("Expected 'object' keyword");
        return false;
    }

    // "{"
    line = GetNextLine();
    if (Trim(line) != "{")
    {
        SetError("Expected '{' after 'object'");
        return false;
    }

    while (HasMoreLines())
    {
        SkipEmptyLines();
        std::string peek = PeekNextLine();
        std::string trimmed = Trim(peek);

        if (trimmed == "}")
        {
            GetNextLine();
            break;
        }

        std::string key, value;
        if (ParseProperty(peek, key, value))
        {
            GetNextLine();

            auto parts = Split(value, ' ');
            const size_t count = parts.size();

            // 1 token  bool / int / float / string
            if (count == 1)
            {
                const std::string &v = parts[0];

                bool b;
                int i;
                float f;

                if (TryParseBool(v, b))
                    obj.SetBool(key, b);
                else if (TryParseInt(v, i))
                    obj.SetInt(key, i);
                else if (TryParseFloat(v, f))
                    obj.SetFloat(key, f);
                else
                    obj.SetString(key, v);
            }
            // 2 tokens  Vec2
            else if (count == 2)
            {
                Vec2 v;
                if (!ParseVector2(value, v))
                {
                    SetError("Invalid Vec2 format for '" + key + "'");
                    return false;
                }
                obj.SetVec2(key, v);
            }
            // 3 tokens  Vec3
            else if (count == 3)
            {
                Vec3 v;
                if (!ParseVector3(value, v))
                {
                    SetError("Invalid Vec3 format for '" + key + "'");
                    return false;
                }
                obj.SetVec3(key, v);
            }
            // 4 tokens  Vec4
            else if (count == 4)
            {
                Vec4 v;
                if (!ParseVector4(value, v))
                {
                    SetError("Invalid Vec4 format for '" + key + "'");
                    return false;
                }
                obj.SetVec4(key, v);
            }
            // Mais que 4 valores  string
            else
            {
                obj.SetString(key, value);
            }
        }
        else
        {
            GetNextLine();
        }
    }

    return true;
}

static std::string Vector4ToString(const Vec4 &vec)
{
    std::ostringstream oss;
    oss << vec.x << " " << vec.y << " " << vec.z << " " << vec.w;
    return oss.str();
}

static std::string Vector3ToString(const Vec3 &vec)
{
    std::ostringstream oss;
    oss << vec.x << " " << vec.y << " " << vec.z;
    return oss.str();
}
static std::string Vector2ToString(const Vec2 &vec)
{
    std::ostringstream oss;
    oss << vec.x << " " << vec.y;
    return oss.str();
}

static void WriteIndent(std::string &output, int level)
{
    for (int i = 0; i < level; i++)
        output += "    ";
}

void SceneParser::WritePropertyLine(
    std::string &output,
    int indent,
    const std::string &key,
    const Property &prop)
{
    WriteIndent(output, indent);

    switch (prop.type)
    {
    case Property::INT:
        output += key + " " + std::to_string(prop.i) + "\n";
        break;
    case Property::FLOAT:
        output += key + " " + std::to_string(prop.f) + "\n";
        break;
    case Property::BOOL:
        output += key + " " + std::string(prop.b ? "true" : "false") + "\n";
        break;
    case Property::STRING:
        output += key + " \"" + prop.str + "\"\n";
        break;
    case Property::VEC2:
        output += key + " " + Vector2ToString(prop.v2) + "\n";
        break;
    case Property::VEC3:
        output += key + " " + Vector3ToString(prop.v3) + "\n";
        break;
    case Property::VEC4:
        output += key + " " + Vector4ToString(prop.v4) + "\n";
        break;
    }
}

void SceneParser::WriteObjectFromSerialize(
    std::string &output,
    int indent,
    const Serialize &s)
{
    WriteIndent(output, indent);
    output += "object\n";
    WriteIndent(output, indent);
    output += "{\n";

    const auto &props = s.GetAllProperties();

    auto writeIfExists = [&](const char *key)
    {
        auto it = props.find(key);
        if (it != props.end())
            WritePropertyLine(output, indent + 1, it->first, it->second);
    };

    writeIfExists("name");
    writeIfExists("type");

    for (auto &kv : props)
    {
        const std::string &key = kv.first;
        if (key == "name" || key == "type")
            continue;
        WritePropertyLine(output, indent + 1, key, kv.second);
    }

    WriteIndent(output, indent);
    output += "}\n";
}

std::string SceneParser::ToString(const Scene &scene)
{

    std::string output;

    output += "// Scene Script\n";
    output += "scene\n";
    output += "{\n";

    WriteIndent(output, 1);
    output += "name scene\n\n";

    // Objects
    if (!scene.m_objects.empty())
    {
        WriteIndent(output, 1);
        output += "// Objects in scene\n";
        WriteIndent(output, 1);
        output += "objects\n";
        WriteIndent(output, 1);
        output += "{\n";

        for (auto *obj : scene.m_objects)
        {
            Serialize s;
            obj->serialize(s);
            WriteObjectFromSerialize(output, 2, s);
        }

        WriteIndent(output, 1);
        output += "}\n";
    }

    output += "}\n";
    return output;
}

bool SceneParser::Save(const std::string &filename, const Scene &scene)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        m_error = "Failed to open file for writing: " + filename;
        return false;
    }

    std::string content = ToString(scene);
    file << content;
    file.close();

    return true;
}
