#pragma once
#include "Node3D.hpp"
#include <vector>

class VertexArray;
class VertexBuffer;
class IndexBuffer;
class Texture;
class Shader;
class ParticleAffector;

// ==========================================
// ENUMS
// ==========================================

enum class EmissionMode
{
    Continuous,  // Emite constantemente (fogo, fumo, motor)
    Burst,       // Rajada única ou periódica (explosão)
    OneShot,     // Dispara uma vez e para (tiro, spark)
    Pulse        // Pulsos periódicos (piscar, intermitente)
};

enum class EmitterShape
{
    Point,       // Ponto único
    Sphere,      // Esfera
    Box,         // Caixa
    Cone,        // Cone
    Circle,      // Círculo plano
    Ring         // Anel
};

enum class EmitterState
{
    Stopped,
    Playing,
    Paused
};

struct Particle
{
    // Transform
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    
    // Visual
    Vec4 color;
    Vec2 size;
    float rotation;
    float rotationSpeed;
    
 
    FloatRect texRect;  // UV region na atlas (x, y, width, height)
    
    // Lifetime
    float lifetime;
    float timeAlive;
    
    // State
    bool active;
    
    // Initial values
    Vec4 colorStart;
    Vec4 colorEnd;
    Vec2 sizeStart;
    Vec2 sizeEnd;
    
    Particle()
        : position(0, 0, 0)
        , velocity(0, 0, 0)
        , acceleration(0, 0, 0)
        , color(1, 1, 1, 1)
        , size(1, 1)
        , rotation(0)
        , rotationSpeed(0)
        , texRect(0, 0, 1, 1)  // Default: textura inteira
        , lifetime(1.0f)
        , timeAlive(0.0f)
        , active(false)
        , colorStart(1, 1, 1, 1)
        , colorEnd(1, 1, 1, 0)
        , sizeStart(1, 1)
        , sizeEnd(0.5f, 0.5f)
    {
    }
};


 
struct ParticleVertex
{
    Vec3 position;
    Vec2 texCoord;
    Vec4 color;
    
    ParticleVertex() : position(0, 0, 0), texCoord(0, 0), color(1, 1, 1, 1) {}
    ParticleVertex(const Vec3& pos, const Vec2& uv, const Vec4& col)
        : position(pos), texCoord(uv), color(col) {}
};

 
class ParticleSystem : public Node3D
{
private:
    // Particles pool
    std::vector<Particle> particles;
    std::vector<ParticleAffector*> affectors;
    
    // Rendering data
    std::vector<ParticleVertex> vertices;
    std::vector<u32> indices;
    VertexArray* vao;
    VertexBuffer* vb;
    IndexBuffer* ib;
    Texture* texture;
    
    // Configuration
    EmissionMode emissionMode;
    EmitterShape emitterShape;
    EmitterState emitterState;
    
    // Emitter offset/direction (local space)
    Vec3 emissionOffset;      // Offset relativo ao transform do node
    Vec3 emissionDirection;   // Direção local
    
    // Emission parameters
    float emissionRate;
    float burstInterval;
    int burstCount;
    int oneShotCount;
    float pulseRate;
    int particlesPerPulse;
    
    std::vector<FloatRect> atlasFrames;  
    bool useAtlas;
    int currentFrame; 

    // Shape parameters
    float radius;
    float innerRadius;
    float angle;
    Vec3 boxSize;
    
    // Particle properties
    float lifetimeMin;
    float lifetimeMax;
    float speedMin;
    float speedMax;
    Vec2 sizeStart;
    Vec2 sizeEnd;
    Vec4 colorStart;
    Vec4 colorEnd;
    float rotationSpeedMin;
    float rotationSpeedMax;
    float spreadAngle;
    
    // Physics
    Vec3 gravity;
    float drag;
    
    // Control
    int maxParticles;
    int activeCount;
    float duration;
    bool loop;
    bool autoPlay;
    
