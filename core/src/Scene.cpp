#include "pch.h"
#include "Scene.hpp"
#include "Node3D.hpp"
#include "GameObject.hpp"
#include "Camera.hpp"
#include "Frustum.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "Batch.hpp"



Camera* Scene::createCamera(const std::string &name)
{
    Camera *camera = new Camera(name);
    m_cameras.push_back(camera);
    ActiveCamera = camera;
    return camera;
}

Scene::Scene()
{
    m_frustum = new Frustum();
    m_current_camera = 0;
    m_ready = false;
}

Scene::~Scene()
{
    delete m_frustum;
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
       // batch->Box(m_objects[i]->getTransformedBoundingBox());
    }
}

Node3D *Scene::createNode3D(const std::string &name, Node3D *parent)
{
    Node3D *node = new Node3D(name);
    if (parent)
    {
        node->setParent(parent);
    }
    m_objects.push_back(node);
    return node;
}

GameObject *Scene::createGameObject(const std::string &name, Node3D *parent)
{
    GameObject *node = new GameObject(name);
    node->setRenderType(RenderType::Solid);
    if (parent)
    {
        node->setParent(parent);
    }
    m_objects.push_back(node);
    return node;
}

void Scene::setActiveCamera(Camera *camera)
{
    ActiveCamera = camera;
}



const Mat4 &Scene::getViewMatrix() const
{
    return m_view;
}

const Mat4 &Scene::getProjectionMatrix() const
{
    return m_proj;
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
   return true;// return (m_frustum->intersectsAABB(node->getTransformedBoundingBox()) && node->isActive());
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
    renderPass(shader, RenderType::Sky);
    renderPass(shader, RenderType::Solid);
    renderPass(shader, RenderType::Trasparent);
    renderPass(shader, RenderType::Special);
    renderPass(shader, RenderType::Mirror);
    renderPass(shader, RenderType::Water);
}

void Scene::SetCamera(Camera *camera)
{
    if (!m_ready || !camera)
    {
        LogWarning("[Scene] Scene is not ready");
        return;
    }
    ActiveCamera = camera;
    m_view = ActiveCamera->getViewMatrix();
    m_proj = ActiveCamera->getProjectionMatrix();
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
    
    // Clear lists
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

        if (isVisible(object) && object->getRenderType() == RenderType::Solid)
        {
            m_render_solids.push_back(object);
            m_visible++;
        } else if (object->isActive() && object->getRenderType() == RenderType::Terrain)
        {
            m_render_terrains.push_back(object);
            m_visible++;
        }
        else if (isVisible(object) && object->getRenderType() == RenderType::Trasparent)
        {
            m_render_trasparent.push_back(object);
            m_visible++;
        }
        else if (isVisible(object) && object->getRenderType() == RenderType::Special)
        {
            m_render_special.push_back(object);
            m_visible++;
        }
        else if (object->isActive() && object->getRenderType() == RenderType::Sky)
        {
            m_render_skyes.push_back(object);
            m_visible++;
        }
        else if (isVisible(object) && object->getRenderType() == RenderType::Light)
        {
            m_render_lights.push_back(object);
            m_visible++;
        }
        else if (object->isActive() && object->getRenderType() == RenderType::Water)
        {
            m_render_waters.push_back(object);
            m_visible++;
        }
        else if (object->isActive() && object->getRenderType() == RenderType::Mirror)
        {
            m_render_mirrors.push_back(object);
            m_visible++;
        }
    }
    
    // // Sort transparent back-to-front
    // std::sort(m_render_trasparent.begin(), m_render_trasparent.end(), 
    //     [this](Node3D* a, Node3D* b) {
    //         float distA = (a->getPosition() - camWorldPos).lengthSquared();
    //         float distB = (b->getPosition() - camWorldPos).lengthSquared();
    //         return distA > distB;  // Back to front
    //     });
    
    // // Sort solid front-to-back (optional, for depth optimization)
    // std::sort(m_render_solids.begin(), m_render_solids.end(),
    //     [this](Node3D* a, Node3D* b) {
    //         float distA = (a->getPosition() - camWorldPos).lengthSquared();
    //         float distB = (b->getPosition() - camWorldPos).lengthSquared();
    //         return distA < distB;  // Front to back
    //     });
    
    m_needRebuildLists = false;
}

void Scene::renderPass(Shader *shader, RenderType renderPass)
{

    rebuildRenderLists();

    if (renderPass == RenderType::Terrain)
    {
        for (Node3D *object : m_render_terrains)
        {
            if (!object->isActive())
                continue;
            const Mat4 model = object->getWorldTransform();
            shader->SetUniformMat4("model", model.m);
           
        }
    }

    if (renderPass == RenderType::Light)
    {
        for (Node3D *object : m_render_lights)
        {
            if (!object->isActive())
                continue;
            const Mat4 model = object->getWorldTransform();
            shader->SetUniformMat4("model", model.m);
            object->render();
        }
    }

    if (renderPass == RenderType::Sky)
    {
        for (Node3D *object : m_render_skyes)
        {
            if (!object->isActive())
                continue;
            const Mat4 model = object->getWorldTransform();
            shader->SetUniformMat4("model", model.m);
            object->render();
        }
    }

    if (renderPass == RenderType::Solid)
    {
        for (Node3D *object : m_render_solids)
        {
            if (!object->isActive())
                continue;
            const Mat4 model = object->getWorldTransform();
            shader->SetUniformMat4("model", model.m);
            object->render();
        }
    }

    if (renderPass == RenderType::Trasparent)
    {
        for (Node3D *object : m_render_trasparent)
        {
            if (!object->isActive())
                continue;
            const Mat4 model = object->getWorldTransform();
            shader->SetUniformMat4("model", model.m);
            object->render();
        }
    }

    if (renderPass == RenderType::Special)
    {
        for (Node3D *object : m_render_special)
        {
            if (!object->isActive())
                continue;
            const Mat4 model = object->getWorldTransform();
            shader->SetUniformMat4("model", model.m);
            object->render();
        }
    }

    if (renderPass == RenderType::Water)
    {
        for (Node3D *object : m_render_waters)
        {
            if (!object->isActive())
                continue;
            const Mat4 model = object->getWorldTransform();
            shader->SetUniformMat4("model", model.m);
            object->render();
        }
    }

    if (renderPass == RenderType::Mirror)
    {
        for (Node3D *object : m_render_mirrors)
        {
            if (!object->isActive())
                continue;
            const Mat4 model = object->getWorldTransform();
            shader->SetUniformMat4("model", model.m);
            object->render();
        }
    }
}

void Scene::Render()
{
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

    // TODO  sort trasnsparent and lights by distance from camera, solid sort by texture index

    // LogInfo("Objects: %i visible / %i total", visible, total);

    for (Node3D *object : m_render_terrains)
    {
        object->update(dt);
    }

    for (Node3D *object : m_render_lights)
    {

        object->update(dt);
    }

    for (Node3D *object : m_render_skyes)
    {
        object->update(dt);
    }

    for (Node3D *object : m_render_solids)
    {
        object->update(dt);
    }

    for (Node3D *object : m_render_trasparent)
    {
        object->update(dt);
    }

    for (Node3D *object : m_render_special)
    {
        object->update(dt);
    }

    for (Node3D *object : m_render_waters)
    {
        object->update(dt);
    }

    for (Node3D *object : m_render_mirrors)
    {
        object->update(dt);
    }
}

