#pragma once

#define SMALL_FLOAT 0.000001f

const unsigned int MaxUInt32 = 0xFFFFFFFF;
const int MinInt32 = 0x80000000;
const int MaxInt32 = 0x7FFFFFFF;
const float MaxFloat = 3.402823466e+38F;
const float MinPosFloat = 1.175494351e-38F;

const float Pi = 3.141592654f;
const float TwoPi = 6.283185307f;
const float PiHalf = 1.570796327f;

const float Epsilon = 0.000001f;
const float ZeroEpsilon = 32.0f * MinPosFloat; // Very small epsilon for checking against 0.0f

const float M_INFINITY = 1.0e30f;

#define powi(base, exp) (int)powf((float)(base), (float)(exp))

#define ToRadians(x) (float)(((x) * Pi / 180.0f))
#define ToDegrees(x) (float)(((x) * 180.0f / Pi))
 

inline float Sin(float a) { return sin(a * Pi / 180); }
inline float Cos(float a) { return cos(a * Pi / 180); }
inline float Tan(float a) { return tan(a * Pi / 180); }
inline float SinRad(float a) { return sin(a); }
inline float CosRad(float a) { return cos(a); }
inline float TanRad(float a) { return tan(a); }
inline float ASin(float a) { return asin(a) * 180 / Pi; }
inline float ACos(float a) { return acos(a) * 180 / Pi; }
inline float ATan(float a) { return atan(a) * 180 / Pi; }
inline float ATan2(float y, float x) { return atan2(y, x) * 180 / Pi; }
inline float ASinRad(float a) { return asin(a); }
inline float ACosRad(float a) { return acos(a); }
inline float ATanRad(float a) { return atan(a); }
inline float ATan2Rad(float y, float x) { return atan2(y, x); }
inline int Floor(float a) { return (int)(floor(a)); }
inline int Ceil(float a) { return (int)(ceil(a)); }
inline int Trunc(float a)
{
    if (a > 0)
        return Floor(a);
    else
        return Ceil(a);
}
inline int Round(float a)
{
    if (a < 0)
        return (int)(ceil(a - 0.5f));
    else
        return (int)(floor(a + 0.5f));
}
inline float Sqrt(float a)
{
    if (a > 0)
        return sqrt(a);
    else
        return 0;
}
inline float Abs(float a)
{
    if (a < 0)
        a = -a;
    return a;
}
inline int Mod(int a, int b)
{
    if (b == 0)
        return 0;
    return a % b;
}
inline float FMod(float a, float b)
{
    if (b == 0)
        return 0;
    return fmod(a, b);
}
inline float Pow(float a, float b) { return pow(a, b); }
inline int Sign(float a)
{
    if (a < 0)
        return -1;
    else if (a > 0)
        return 1;
    else
        return 0;
}
inline float Min(float a, float b) { return a < b ? a : b; }
inline float Max(float a, float b) { return a > b ? a : b; }
inline int Min(int a, int b) { return a < b ? a : b; }
inline int Max(int a, int b) { return a > b ? a : b; }
inline float Clamp(float a, float min, float max)
{
    if (a < min)
        a = min;
    else if (a > max)
        a = max;
    return a;
}
inline int Clamp(int a, int min, int max)
{
    if (a < min)
        a = min;
    else if (a > max)
        a = max;
    return a;
}

template <typename T>
struct Rectangle
{

    T x;
    T y;
    T width;
    T height;

    Rectangle() : x(0), y(0), width(0), height(0) {}
    Rectangle(T x, T y, T width, T height)
        : x(x), y(y), width(width), height(height)
    {
    }
    Rectangle(const Rectangle &rect)
        : x(rect.x), y(rect.y), width(rect.width), height(rect.height)
    {
    }

    void Set(T x, T y, T width, T height)
    {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
    }

    void Merge(const Rectangle &rect)
    {
        T right = x + width;
        T bottom = y + height;
        T rectRight = rect.x + rect.width;
        T rectBottom = rect.y + rect.height;
        x = Min(x, rect.x);
        y = Min(y, rect.y);
        right = Max(right, rectRight);
        bottom = Max(bottom, rectBottom);
        width = right - x;
        height = bottom - y;
    }

    void Clear()
    {
        x = 0;
        y = 0;
        width = 0;
        height = 0;
    }

    bool operator==(const Rectangle &other)
    {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }
    bool operator!=(const Rectangle &other)
    {
        return !(*this == other);
    }

    Rectangle &operator=(const Rectangle &rect)
    {
        if (this == &rect)
            return *this;
        x = rect.x;
        y = rect.y;
        width = rect.width;
        height = rect.height;
        return *this;
    }
};