    // Timing
    float emissionTimer;
    float emissionTimeAlive;
    float burstTimer;
    float pulseTimer;
    bool hasEmittedOneShot;

public:
    // ==========================================
    // CONSTRUCTOR / DESTRUCTOR
    // ==========================================
    ParticleSystem(const std::string& name, int maxParticles = 1000);
    virtual ~ParticleSystem();
    
 
    void update(float deltaTime) override;
    void render(Shader* shader) override;
    
    
    void RenderParticles(const Mat4& view, const Mat4& proj);
    

    void serialize(Serialize& serialize) override;
    void deserialize(const Serialize& in) override;
    
    ParticleSystem* SetAtlasFrames(const std::vector<FloatRect>& frames);
    ParticleSystem* AddAtlasFrame(const FloatRect& frame);
    ParticleSystem* AddAtlasFrame(float x, float y, float w, float h);
    
    // Grid atlas (divide textura em grid NxM)
    ParticleSystem* SetAtlasGrid(int columns, int rows);
    
    ParticleSystem* SetFrame(int frameIndex);
    
    // Clear atlas
    void ClearAtlas();

    ParticleSystem* SetEmissionMode(EmissionMode mode);
    ParticleSystem* SetContinuous(float particlesPerSecond);
    ParticleSystem* SetBurst(int count, float interval = 0.0f);
    ParticleSystem* SetOneShot(int count);
    ParticleSystem* SetPulse(float pulsesPerSecond, int particlesPerPulse);
    
    
    // ==========================================
    // EMITTER SHAPE
    // ==========================================
    ParticleSystem* SetShapePoint();
    ParticleSystem* SetShapeSphere(float radius);
    ParticleSystem* SetShapeBox(const Vec3& size);
    ParticleSystem* SetShapeCone(float angleDegrees, float baseRadius = 0.0f);
    ParticleSystem* SetShapeCircle(float radius);
    ParticleSystem* SetShapeRing(float outerRadius, float innerRadius);
    
    // ==========================================
    // OFFSET & DIRECTION (local space)
    // ==========================================
    ParticleSystem* SetEmissionOffset(const Vec3& offset);
    ParticleSystem* SetEmissionDirection(const Vec3& dir);
    ParticleSystem* SetSpreadAngle(float degrees);
    
    Vec3 GetEmissionOffset() const { return emissionOffset; }
    Vec3 GetEmissionDirection() const { return emissionDirection; }
    
    // Get world space emission (calculado do transform)
    Vec3 GetWorldEmissionPosition() const;
    Vec3 GetWorldEmissionDirection() const;
    
    // ==========================================
    // PARTICLE PROPERTIES
    // ==========================================
    ParticleSystem* SetLifetime(float min, float max);
    ParticleSystem* SetLifetime(float lifetime);
    
    ParticleSystem* SetSpeed(float min, float max);
    ParticleSystem* SetSpeed(float speed);
    
    ParticleSystem* SetSize(const Vec2& start, const Vec2& end);
    ParticleSystem* SetSize(float size);
    ParticleSystem* SetSize(float startSize, float endSize);
    
    ParticleSystem* SetColor(const Vec4& start, const Vec4& end);
    ParticleSystem* SetColor(const Vec4& color);
    
    ParticleSystem* SetRotationSpeed(float min, float max);
    ParticleSystem* SetRotationSpeed(float speed);
    
    // ==========================================
    // PHYSICS
    // ==========================================
    ParticleSystem* SetGravity(const Vec3& g);
    ParticleSystem* SetDrag(float d);
    
    // ==========================================
    // DURATION & LOOP
    // ==========================================
    ParticleSystem* SetDuration(float seconds);
    ParticleSystem* SetLoop(bool enabled);
    ParticleSystem* SetAutoPlay(bool enabled);
    
    // ==========================================
    // TEXTURE
    // ==========================================
    ParticleSystem* SetTexture(Texture* tex);
    Texture* GetTexture() const { return texture; }
    
    // ==========================================
    // CONTROL
    // ==========================================
    void Play();
    void Stop();
    void Pause();
    void Reset();
    
    void Fire(int count = -1);  // Dispara usando transform do node
    void EmitBurst(int count);
    
