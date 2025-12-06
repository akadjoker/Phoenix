
#include "pch.h"
#include "ParticleSystem.hpp"
#include "Vertex.hpp"
#include "Driver.hpp"
#include "Math.hpp"
#include "Texture.hpp"
#include "Stream.hpp"
#include "Batch.hpp"
#include "Pixmap.hpp"
#include "Animation.hpp"
#include "glad/glad.h"

// ==========================================
// CONSTRUCTOR
// ==========================================

ParticleSystem::ParticleSystem(const std::string& name, int maxParticles)
    : Node3D(name)
    , maxParticles(maxParticles)
    , activeCount(0)
    , texture(nullptr)
    , emissionMode(EmissionMode::Continuous)
    , emitterShape(EmitterShape::Point)
    , emitterState(EmitterState::Stopped)
    , emissionOffset(0, 0, 0)
    , emissionDirection(0, 1, 0)
    , emissionRate(10.0f)
    , burstInterval(1.0f)
    , burstCount(10)
    , oneShotCount(10)
    , pulseRate(1.0f)
    , particlesPerPulse(5)
    , radius(1.0f)
    , innerRadius(0.5f)
    , angle(Pi * 0.25f)
    , boxSize(1, 1, 1)
    , lifetimeMin(1.0f)
    , lifetimeMax(3.0f)
    , speedMin(1.0f)
    , speedMax(5.0f)
    , sizeStart(0.5f, 0.5f)
    , sizeEnd(0.1f, 0.1f)
    , colorStart(1, 1, 1, 1)
    , colorEnd(1, 1, 1, 0)
    , rotationSpeedMin(0.0f)
    , rotationSpeedMax(0.0f)
    , spreadAngle(0.0f)
    , gravity(0, 0, 0)
    , drag(0.0f)
    , duration(-1.0f)
    , loop(true)
    , autoPlay(false)
    , emissionTimer(0.0f)
    , emissionTimeAlive(0.0f)
    , burstTimer(0.0f)
    , pulseTimer(0.0f)
    , hasEmittedOneShot(false)
    , useAtlas(false)
    , currentFrame(0)
{
    // Pre-alocar pool de partículas
    particles.resize(maxParticles);
    for (auto& p : particles)
        p.active = false;
    
    // Setup rendering
    vao = new VertexArray();
    
    int maxVertices = maxParticles * 4;
    vb = vao->AddVertexBuffer(sizeof(ParticleVertex), maxVertices, true);
    
    auto* decl = vao->GetVertexDeclaration();
    decl->AddElement(0, 0, VET_FLOAT3, VES_POSITION);
    decl->AddElement(0, 3 * sizeof(float), VET_FLOAT2, VES_TEXCOORD, 0);
    decl->AddElement(0, 5 * sizeof(float), VET_FLOAT4, VES_COLOR);
    
    vertices.reserve(maxVertices);
    
    // PRÉ-CALCULAR ÍNDICES (UMA VEZ!)
    int maxIndices = maxParticles * 6;
    indices.reserve(maxIndices);
    
    for (int i = 0; i < maxParticles; i++)
    {
        u32 baseVertex = i * 4;
        
        // Triângulo 1
        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 2);
        
        // Triângulo 2
        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 3);
    }
    
    // Upload índices (UMA VEZ!)
    ib = vao->CreateIndexBuffer(maxIndices, false, false);
    ib->SetSubData(0, indices.size(), indices.data());
    
    // Bounding box
    m_boundBox = BoundingBox(Vec3(-10, -10, -10), Vec3(10, 10, 10));
    setPickable(false);
}

// ==========================================
// DESTRUCTOR
// ==========================================

ParticleSystem::~ParticleSystem()
{
    for (auto* affector : affectors)
        delete affector;
    
    delete vao;
}

// ==========================================
// EMISSION MODE
// ==========================================

ParticleSystem* ParticleSystem::SetEmissionMode(EmissionMode mode)
{
    emissionMode = mode;
    return this;
}

