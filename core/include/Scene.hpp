#pragma once
#include "Config.hpp"
#include "Math.hpp"
#include "Node.hpp"
#include <vector>
#include <map>


 
class Node3D;
class GameObject;
class RenderBatch;
class Frustum;
class Shader;
class Camera;

class Scene
{
private:
    std::vector<Node3D *> m_objects;
    std::vector<Node3D *> m_objects_to_remove;

    std::vector<Camera *> m_cameras;

    std::vector<Node3D *> m_render_solids;
    std::vector<Node3D *> m_render_trasparent;
    std::vector<Node3D *> m_render_special;
    std::vector<Node3D *> m_render_mirrors;
    std::vector<Node3D *> m_render_waters;
    std::vector<Node3D *> m_render_lights;
    std::vector<Node3D *> m_render_skyes;
    std::vector<Node3D *> m_render_terrains;

    Camera *ActiveCamera;
    Mat4 m_view;
    Mat4 m_proj;
    Vec3 camWorldPos;
    Frustum *m_frustum;
    u32 m_total;
    u32 m_visible;
    u32 m_current_camera;
    bool m_ready;
    bool m_needRebuildLists=false;

public:
    explicit Scene();
    virtual ~Scene();

    Scene(const Scene &) = delete;
    Scene &operator=(const Scene &) = delete;
    Scene(Scene &&) noexcept = default;
    Scene &operator=(Scene &&) noexcept = default;


    bool Init();

    bool isVisible(Node3D *node);

    void renderPass(Shader *shader, RenderType renderPass);
    void renderAll(Shader *shader);

    void Render();
    void Update(float dt);

    void Clear();
    void removeObjects();

    void Release();

    void Debug(RenderBatch *batch);

    Camera *createCamera(const std::string &name = "Camera");
 

    u32 getTotalObjects() const { return m_total; }
    u32 getVisibleObjects() const { return m_visible; }

    Node3D *createNode3D(const std::string &name = "Node3D", Node3D *parent = nullptr);
    GameObject *createGameObject(const std::string &name = "GameObject", Node3D *parent = nullptr);
    const std::vector<Node3D *> &getObjects() const { return m_objects; }

    Camera *getActiveCamera() const { return ActiveCamera; }

    void setActiveCamera(Camera *camera) ;

    void SetCamera(Camera *camera);

    const Mat4 &getViewMatrix() const;
    const Mat4 &getProjectionMatrix() const;
protected:
    virtual void OnRender( ) = 0;
    virtual void OnUpdate(float dt) {};
    virtual bool OnCreate() { return true; }
    virtual void OnDestroy() {}
    virtual void OnResize(u32  w, u32 h) {}

    void rebuildRenderLists();
 
};