    EmitterState GetState() const { return emitterState; }
    bool IsPlaying() const { return emitterState == EmitterState::Playing; }
    
    // ==========================================
    // AFFECTORS
    // ==========================================
    ParticleSystem* AddGravity(const Vec3& g);
    ParticleSystem* AddDrag(float strength);
    ParticleSystem* AddVortex(const Vec3& center, float strength, float radius);
    ParticleSystem* AddAttractor(const Vec3& pos, float strength, float radius, bool repulse = false);
    ParticleSystem* AddTurbulence(float strength, float frequency);
    ParticleSystem* AddColorOverLifetime(const Vec4& start, const Vec4& end);
    ParticleSystem* AddSizeOverLifetime(const Vec2& start, const Vec2& end);
    
    void ClearAffectors();
    int GetAffectorCount() const { return affectors.size(); }
    
    // ==========================================
    // STATS
    // ==========================================
    int GetActiveCount() const { return activeCount; }
    int GetMaxParticles() const { return maxParticles; }
    
private:
    // ==========================================
    // INTERNAL METHODS
    // ==========================================
    
    // Emission
    void EmitContinuous(float deltaTime);
    void EmitBurstMode(float deltaTime);
    void EmitPulseMode(float deltaTime);
    
    // Particle management
    Particle* GetFreeParticle();
    void InitializeParticle(Particle* p);
    
    // Generation
    Vec3 GetEmissionPosition();
    Vec3 GetEmissionVelocity();
    
    // Update
    void UpdateParticles(float deltaTime);
    void ApplyAffectors(float deltaTime);
    
    // Rendering
    void BuildGeometry(const Mat4& view);
    
    // Helpers
    float Random(float min, float max);
    Vec3 RandomUnitVector();
};

// ==========================================
// AFFECTOR BASE CLASS
// ==========================================
class ParticleAffector
{
public:
    bool enabled;
    
    ParticleAffector() : enabled(true) {}
    virtual ~ParticleAffector() = default;
    
    virtual void Apply(Particle& particle, float deltaTime) = 0;
};

// Affectors (mesmo que antes)
class GravityAffector : public ParticleAffector
{
public:
    Vec3 gravity;
    GravityAffector(const Vec3& g) : gravity(g) {}
    void Apply(Particle& particle, float deltaTime) override;
};

class DragAffector : public ParticleAffector
{
public:
    float drag;
    DragAffector(float d) : drag(d) {}
    void Apply(Particle& particle, float deltaTime) override;
};

class VortexAffector : public ParticleAffector
{
public:
    Vec3 center;
    float strength;
    float radius;
    VortexAffector(const Vec3& c, float s, float r) : center(c), strength(s), radius(r) {}
    void Apply(Particle& particle, float deltaTime) override;
};

class AttractorAffector : public ParticleAffector
{
public:
    Vec3 position;
    float strength;
    float radius;
    bool repulse;
    AttractorAffector(const Vec3& pos, float s, float r, bool rep = false)
        : position(pos), strength(s), radius(r), repulse(rep) {}
    void Apply(Particle& particle, float deltaTime) override;
};

class TurbulenceAffector : public ParticleAffector
{
public:
    float strength;
    float frequency;
    float time;
    TurbulenceAffector(float s, float f) : strength(s), frequency(f), time(0.0f) {}
    void Apply(Particle& particle, float deltaTime) override;
};

class ColorOverLifetimeAffector : public ParticleAffector
{
public:
    Vec4 startColor;
    Vec4 endColor;
    ColorOverLifetimeAffector(const Vec4& start, const Vec4& end)
        : startColor(start), endColor(end) {}
    void Apply(Particle& particle, float deltaTime) override;
};

class SizeOverLifetimeAffector : public ParticleAffector
{
public:
    Vec2 startSize;
    Vec2 endSize;
    SizeOverLifetimeAffector(const Vec2& start, const Vec2& end)
        : startSize(start), endSize(end) {}
    void Apply(Particle& particle, float deltaTime) override;
};