ParticleSystem* ParticleSystem::SetContinuous(float particlesPerSecond)
{
    emissionMode = EmissionMode::Continuous;
    emissionRate = particlesPerSecond;
    return this;
}

ParticleSystem* ParticleSystem::SetBurst(int count, float interval)
{
    emissionMode = EmissionMode::Burst;
    burstCount = count;
    burstInterval = interval;
    return this;
}

ParticleSystem* ParticleSystem::SetOneShot(int count)
{
    emissionMode = EmissionMode::OneShot;
    oneShotCount = count;
    return this;
}

ParticleSystem* ParticleSystem::SetPulse(float pulsesPerSecond, int particlesPerPulse)
{
    emissionMode = EmissionMode::Pulse;
    pulseRate = pulsesPerSecond;
    this->particlesPerPulse = particlesPerPulse;
    return this;
}

// ==========================================
// EMITTER SHAPE
// ==========================================

ParticleSystem* ParticleSystem::SetShapePoint()
{
    emitterShape = EmitterShape::Point;
    return this;
}

ParticleSystem* ParticleSystem::SetShapeSphere(float radius)
{
    emitterShape = EmitterShape::Sphere;
    this->radius = radius;
    return this;
}

ParticleSystem* ParticleSystem::SetShapeBox(const Vec3& size)
{
    emitterShape = EmitterShape::Box;
    boxSize = size;
    return this;
}

ParticleSystem* ParticleSystem::SetShapeCone(float angleDegrees, float baseRadius)
{
    emitterShape = EmitterShape::Cone;
    angle = angleDegrees * DEG2RAD;
    radius = baseRadius;
    return this;
}

ParticleSystem* ParticleSystem::SetShapeCircle(float radius)
{
    emitterShape = EmitterShape::Circle;
    this->radius = radius;
    return this;
}

ParticleSystem* ParticleSystem::SetShapeRing(float outerRadius, float innerRadius)
{
    emitterShape = EmitterShape::Ring;
    radius = outerRadius;
    this->innerRadius = innerRadius;
    return this;
}

// ==========================================
// OFFSET & DIRECTION
// ==========================================

ParticleSystem* ParticleSystem::SetEmissionOffset(const Vec3& offset)
{
    emissionOffset = offset;
    return this;
}

ParticleSystem* ParticleSystem::SetEmissionDirection(const Vec3& dir)
{
    emissionDirection = dir.normalized();
    return this;
}

ParticleSystem* ParticleSystem::SetSpreadAngle(float degrees)
{
    spreadAngle = degrees * DEG2RAD;
    return this;
}

Vec3 ParticleSystem::GetWorldEmissionPosition() const
{
    const Mat4& worldMat = getWorldTransform();
    return worldMat.TransformPoint(emissionOffset);
}

Vec3 ParticleSystem::GetWorldEmissionDirection() const
{
    const Mat4& worldMat = getWorldTransform();
    return worldMat.TransformVector(emissionDirection).normalized();
}

// ==========================================
// PARTICLE PROPERTIES
// ==========================================

ParticleSystem* ParticleSystem::SetLifetime(float min, float max)
{
    lifetimeMin = min;
    lifetimeMax = max;
    return this;
}

ParticleSystem* ParticleSystem::SetLifetime(float lifetime)
{
    lifetimeMin = lifetimeMax = lifetime;
    return this;
}

ParticleSystem* ParticleSystem::SetSpeed(float min, float max)
{
    speedMin = min;
    speedMax = max;
    return this;
}

ParticleSystem* ParticleSystem::SetSpeed(float speed)
{
    speedMin = speedMax = speed;
    return this;
}

ParticleSystem* ParticleSystem::SetSize(const Vec2& start, const Vec2& end)
{
    sizeStart = start;
    sizeEnd = end;
    return this;
}

ParticleSystem* ParticleSystem::SetSize(float size)
{
    sizeStart = sizeEnd = Vec2(size, size);
    return this;
}

ParticleSystem* ParticleSystem::SetSize(float startSize, float endSize)
{
    sizeStart = Vec2(startSize, startSize);
    sizeEnd = Vec2(endSize, endSize);
    return this;
}