template <typename T>
struct Size
{
    T width;
    T height;

    Size() : width(0), height(0) {}
    Size(T w, T h) : width(w), height(h) {}
    Size(const Size &size) : width(size.width), height(size.height) {}

    Size &operator=(const Size &size)
    {
        if (this == &size)
            return *this;
        width = size.width;
        height = size.height;
        return *this;
    }
    bool operator==(const Size &other)
    {
        return width == other.width && height == other.height;
    }
    bool operator!=(const Size &other)
    {
        return !(*this == other);
    }
};

typedef Rectangle<int> IntRect;
typedef Rectangle<float> FloatRect;
typedef Size<int> IntSize;
typedef Size<float> FloatSize;




class Quat;
class Mat4;
class Mat3;

class Vec2
{
public:
    float x, y;

    // Construtores
    Vec2();
    Vec2(float x, float y);
    explicit Vec2(float scalar);

    // Acesso por índice
    float &operator[](int index);
    const float &operator[](int index) const;

    // Operadores aritméticos
    Vec2 operator+(const Vec2 &other) const;
    Vec2 operator-(const Vec2 &other) const;
    Vec2 operator*(const Vec2 &other) const;
    Vec2 operator/(const Vec2 &other) const;

    Vec2 operator*(float scalar) const;
    Vec2 operator/(float scalar) const;

    // Operadores compostos
    Vec2 &operator+=(const Vec2 &other);
    Vec2 &operator-=(const Vec2 &other);
    Vec2 &operator*=(const Vec2 &other);
    Vec2 &operator/=(const Vec2 &other);
    Vec2 &operator*=(float scalar);
    Vec2 &operator/=(float scalar);

    // Operador unário
    Vec2 operator-() const;

    // Comparação
    bool operator==(const Vec2 &other) const;
    bool operator!=(const Vec2 &other) const;

    // Métodos úteis
    float lengthSquared() const;
    float length() const;
    Vec2 normalized() const;
    void normalize();
    float dot(const Vec2 &other) const;
    float cross(const Vec2 &other) const; // Retorna scalar (componente z em 3D)

    // Funções de ângulo
    float angle() const;                                        // Ângulo em radianos em relação ao eixo X
    float angleDeg() const;                                     // Ângulo em graus em relação ao eixo X
    static float AngleBetween(const Vec2 &a, const Vec2 &b);    // Ângulo entre vetores (radianos)
    static float AngleBetweenDeg(const Vec2 &a, const Vec2 &b); // Ângulo entre vetores (graus)
    Vec2 rotate(float angleRad) const;                          // Roda o vetor por um ângulo em radianos
    Vec2 rotateDeg(float angleDeg) const;                       // Roda o vetor por um ângulo em graus
    static Vec2 FromAngle(float angleRad);                      // Cria vetor unitário a partir de ângulo
    static Vec2 FromAngleDeg(float angleDeg);                   // Cria vetor unitário a partir de ângulo em graus

    // Funções estáticas
    static float Dot(const Vec2 &a, const Vec2 &b);
    static float Distance(const Vec2 &a, const Vec2 &b);
    static float DistanceSquared(const Vec2 &a, const Vec2 &b);
    static Vec2 Lerp(const Vec2 &a, const Vec2 &b, float t);
    static Vec2 Min(const Vec2 &a, const Vec2 &b);
    static Vec2 Max(const Vec2 &a, const Vec2 &b);
};

// Scalar * Vec2
Vec2 operator*(float scalar, const Vec2 &vec);

 

class Vec3
{
public:
    float x, y, z;

    // Construtores
    Vec3();
    Vec3(float x, float y, float z);
    explicit Vec3(float scalar);
    Vec3(const Vec2 &xy, float z);

    // Acesso por índice
    float &operator[](int index);
    const float &operator[](int index) const;

    // Operadores aritméticos
    Vec3 operator+(const Vec3 &other) const;
    Vec3 operator-(const Vec3 &other) const;
    Vec3 operator*(const Vec3 &other) const;
    Vec3 operator/(const Vec3 &other) const;

    Vec3 operator*(float scalar) const;
    Vec3 operator/(float scalar) const;

    // Operadores compostos
    Vec3 &operator+=(const Vec3 &other);
    Vec3 &operator-=(const Vec3 &other);
    Vec3 &operator*=(const Vec3 &other);
    Vec3 &operator/=(const Vec3 &other);
    Vec3 &operator*=(float scalar);
    Vec3 &operator/=(float scalar);

