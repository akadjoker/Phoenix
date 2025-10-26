#pragma once
#include <vector>
#include "Config.hpp"
#include "Math.hpp"
#include "glad/glad.h"

class Color;
class Texture;
class Shader;


struct BatchVertexBuffer 
{
    int elementCount;          
    std::vector<float>  vertices;        
    std::vector<float>  texcoords;           
    std::vector<unsigned char> colors;     
    std::vector<unsigned int>  indices;    
    unsigned int vaoId;         
    unsigned int vboId[4];
} ;

struct DrawCall 
{
    int mode;                   
    int vertexCount;          
    int vertexAlignment;       
   unsigned int textureId;
};



class    RenderBatch  
{
public:
        
    RenderBatch( );
    virtual ~RenderBatch();

    void Release();
    

    void Init(int numBuffers = 1, int bufferElements = 10000);
 

    void SetColor(const Color &color);
    void SetColor(float r , float g , float b);

    
    void SetColor(u8 r, u8 g, u8 b, u8 a);

    void SetAlpha(float a);

    void Line2D(int startPosX, int startPosY, int endPosX, int endPosY);
    void Line2D(const Vec2 &start,const Vec2 &end);

    void Line3D(const Vec3 &start, const Vec3 &end);
    void Line3D(float startX, float startY, float startZ, float endX, float endY, float endZ);


    void Circle(int centerX, int centerY, float radius,  bool fill = false);
    void Rectangle(int posX, int posY, int width, int height, bool fill = false);
   
    

    void Box(const BoundingBox &box);
    void Box(const BoundingBox &box,const Mat4 &transform);

    void Cube(const Vec3 &position, float width, float height, float depth,bool wire=true);
    void Sphere(const Vec3 &position, float radius, int rings, int slices,bool wire=true);
    void Cone(const Vec3& position, float radius, float height, int segments, bool wire);
    void Cylinder(const Vec3& position, float radius, float height, int segments, bool wire);
    void Capsule(const Vec3& position, float radius, float height, int segments, bool wire);

    void TriangleLines(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3);

    void Triangle(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3);
    void Triangle(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, const Vec2 &t1, const Vec2 &t2, const Vec2 &t3);


    void Grid(int slices, float spacing,bool axes=true);
    
    void Quad(const Vec2 *coords, const Vec2 *texcoords);
    void Quad(Texture *texture, const Vec2 *coords, const Vec2 *texcoords);
    void Quad(Texture *texture, float x, float y,float width, float height);
    void Quad(Texture *texture, const FloatRect &src,float x, float y,float width, float height);
    void Quad(u32 texture, float x, float y,float width, float height);

    void BeginTransform(const Mat4 &transform);
    void EndTransform();

   
    void Render();

    void SetMode(int mode);                        
       

          
    void Vertex2f(float x, float y);          
    void Vertex3f(float x, float y, float z);     
    void TexCoord2f(float x, float y);          

    void SetTexture(unsigned int id);

    void SetTexture(Texture *texture);

    void SetMatrix(const Mat4 &matrix);


    private:
        bool CheckRenderBatchLimit(int vCount);


    int bufferCount;            
    int currentBuffer;         
    int drawCounter;           
    float currentDepth;         
    int vertexCounter;
    s32 defaultTextureId;
    bool use_matrix;
    Mat4 modelMatrix;
    Mat4 viewMatrix;


    Texture *m_defaultTexture;

    std::vector<DrawCall*> draws;
    std::vector<BatchVertexBuffer*> vertexBuffer;

    float texcoordx, texcoordy;         
    u8 colorr, colorg, colorb, colora;
    Shader* shader;

private:




    

};