ParticleSystem* ParticleSystem::SetColor(const Vec4& start, const Vec4& end)
{
    colorStart = start;
    colorEnd = end;
    return this;
}

ParticleSystem* ParticleSystem::SetColor(const Vec4& color)
{
    colorStart = colorEnd = color;
    return this;
}

ParticleSystem* ParticleSystem::SetRotationSpeed(float min, float max)
{
    rotationSpeedMin = min;
    rotationSpeedMax = max;
    return this;
}

ParticleSystem* ParticleSystem::SetRotationSpeed(float speed)
{
    rotationSpeedMin = rotationSpeedMax = speed;
    return this;
}

// ==========================================
// PHYSICS
// ==========================================

ParticleSystem* ParticleSystem::SetGravity(const Vec3& g)
{
    gravity = g;
    return this;
}

ParticleSystem* ParticleSystem::SetDrag(float d)
{
    drag = d;
    return this;
}

// ==========================================
// DURATION & LOOP
// ==========================================

ParticleSystem* ParticleSystem::SetDuration(float seconds)
{
    duration = seconds;
    return this;
}

ParticleSystem* ParticleSystem::SetLoop(bool enabled)
{
    loop = enabled;
    return this;
}

ParticleSystem* ParticleSystem::SetAutoPlay(bool enabled)
{
    autoPlay = enabled;
    return this;
}

// ==========================================
// TEXTURE
// ==========================================

ParticleSystem* ParticleSystem::SetTexture(Texture* tex)
{
    texture = tex;
    return this;
}

// ==========================================
// TEXTURE ATLAS
// ==========================================

ParticleSystem* ParticleSystem::SetAtlasFrames(const std::vector<FloatRect>& frames)
{
    atlasFrames = frames;
    useAtlas = !atlasFrames.empty();
    return this;
}

ParticleSystem* ParticleSystem::AddAtlasFrame(const FloatRect& frame)
{
    atlasFrames.push_back(frame);
    useAtlas = true;
    return this;
}

ParticleSystem* ParticleSystem::AddAtlasFrame(float x, float y, float w, float h)
{
    return AddAtlasFrame(FloatRect(x, y, w, h));
}

ParticleSystem* ParticleSystem::SetAtlasGrid(int columns, int rows)
{
    atlasFrames.clear();
    
    float cellWidth = 1.0f / columns;
    float cellHeight = 1.0f / rows;
    
    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < columns; col++)
        {
            float x = col * cellWidth;
            float y = row * cellHeight;
            
            atlasFrames.push_back(FloatRect(x, y, cellWidth, cellHeight));
        }
    }
    
    useAtlas = true;
    return this;
}

ParticleSystem* ParticleSystem::SetFrame(int frameIndex)
{
    currentFrame = frameIndex;
    return this;
}

void ParticleSystem::ClearAtlas()
{
    atlasFrames.clear();
    useAtlas = false;
}

// ==========================================
// CONTROL
// ==========================================

void ParticleSystem::Play()
{
    emitterState = EmitterState::Playing;
    
    if (emissionMode == EmissionMode::OneShot && !hasEmittedOneShot)
    {
        EmitBurst(oneShotCount);
        hasEmittedOneShot = true;
    }
}

void ParticleSystem::Stop()
{
    emitterState = EmitterState::Stopped;
    emissionTimer = 0.0f;
    emissionTimeAlive = 0.0f;
    burstTimer = 0.0f;
    pulseTimer = 0.0f;
    hasEmittedOneShot = false;
}

void ParticleSystem::Pause()
{
    emitterState = EmitterState::Paused;
}

void ParticleSystem::Reset()
{
    Stop();
    for (auto& p : particles)
        p.active = false;
    activeCount = 0;
}

void ParticleSystem::Fire(int count)
{
    if (count < 0)
        count = oneShotCount;
    
    EmitBurst(count);
}

void ParticleSystem::EmitBurst(int count)
{
    for (int i = 0; i < count; i++)
    {
        Particle* p = GetFreeParticle();
        if (p)
            InitializeParticle(p);
    }
}