    // Operador unário
    Vec3 operator-() const;

    // Comparação
    bool operator==(const Vec3 &other) const;
    bool operator!=(const Vec3 &other) const;

    // Métodos úteis
    float lengthSquared() const;
    float length() const;
    Vec3 normalized() const;
    void normalize();
    float dot(const Vec3 &other) const;
    Vec3 cross(const Vec3 &other) const;

    // Funções de ângulo
    static float AngleBetween(const Vec3 &a, const Vec3 &b);    // Ângulo entre vetores (radianos)
    static float AngleBetweenDeg(const Vec3 &a, const Vec3 &b); // Ângulo entre vetores (graus)
    Vec3 rotateX(float angleRad) const;                         // Roda em torno do eixo X
    Vec3 rotateY(float angleRad) const;                         // Roda em torno do eixo Y
    Vec3 rotateZ(float angleRad) const;                         // Roda em torno do eixo Z
    Vec3 rotateXDeg(float angleDeg) const;                      // Roda em torno do eixo X (graus)
    Vec3 rotateYDeg(float angleDeg) const;                      // Roda em torno do eixo Y (graus)
    Vec3 rotateZDeg(float angleDeg) const;                      // Roda em torno do eixo Z (graus)

    // Funções estáticas
    static float Dot(const Vec3 &a, const Vec3 &b);
    static Vec3 Cross(const Vec3 &a, const Vec3 &b);
    static float Distance(const Vec3 &a, const Vec3 &b);
    static float DistanceSquared(const Vec3 &a, const Vec3 &b);
    static Vec3 Lerp(const Vec3 &a, const Vec3 &b, float t);
    static Vec3 Min(const Vec3 &a, const Vec3 &b);
    static Vec3 Max(const Vec3 &a, const Vec3 &b);
};

// Scalar * Vec3
Vec3 operator*(float scalar, const Vec3 &vec);

 

class Vec4
{
public:
    float x, y, z, w;

    // Construtores
    Vec4();
    Vec4(float x, float y, float z, float w);
    explicit Vec4(float scalar);
    Vec4(const Vec2 &xy, float z, float w);
    Vec4(const Vec3 &xyz, float w);

    // Acesso por índice
    float &operator[](int index);
    const float &operator[](int index) const;

    // Operadores aritméticos
    Vec4 operator+(const Vec4 &other) const;
    Vec4 operator-(const Vec4 &other) const;
    Vec4 operator*(const Vec4 &other) const;
    Vec4 operator/(const Vec4 &other) const;

    Vec4 operator*(float scalar) const;
    Vec4 operator/(float scalar) const;

    // Operadores compostos
    Vec4 &operator+=(const Vec4 &other);
    Vec4 &operator-=(const Vec4 &other);
    Vec4 &operator*=(const Vec4 &other);
    Vec4 &operator/=(const Vec4 &other);
    Vec4 &operator*=(float scalar);
    Vec4 &operator/=(float scalar);

    // Operador unário
    Vec4 operator-() const;

    // Comparação
    bool operator==(const Vec4 &other) const;
    bool operator!=(const Vec4 &other) const;

    // Métodos úteis
    float lengthSquared() const;
    float length() const;
    Vec4 normalized() const;
    void normalize();
    float dot(const Vec4 &other) const;

    // Funções estáticas
    static float Dot(const Vec4 &a, const Vec4 &b);
    static float Distance(const Vec4 &a, const Vec4 &b);
    static float DistanceSquared(const Vec4 &a, const Vec4 &b);
    static Vec4 Lerp(const Vec4 &a, const Vec4 &b, float t);
    static Vec4 Min(const Vec4 &a, const Vec4 &b);
    static Vec4 Max(const Vec4 &a, const Vec4 &b);
};

// Scalar * Vec4
Vec4 operator*(float scalar, const Vec4 &vec);

 

// ==================== Mat3 ====================

class Mat3
{
public:
    float m[9]; // Column-major order (como OpenGL)

    // Construtores
    Mat3();               // Identidade
    Mat3(float diagonal); // Matriz diagonal
    Mat3(float m0, float m1, float m2,
         float m3, float m4, float m5,
         float m6, float m7, float m8);

    // Acesso por índice
    float &operator[](int index);
    const float &operator[](int index) const;
    float &operator()(int row, int col); // Acesso por linha e coluna
    const float &operator()(int row, int col) const;

    // Operadores aritméticos
    Mat3 operator+(const Mat3 &other) const;
    Mat3 operator-(const Mat3 &other) const;
    Mat3 operator*(const Mat3 &other) const;
    Mat3 operator*(float scalar) const;
    Mat3 operator/(float scalar) const;



