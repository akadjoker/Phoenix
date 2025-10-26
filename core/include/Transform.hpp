#pragma once

#include "Math.hpp"
#include <vector>

 
class Transform 
{
private:
    // Local transform (relativo ao parent)
    Vec3 localPosition;
    Quat localRotation;
    Vec3 localScale;
    
    // Cached world transform
    Vec3 worldPosition;
    Quat worldRotation;
    Vec3 worldScale;
    Mat4 worldMatrix;
    
    // Hierarquia
    Transform* parent;
    std::vector<Transform*> children;
    
    // Dirty flags para otimização
    bool localDirty;
    bool worldDirty;
    
    // Atualiza world transform a partir do local
    void updateWorldTransform();
    
    // Marca este transform e todos os filhos como dirty
    void markDirty();

public:
    // Construtor
    Transform();
    Transform(const Vec3& position);
    Transform(const Vec3& position, const Quat& rotation);
    Transform(const Vec3& position, const Quat& rotation, const Vec3& scale);
    
    // Destrutor
    ~Transform();
    
    // ==================== Local Transform ====================
    
    // Position
    void setLocalPosition(const Vec3& position);
    void setLocalPosition(float x, float y, float z);
    Vec3 getLocalPosition() const;
    
    // Rotation
    void setLocalRotation(const Quat& rotation);
    void setLocalRotation(const Vec3& eulerDegrees); // Pitch, Yaw, Roll
    void setLocalRotation(float pitch, float yaw, float roll); // Em graus
    Quat getLocalRotation() const;
    Vec3 getLocalEulerAngles() const; // Retorna em graus
    
    // Scale
    void setLocalScale(const Vec3& scale);
    void setLocalScale(float uniformScale);
    void setLocalScale(float x, float y, float z);
    Vec3 getLocalScale() const;
    
    // ==================== World Transform ====================
    
    // Position
    void setPosition(const Vec3& position);
    void setPosition(float x, float y, float z);
    Vec3 getPosition();
    
    // Rotation
    void setRotation(const Quat& rotation);
    void setRotation(const Vec3& eulerDegrees);
    void setRotation(float pitch, float yaw, float roll);
    Quat getRotation();
    Vec3 getEulerAngles(); // Retorna em graus
    
    // Scale
    void setScale(const Vec3& scale);
    void setScale(float uniformScale);
    Vec3 getScale();
    
    // ==================== Matrizes ====================
    
    Mat4 getLocalMatrix();
    Mat4 getWorldMatrix();
    
    // ==================== Hierarquia ====================
    
    void setParent(Transform* newParent);
    Transform* getParent() const;
    void removeParent();
    
    void addChild(Transform* child);
    void removeChild(Transform* child);
    const std::vector<Transform*>& getChildren() const;
    int getChildCount() const;
    Transform* getChild(int index) const;
    
    // ==================== Direções (World Space) ====================
    
    Vec3 forward();  // -Z (OpenGL convention)
    Vec3 back();     // +Z
    Vec3 right();    // +X
    Vec3 left();     // -X
    Vec3 up();       // +Y
    Vec3 down();     // -Y
    
    // ==================== Movimento ====================
    
    // Movimento em world space
    void translate(const Vec3& translation);
    void translate(float x, float y, float z);
    
    // Movimento em local space
    void translateLocal(const Vec3& translation);
    void translateLocal(float x, float y, float z);
    
    // Atalhos úteis (local space)
    void moveForward(float distance);
    void moveBack(float distance);
    void moveRight(float distance);
    void moveLeft(float distance);
    void moveUp(float distance);
    void moveDown(float distance);
    
    // Strafe (movimento lateral sem rotação)
    void strafe(float right, float up);
    void strafe(float right, float forward, float up);
    
    // ==================== Rotação ====================
    
    // Rotação em world space
    void rotate(const Quat& rotation);
    void rotate(const Vec3& axis, float degrees);
    void rotateX(float degrees);
    void rotateY(float degrees);
    void rotateZ(float degrees);
    
    // Rotação em local space
    void rotateLocal(const Quat& rotation);
    void rotateLocal(const Vec3& axis, float degrees);
    void rotateLocalX(float degrees);
    void rotateLocalY(float degrees);
    void rotateLocalZ(float degrees);
    
    // Rotação tipo FPS (pitch/yaw limitados)
    void rotateFPS(float pitch, float yaw);
    
    // ==================== Look At ====================
    
    void lookAt(const Vec3& target);
    void lookAt(const Vec3& target, const Vec3& up);
    void lookAt(const Transform& target);
    
    // Look direction (sem especificar ponto)
    void lookDirection(const Vec3& direction);
    void lookDirection(const Vec3& direction, const Vec3& up);
    
    // ==================== Interpolação (para animações) ====================
    
    // Linear interpolation
    static Transform Lerp(const Transform& a, const Transform& b, float t);
    
    // Smooth interpolation (usa Slerp para rotação)
    static Transform Slerp(const Transform& a, const Transform& b, float t);
    
    // ==================== Utilidades ====================
    
    // Transforma ponto de local para world space
    Vec3 transformPoint(const Vec3& localPoint);
    
    // Transforma direção de local para world space (ignora position)
    Vec3 transformDirection(const Vec3& localDirection);
    
    // Transforma ponto de world para local space
    Vec3 inverseTransformPoint(const Vec3& worldPoint);
    
    // Transforma direção de world para local space
    Vec3 inverseTransformDirection(const Vec3& worldDirection);
    
    // Reset
    void reset(); // Identity transform
    
    // Debug
    void print() const;
};

 