// ==========================================
// AFFECTORS
// ==========================================

ParticleSystem* ParticleSystem::AddGravity(const Vec3& g)
{
    affectors.push_back(new GravityAffector(g));
    return this;
}

ParticleSystem* ParticleSystem::AddDrag(float strength)
{
    affectors.push_back(new DragAffector(strength));
    return this;
}

ParticleSystem* ParticleSystem::AddVortex(const Vec3& center, float strength, float radius)
{
    affectors.push_back(new VortexAffector(center, strength, radius));
    return this;
}

ParticleSystem* ParticleSystem::AddAttractor(const Vec3& pos, float strength, float radius, bool repulse)
{
    affectors.push_back(new AttractorAffector(pos, strength, radius, repulse));
    return this;
}

ParticleSystem* ParticleSystem::AddTurbulence(float strength, float frequency)
{
    affectors.push_back(new TurbulenceAffector(strength, frequency));
    return this;
}

ParticleSystem* ParticleSystem::AddColorOverLifetime(const Vec4& start, const Vec4& end)
{
    affectors.push_back(new ColorOverLifetimeAffector(start, end));
    return this;
}

ParticleSystem* ParticleSystem::AddSizeOverLifetime(const Vec2& start, const Vec2& end)
{
    affectors.push_back(new SizeOverLifetimeAffector(start, end));
    return this;
}

void ParticleSystem::ClearAffectors()
{
    for (auto* affector : affectors)
        delete affector;
    affectors.clear();
}

// ==========================================
// SERIALIZATION
// ==========================================

void ParticleSystem::serialize(Serialize& serialize)
{
    Node3D::serialize(serialize);
    
    serialize.SetVec3("emissionOffset", emissionOffset);
    serialize.SetVec3("emissionDirection", emissionDirection);
    serialize.SetBool("autoPlay", autoPlay);
    serialize.SetInt("maxParticles", maxParticles);
    serialize.SetInt("emissionMode", (int)emissionMode);
    serialize.SetInt("emitterShape", (int)emitterShape);
    
    // Emission params
    serialize.SetFloat("emissionRate", emissionRate);
    serialize.SetInt("burstCount", burstCount);
    serialize.SetFloat("burstInterval", burstInterval);
    
    // Particle properties
    serialize.SetFloat("lifetimeMin", lifetimeMin);
    serialize.SetFloat("lifetimeMax", lifetimeMax);
    serialize.SetFloat("speedMin", speedMin);
    serialize.SetFloat("speedMax", speedMax);
    serialize.SetVec2("sizeStart", sizeStart);
    serialize.SetVec2("sizeEnd", sizeEnd);
    serialize.SetVec4("colorStart", colorStart);
    serialize.SetVec4("colorEnd", colorEnd);
}

void ParticleSystem::deserialize(const Serialize& in)
{
    Node3D::deserialize(in);
    
    emissionOffset = in.GetVec3("emissionOffset", Vec3(0, 0, 0));
    emissionDirection = in.GetVec3("emissionDirection", Vec3(0, 1, 0));
    autoPlay = in.GetBool("autoPlay", false);
    
    emissionMode = (EmissionMode)in.GetInt("emissionMode", 0);
    emitterShape = (EmitterShape)in.GetInt("emitterShape", 0);
    
    emissionRate = in.GetFloat("emissionRate", 10.0f);
    burstCount = in.GetInt("burstCount", 10);
    burstInterval = in.GetFloat("burstInterval", 1.0f);
    
    lifetimeMin = in.GetFloat("lifetimeMin", 1.0f);
    lifetimeMax = in.GetFloat("lifetimeMax", 3.0f);
    speedMin = in.GetFloat("speedMin", 1.0f);
    speedMax = in.GetFloat("speedMax", 5.0f);
    sizeStart = in.GetVec2("sizeStart", Vec2(0.5f, 0.5f));
    sizeEnd = in.GetVec2("sizeEnd", Vec2(0.1f, 0.1f));
    colorStart = in.GetVec4("colorStart", Vec4(1, 1, 1, 1));
    colorEnd = in.GetVec4("colorEnd", Vec4(1, 1, 1, 0));
    
    if (autoPlay)
        Play();
}