    // Transformação de vetores
    Vec3 operator*(const Vec3 &vec) const;
    Vec2 operator*(const Vec2 &vec) const; // Trata como (x, y, 1)

    // Operadores compostos
    Mat3 &operator+=(const Mat3 &other);
    Mat3 &operator-=(const Mat3 &other);
    Mat3 &operator*=(const Mat3 &other);
    Mat3 &operator*=(float scalar);
    Mat3 &operator/=(float scalar);

    // Comparação
    bool operator==(const Mat3 &other) const;
    bool operator!=(const Mat3 &other) const;

    // Operações de matriz
    Mat3 transposed() const;
    void transpose();
    float determinant() const;
    Mat3 inverse() const;

    // Funções de criação de transformações
    static Mat3 Identity();
    static Mat3 Scale(float sx, float sy);
    static Mat3 Scale(const Vec2 &scale);
    static Mat3 Rotation(float angleRad);
    static Mat3 RotationDeg(float angleDeg);
    static Mat3 Translation(float tx, float ty);
    static Mat3 Translation(const Vec2 &translation);
};

// Scalar * Mat3
Mat3 operator*(float scalar, const Mat3 &mat);

 

// ==================== Mat4 ====================

class Mat4
{
public:
    float m[16]; // Column-major order (como OpenGL)

    // Construtores
    Mat4();               // Identidade
    Mat4(float diagonal); // Matriz diagonal
    Mat4(float m0, float m1, float m2, float m3,
         float m4, float m5, float m6, float m7,
         float m8, float m9, float m10, float m11,
         float m12, float m13, float m14, float m15);

    // Acesso por índice
    float &operator[](int index);
    const float &operator[](int index) const;
    float &operator()(int row, int col); // Acesso por linha e coluna
    const float &operator()(int row, int col) const;

    // Operadores aritméticos
    Mat4 operator+(const Mat4 &other) const;
    Mat4 operator-(const Mat4 &other) const;
    Mat4 operator*(const Mat4 &other) const;
    Mat4 operator*(float scalar) const;
    Mat4 operator/(float scalar) const;

    // Transformação de vetores
    Vec4 operator*(const Vec4 &vec) const;
    Vec3 operator*(const Vec3 &vec) const; // Trata como (x, y, z, 1), divide por w

    // Operadores compostos
    Mat4 &operator+=(const Mat4 &other);
    Mat4 &operator-=(const Mat4 &other);
    Mat4 &operator*=(const Mat4 &other);
    Mat4 &operator*=(float scalar);
    Mat4 &operator/=(float scalar);

    // Comparação
    bool operator==(const Mat4 &other) const;
    bool operator!=(const Mat4 &other) const;

    // Operações de matriz
    Mat4 transposed() const;
    void transpose();
    float determinant() const;
    Mat4 inverse() const;

    Vec3 TransformPoint(const Vec3& point) const;
    Vec3 TransformVector(const Vec3& vec) const;

    // Funções de criação de transformações
    static Mat4 Identity();
    static Mat4 Inverse(const Mat4 &mat);
    static Mat4 Scale(float sx, float sy, float sz);
    static Mat4 Scale(const Vec3 &scale);
    static Mat4 Translation(float tx, float ty, float tz);
    static Mat4 Translation(const Vec3 &translation);
    static Mat4 RotationX(float angleRad);
    static Mat4 RotationY(float angleRad);
    static Mat4 RotationZ(float angleRad);
    static Mat4 RotationXDeg(float angleDeg);
    static Mat4 RotationYDeg(float angleDeg);
    static Mat4 RotationZDeg(float angleDeg);
    static Mat4 Rotation(const Vec3 &axis, float angleRad);
    static Mat4 RotationDeg(const Vec3 &axis, float angleDeg);
    static void DecomposeMatrix(const Mat4& matrix, Vec3* outPosition, Quat* outRotation);

    // Funções para câmera/projeção
    static Mat4 LookAt(const Vec3 &eye, const Vec3 &center, const Vec3 &up);
    static Mat4 Perspective(float fovYRad, float aspect, float near, float far);
    static Mat4 PerspectiveDeg(float fovYDeg, float aspect, float near, float far);
    static Mat4 Ortho(float left, float right, float bottom, float top, float near, float far);

    // Funções auxiliares para Cascaded Shadow Maps
    void getFrustumCorners(Vec3 corners[8]) const;                              // Extrai os 8 cantos do frustum
    static Mat4 OrthoFromCorners(const Vec3 corners[8], const Mat4 &lightView); // Cria ortho que engloba corners
};