// ParticleSystem.cpp (Continuação - Parte 2)

// ==========================================
// NODE3D OVERRIDE - UPDATE
// ==========================================

void ParticleSystem::update(float deltaTime)
{
    if (emitterState == EmitterState::Paused)
        return;
    
    emissionTimeAlive += deltaTime;
    
    // Emitir novas partículas
    if (emitterState == EmitterState::Playing)
    {
        switch (emissionMode)
        {
            case EmissionMode::Continuous:
                EmitContinuous(deltaTime);
                break;
            case EmissionMode::Burst:
                EmitBurstMode(deltaTime);
                break;
            case EmissionMode::Pulse:
                EmitPulseMode(deltaTime);
                break;
            case EmissionMode::OneShot:
                // Já emitiu no Play()
                break;
        }
        
        // Verificar duração
        if (duration > 0.0f && emissionTimeAlive >= duration)
        {
            if (loop)
            {
                emissionTimeAlive = 0.0f;
                hasEmittedOneShot = false;
            }
            else
            {
                Stop();
            }
        }
    }
    
    // Atualizar partículas
    UpdateParticles(deltaTime);
    
    // Aplicar affectors
    ApplyAffectors(deltaTime);
    
    // Contar ativas
    activeCount = 0;
    for (auto& p : particles)
        if (p.active) activeCount++;
    
    // Atualizar filhos
    UpdateChildren(deltaTime);
}

// ==========================================
// NODE3D OVERRIDE - RENDER
// ==========================================

void ParticleSystem::render(Shader* shader, bool useMaterial)
{
    // Partículas são renderizadas no RenderParticles
    // Apenas renderizar filhos aqui
    RenderChildren(shader, useMaterial);
}

void ParticleSystem::RenderParticles(const Mat4& view, const Mat4& proj)
{
    if (activeCount == 0)
        return;
    
    // Build geometry (billboards)
    BuildGeometry(view);
    
    if (vertices.empty())
        return;
    
    
    
    // Upload vértices (índices já estão no GPU!)
    vb->SetSubData(0, vertices.size()  , vertices.data());
    
    // Bind texture
    if (texture)
        texture->Bind(0);
    
    // Render apenas partículas ativas
    int activeIndices = activeCount * 6;
    vao->Render(PrimitiveType::PT_TRIANGLES, activeIndices);
}

// ==========================================
// EMISSION INTERNAL
// ==========================================

void ParticleSystem::EmitContinuous(float deltaTime)
{
    emissionTimer += deltaTime;
    
    float intervalPerParticle = 1.0f / emissionRate;
    
    int particlesToEmit = 0;
    while (emissionTimer >= intervalPerParticle)
    {
        particlesToEmit++;
        emissionTimer -= intervalPerParticle;
        
        // Proteção contra lag spike
        if (particlesToEmit > 100)
        {
            emissionTimer = 0.0f;
            break;
        }
    }
    
    for (int i = 0; i < particlesToEmit; i++)
    {
        Particle* p = GetFreeParticle();
        if (p)
            InitializeParticle(p);
    }
}

void ParticleSystem::EmitBurstMode(float deltaTime)
{
    burstTimer += deltaTime;
    
    if (burstTimer >= burstInterval)
    {
        EmitBurst(burstCount);
        burstTimer = 0.0f;
        
        if (!loop)
            Stop();
    }
}

void ParticleSystem::EmitPulseMode(float deltaTime)
{
    pulseTimer += deltaTime;
    
    float intervalPerPulse = 1.0f / pulseRate;
    
    if (pulseTimer >= intervalPerPulse)
    {
        EmitBurst(particlesPerPulse);
        pulseTimer = 0.0f;
    }
}

// ==========================================
// PARTICLE MANAGEMENT
// ==========================================

Particle* ParticleSystem::GetFreeParticle()
{
    // Procurar primeira inativa
    for (auto& p : particles)
    {
        if (!p.active)
            return &p;
    }
    
    // Pool cheio - procurar partícula que já viveu 90%+ do lifetime
    Particle* oldest = nullptr;
    float maxProgress = 0.9f;  // Só reciclar se passou 90% da vida
    
    for (auto& p : particles)
    {
        float progress = p.timeAlive / p.lifetime;
        if (progress > maxProgress)
        {
            maxProgress = progress;
            oldest = &p;
        }
    }
    
    // Se encontrou uma quase morta, reciclar
    if (oldest)
        return oldest;
    
    // Senão, não emitir (pool cheio com partículas novas)
    return nullptr;
}


void ParticleSystem::InitializeParticle(Particle* p)
{
    if (!p) return;
    
    p->position = GetEmissionPosition();
    p->velocity = GetEmissionVelocity();
    p->acceleration = gravity;
    
    p->lifetime = Random(lifetimeMin, lifetimeMax);
    p->timeAlive = 0.0f;
    
    p->sizeStart = sizeStart;
    p->sizeEnd = sizeEnd;
    p->size = sizeStart;
    
    p->colorStart = colorStart;
    p->colorEnd = colorEnd;
    p->color = colorStart;
    
    p->rotation = Random(0.0f, TwoPi);
    p->rotationSpeed = Random(rotationSpeedMin, rotationSpeedMax);
    
    // Texture rect (atlas)
    if (useAtlas && !atlasFrames.empty())
    {
        int frameIndex = currentFrame % atlasFrames.size();
        p->texRect = atlasFrames[frameIndex];
    }
    else
    {
        p->texRect = FloatRect(0, 0, 1, 1);
    }
    
    p->active = true;
}

// ==========================================
// EMISSION POSITION (baseado no shape)
// ==========================================

Vec3 ParticleSystem::GetEmissionPosition()
{
    Vec3 worldPos = GetWorldEmissionPosition();
    const Mat4& worldMat = getWorldTransform();
    
    Vec3 localOffset(0, 0, 0);
    
    switch (emitterShape)
    {
        case EmitterShape::Point:
            // Sem offset
            break;
        
        case EmitterShape::Sphere:
        {
            Vec3 randomDir = RandomUnitVector();
            float r = pow(Random(0.0f, 1.0f), 1.0f/3.0f) * radius;
            localOffset = randomDir * r;
            break;
        }
        
        case EmitterShape::Box:
        {
            float x = Random(-boxSize.x * 0.5f, boxSize.x * 0.5f);
            float y = Random(-boxSize.y * 0.5f, boxSize.y * 0.5f);
            float z = Random(-boxSize.z * 0.5f, boxSize.z * 0.5f);
            localOffset = Vec3(x, y, z);
            break;
        }
        
        case EmitterShape::Cone:
        {
            if (radius > 0.0f)
            {
                float angle = Random(0.0f, TwoPi);
                float r = sqrt(Random(0.0f, 1.0f)) * radius;
                
                Vec3 right = getRight(TransformSpace::Local);
                Vec3 up = getUp(TransformSpace::Local);
                
                localOffset = right * (cos(angle) * r) + up * (sin(angle) * r);
            }
            break;
        }
        
        case EmitterShape::Circle:
        {
            float angle = Random(0.0f, TwoPi);
            float r = sqrt(Random(0.0f, 1.0f)) * radius;
            
            Vec3 right = getRight(TransformSpace::Local);
            Vec3 up = getUp(TransformSpace::Local);
            
            localOffset = right * (cos(angle) * r) + up * (sin(angle) * r);
            break;
        }
        
        case EmitterShape::Ring:
        {
            float angle = Random(0.0f, TwoPi);
            float r = Random(innerRadius, radius);
            
            Vec3 right = getRight(TransformSpace::Local);
            Vec3 up = getUp(TransformSpace::Local);
            
            localOffset = right * (cos(angle) * r) + up * (sin(angle) * r);
            break;
        }
    }
    
    // Transformar offset para world space
    if (localOffset.lengthSquared() > 0.0001f)
    {
        Vec3 worldOffset = worldMat.TransformVector(localOffset);
        return worldPos + worldOffset;
    }
    
    return worldPos;
}