// Scalar * Mat4
Mat4 operator*(float scalar, const Mat4 &mat);
 

// ==================== Funções auxiliares para CSM ====================

// BoundingBox (Axis-Aligned Bounding Box)
struct BoundingBox
{
    Vec3 min;
    Vec3 max;

    BoundingBox();
    BoundingBox(const Vec3 &min, const Vec3 &max);

    void expand(const Vec3 &point);
    void expand(const BoundingBox &other);
    Vec3 center() const;
    Vec3 size() const;
    bool contains(const Vec3 &point) const;
};

// Funções para Cascaded Shadow Maps
namespace CSM
{
    // Calcula splits para cascatas usando esquema logarítmico/linear
    void calculateCascadeSplits(float near, float far, int numCascades, float lambda, float *outSplits);

    // Cria matriz de projeção perspective para uma cascade específica
    Mat4 createCascadeProjection(float fovYRad, float aspect, float nearSplit, float farSplit);

    // Extrai frustum corners de projection * view matrix
    void extractFrustumCorners(const Mat4 &projView, Vec3 corners[8]);

    // Calcula BoundingBox a partir de pontos
    BoundingBox calculateAABB(const Vec3 *points, int count);

    // Cria matriz ortho que engloba os corners no espaço da luz
    Mat4 createLightOrtho(const Vec3 corners[8], const Mat4 &lightView, float *outNear, float *outFar);
}

// ==================== Quat ====================

class Quat
{
public:
    float x, y, z, w; // w é a parte escalar

    // Construtores
    Quat(); // Identidade (0, 0, 0, 1)
    Quat(float x, float y, float z, float w);
    Quat(const Vec3 &axis, float angleRad);    // De eixo-ângulo
    explicit Quat(const Vec3 &eulerAnglesRad); // De Euler angles (pitch, yaw, roll)

    // Acesso por índice
    float &operator[](int index);
    const float &operator[](int index) const;

    // Operadores aritméticos
    Quat operator+(const Quat &other) const;
    Quat operator-(const Quat &other) const;
    Quat operator*(const Quat &other) const; // Multiplicação de quaternions
    Quat operator*(float scalar) const;
    Quat operator/(float scalar) const;

    // Rotação de vetores
    Vec3 operator*(const Vec3 &vec) const;

    // Operadores compostos
    Quat &operator+=(const Quat &other);
    Quat &operator-=(const Quat &other);
    Quat &operator*=(const Quat &other);
    Quat &operator*=(float scalar);
    Quat &operator/=(float scalar);

    // Operador unário
    Quat operator-() const;

    // Comparação
    bool operator==(const Quat &other) const;
    bool operator!=(const Quat &other) const;

    // Operações de quaternion
    float lengthSquared() const;
    float length() const;
    Quat normalized() const;
    void normalize();
    Quat conjugate() const;
    Quat inverse() const;
    float dot(const Quat &other) const;

    // Conversão
    Mat4 toMat4() const;
    Mat3 toMat3() const;
    Vec3 toEulerAngles() const; // Retorna (pitch, yaw, roll) em radianos
    Vec3 toEulerAnglesDeg() const;

    // Funções de criação
    static Quat Identity();
    static Quat FromAxisAngle(const Vec3 &axis, float angleRad);
    static Quat FromAxisAngleDeg(const Vec3 &axis, float angleDeg);
    static Quat FromEulerAngles(float pitch, float yaw, float roll); // Em radianos
    static Quat FromEulerAnglesDeg(float pitch, float yaw, float roll);
    static Quat FromEulerAngles(const Vec3 &eulerRad);
    static Quat FromEulerAnglesDeg(const Vec3 &eulerDeg);
    static Quat FromMat4(const Mat4 &mat);
    static Quat FromMat3(const Mat3 &mat);

    // Interpolação
    static float Dot(const Quat &a, const Quat &b);
    static Quat Lerp(const Quat &a, const Quat &b, float t);  // Linear
    static Quat Nlerp(const Quat &a, const Quat &b, float t); // Normalized linear
    static Quat Slerp(const Quat &a, const Quat &b, float t); // Spherical linear

    // Rotações básicas
    static Quat RotationX(float angleRad);
    static Quat RotationY(float angleRad);
    static Quat RotationZ(float angleRad);
    static Quat RotationXDeg(float angleDeg);
    static Quat RotationYDeg(float angleDeg);
    static Quat RotationZDeg(float angleDeg);
};

// Scalar * Quat
Quat operator*(float scalar, const Quat &quat);