// ==========================================
// EMISSION VELOCITY (direção + spread)
// ==========================================

Vec3 ParticleSystem::GetEmissionVelocity()
{
    float speed = Random(speedMin, speedMax);
    Vec3 dir = GetWorldEmissionDirection();
    
    // Adicionar spread (cone de dispersão)
    if (spreadAngle > 0.0f)
    {
        float theta = Random(0.0f, TwoPi);
        float phi = Random(0.0f, spreadAngle);
        
        // Criar base ortonormal
        Vec3 up(0, 1, 0);
        if (fabs(dir.dot(up)) > 0.99f)
            up = Vec3(1, 0, 0);
        
        Vec3 right = Vec3::Cross(dir, up).normalized();
        up = Vec3::Cross(right, dir).normalized();
        
        // Aplicar rotação
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        dir = dir * cosPhi + (right * cosTheta + up * sinTheta) * sinPhi;
        dir.normalize();
    }
    
    return dir * speed;
}

// ==========================================
// UPDATE PARTICLES
// ==========================================

void ParticleSystem::UpdateParticles(float deltaTime)
{
    for (auto& p : particles)
    {
        if (!p.active)
            continue;
        
        // Update lifetime
        p.timeAlive += deltaTime;
        
        // Check death
        if (p.timeAlive >= p.lifetime)
        {
            p.active = false;
            continue;
        }
        
        // Physics
        p.velocity += p.acceleration * deltaTime;
        
        // Drag
        if (drag > 0.0f)
        {
            p.velocity *= (1.0f - drag * deltaTime);
        }
        
        p.position += p.velocity * deltaTime;
        
        // Rotation
        p.rotation += p.rotationSpeed * deltaTime;
        
        // Interpolate color
        float t = p.timeAlive / p.lifetime;
        p.color = Vec4(
            Lerp(p.colorStart.x, p.colorEnd.x, t),
            Lerp(p.colorStart.y, p.colorEnd.y, t),
            Lerp(p.colorStart.z, p.colorEnd.z, t),
            Lerp(p.colorStart.w, p.colorEnd.w, t)
        );
        
        // Interpolate size
        p.size = Vec2(
            Lerp(p.sizeStart.x, p.sizeEnd.x, t),
            Lerp(p.sizeStart.y, p.sizeEnd.y, t)
        );
        
        // Reset acceleration (affectors aplicam a cada frame)
        p.acceleration = gravity;
    }
}

// ==========================================
// APPLY AFFECTORS
// ==========================================

void ParticleSystem::ApplyAffectors(float deltaTime)
{
    if (affectors.empty())
        return;
    
    for (auto& p : particles)
    {
        if (!p.active)
            continue;
        
        for (auto* affector : affectors)
        {
            if (affector && affector->enabled)
            {
                affector->Apply(p, deltaTime);
            }
        }
    }
}

// ==========================================
// BUILD GEOMETRY (billboards)
// ==========================================

void ParticleSystem::BuildGeometry(const Mat4& view)
{
    vertices.clear();
    
    // Extrair right/up da view matrix (billboard)
    Vec3 right(view[0], view[4], view[8]);
    Vec3 up(view[1], view[5], view[9]);
    
    for (const auto& p : particles)
    {
        if (!p.active)
            continue;
        
        Vec3 particleRight = right;
        Vec3 particleUp = up;
        
        // Rotação
        if (p.rotation != 0.0f)
        {
            float c = cos(p.rotation);
            float s = sin(p.rotation);
            Vec3 newRight = particleRight * c - particleUp * s;
            Vec3 newUp = particleRight * s + particleUp * c;
            particleRight = newRight;
            particleUp = newUp;
        }
        
        Vec3 halfRight = particleRight * (p.size.x * 0.5f);
        Vec3 halfUp = particleUp * (p.size.y * 0.5f);
        
        // UVs do atlas
        float u0 = p.texRect.x;
        float v0 = p.texRect.y;
        float u1 = p.texRect.x + p.texRect.width;
        float v1 = p.texRect.y + p.texRect.height;
        
        // Bottom-left
        vertices.push_back(ParticleVertex(
            p.position - halfRight - halfUp,
            Vec2(u0, v0),
            p.color
        ));
        
        // Bottom-right
        vertices.push_back(ParticleVertex(
            p.position + halfRight - halfUp,
            Vec2(u1, v0),
            p.color
        ));
        
        // Top-right
        vertices.push_back(ParticleVertex(
            p.position + halfRight + halfUp,
            Vec2(u1, v1),
            p.color
        ));
        
        // Top-left
        vertices.push_back(ParticleVertex(
            p.position - halfRight + halfUp,
            Vec2(u0, v1),
            p.color
        ));
    }
}

// ==========================================
// HELPERS
// ==========================================

float ParticleSystem::Random(float min, float max)
{
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

Vec3 ParticleSystem::RandomUnitVector()
{
    // Método de Marsaglia para vetor uniforme na esfera
    float z = Random(-1.0f, 1.0f);
    float a = Random(0.0f, TwoPi);
    float r = sqrt(1.0f - z * z);
    return Vec3(r * cos(a), r * sin(a), z);
}

// ==========================================
// AFFECTOR IMPLEMENTATIONS
// ==========================================

void GravityAffector::Apply(Particle& particle, float deltaTime)
{
    if (enabled)
        particle.acceleration += gravity;
}

void DragAffector::Apply(Particle& particle, float deltaTime)
{
    if (enabled)
        particle.velocity *= (1.0f - drag * deltaTime);
}

void VortexAffector::Apply(Particle& particle, float deltaTime)
{
    if (!enabled) return;
    
    Vec3 toCenter = center - particle.position;
    float dist = toCenter.length();
    
    if (dist < radius && dist > 0.001f)
    {
        Vec3 dir = toCenter.normalized();
        Vec3 tangent(-dir.z, 0, dir.x);  // Perpendicular no plano XZ
        
        float falloff = 1.0f - dist / radius;
        particle.acceleration += tangent * strength * falloff;
    }
}

void AttractorAffector::Apply(Particle& particle, float deltaTime)
{
    if (!enabled) return;
    
    Vec3 toAttractor = position - particle.position;
    float dist = toAttractor.length();
    
    if (dist < radius && dist > 0.001f)
    {
        Vec3 dir = toAttractor.normalized();
        float falloff = 1.0f - dist / radius;
        float force = strength * falloff;
        
        if (repulse)
            force = -force;
        
        particle.acceleration += dir * force;
    }
}

void TurbulenceAffector::Apply(Particle& particle, float deltaTime)
{
    if (!enabled) return;
    
    time += deltaTime;
    
    // Pseudo-noise (simplex seria melhor mas isto funciona)
    float nx = sin(particle.position.x * frequency + time) * strength;
    float ny = sin(particle.position.y * frequency + time * 1.3f) * strength;
    float nz = cos(particle.position.z * frequency + time * 0.7f) * strength;
    
    particle.acceleration += Vec3(nx, ny, nz);
}

void ColorOverLifetimeAffector::Apply(Particle& particle, float deltaTime)
{
    if (!enabled) return;
    
    float t = particle.timeAlive / particle.lifetime;
    
    particle.color = Vec4(
        Lerp(startColor.x, endColor.x, t),
        Lerp(startColor.y, endColor.y, t),
        Lerp(startColor.z, endColor.z, t),
        Lerp(startColor.w, endColor.w, t)
    );
}

void SizeOverLifetimeAffector::Apply(Particle& particle, float deltaTime)
{
    if (!enabled) return;
    
    float t = particle.timeAlive / particle.lifetime;
    
    particle.size = Vec2(
        Lerp(startSize.x, endSize.x, t),
        Lerp(startSize.y, endSize.y, t)
    );
}
