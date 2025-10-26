#include "pch.h"
#include "Batch.hpp"
#include "Color.hpp"
#include "Shader.hpp"
#include "Texture.hpp"


#define BATCH_DRAWCALLS 256


#define LINES 0x0001
#define TRIANGLES 0x0004
#define QUAD 0x0008


RenderBatch::RenderBatch()
{

    colorr = 255;
    colorg = 255;
    colorb = 255;
    colora = 255;
    texcoordx = 0.0f;
    texcoordy = 0.0f;
    currentBuffer = 0;
    vertexCounter = 0;
    currentDepth = -1.0f;
    defaultTextureId = 0;
    bufferCount = 0;
    drawCounter = 1;
    use_matrix = false;
    modelMatrix= Mat4::Identity();
}
void RenderBatch::Init(int numBuffers, int bufferElements)
{

    shader= ShaderManager::Instance().Get("2DShader");

    Texture* m_defaultTexture = TextureManager::Instance().GetDefault();
    defaultTextureId = m_defaultTexture->GetHandle();


    

    for (int i = 0; i < numBuffers; i++)
    {
        vertexBuffer.push_back(new BatchVertexBuffer());
    }

    vertexCounter = 0;
    currentBuffer = 0;


    for (int i = 0; i < numBuffers; i++)
    {
        vertexBuffer[i]->elementCount = bufferElements;

        int k = 0;


        for (int j = 0; j <= bufferElements; j++)
        {

            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->texcoords.push_back(0.0f);
            vertexBuffer[i]->texcoords.push_back(0.0f);
            vertexBuffer[i]->colors.push_back(colorr);
            vertexBuffer[i]->colors.push_back(colorg);
            vertexBuffer[i]->colors.push_back(colorb);
            vertexBuffer[i]->colors.push_back(colora);

            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->texcoords.push_back(0.0f);
            vertexBuffer[i]->texcoords.push_back(0.0f);
            vertexBuffer[i]->colors.push_back(colorr);
            vertexBuffer[i]->colors.push_back(colorg);
            vertexBuffer[i]->colors.push_back(colorb);
            vertexBuffer[i]->colors.push_back(colora);

            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->texcoords.push_back(0.0f);
            vertexBuffer[i]->texcoords.push_back(0.0f);
            vertexBuffer[i]->colors.push_back(colorr);
            vertexBuffer[i]->colors.push_back(colorg);
            vertexBuffer[i]->colors.push_back(colorb);
            vertexBuffer[i]->colors.push_back(colora);

            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->vertices.push_back(0.0f);
            vertexBuffer[i]->texcoords.push_back(0.0f);
            vertexBuffer[i]->texcoords.push_back(0.0f);
            vertexBuffer[i]->colors.push_back(colorr);
            vertexBuffer[i]->colors.push_back(colorg);
            vertexBuffer[i]->colors.push_back(colorb);
            vertexBuffer[i]->colors.push_back(colora);


            vertexBuffer[i]->indices.push_back(k);
            vertexBuffer[i]->indices.push_back(k + 1);
            vertexBuffer[i]->indices.push_back(k + 2);
            vertexBuffer[i]->indices.push_back(k);
            vertexBuffer[i]->indices.push_back(k + 2);
            vertexBuffer[i]->indices.push_back(k + 3);


            k += 4;
        }
    }


    for (int i = 0; i < numBuffers; i++)
    {
        glGenVertexArrays(1, &vertexBuffer[i]->vaoId);
        glBindVertexArray(vertexBuffer[i]->vaoId);

        glGenBuffers(1, &vertexBuffer[i]->vboId[0]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[i]->vboId[0]);
        glBufferData(GL_ARRAY_BUFFER,
                     vertexBuffer[i]->vertices.size() * sizeof(float),
                     vertexBuffer[i]->vertices.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);

        glGenBuffers(1, &vertexBuffer[i]->vboId[1]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[i]->vboId[1]);
        glBufferData(GL_ARRAY_BUFFER,
                     vertexBuffer[i]->texcoords.size() * sizeof(float),
                     vertexBuffer[i]->texcoords.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, 0);


        glGenBuffers(1, &vertexBuffer[i]->vboId[2]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[i]->vboId[2]);
        glBufferData(GL_ARRAY_BUFFER,
                     vertexBuffer[i]->colors.size() * sizeof(unsigned char),
                     vertexBuffer[i]->colors.data(), GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);


        glGenBuffers(1, &vertexBuffer[i]->vboId[3]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBuffer[i]->vboId[3]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     vertexBuffer[i]->indices.size() * sizeof(unsigned int),
                     vertexBuffer[i]->indices.data(), GL_STATIC_DRAW);
    }


    glBindVertexArray(0);


    for (int i = 0; i < BATCH_DRAWCALLS; i++)
    {
        draws.push_back(new DrawCall());
        draws[i]->mode = QUAD;
        draws[i]->vertexCount = 0;
        draws[i]->vertexAlignment = 0;
        draws[i]->textureId = defaultTextureId;
    }

    bufferCount = numBuffers; // Record buffer count
    drawCounter = 1; // Reset draws counter
    currentDepth = -1.0f; // Reset depth value
}


void UnloadVertexArray(unsigned int vaoId)
{
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vaoId);
}


void RenderBatch::Release()
{
    if (vertexBuffer.size() == 0) return;

    for (int i = 0; i < (int)vertexBuffer.size(); i++)
    {
        glDeleteBuffers(1, &vertexBuffer[i]->vboId[0]);
        glDeleteBuffers(1, &vertexBuffer[i]->vboId[1]);
        glDeleteBuffers(1, &vertexBuffer[i]->vboId[2]);
        glDeleteBuffers(1, &vertexBuffer[i]->vboId[3]);
        UnloadVertexArray(vertexBuffer[i]->vaoId);
    }
    for (int i = 0; i < (int)draws.size(); i++)
    {

        delete draws[i];
    }
    for (int i = 0; i < (int)vertexBuffer.size(); i++)
    {
        vertexBuffer[i]->vertices.clear();
        vertexBuffer[i]->texcoords.clear();
        vertexBuffer[i]->colors.clear();
        vertexBuffer[i]->indices.clear();

        delete vertexBuffer[i];
    }
    draws.clear();
    vertexBuffer.clear();
    m_defaultTexture = nullptr; 
    shader=nullptr;
    LogInfo("Render batch  unloaded successfully from VRAM (GPU)");
}

RenderBatch::~RenderBatch() { Release(); }


 


void RenderBatch::Render()
{

    if (vertexCounter > 0)
    {
        glBindVertexArray(vertexBuffer[currentBuffer]->vaoId);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[currentBuffer]->vboId[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCounter * 3 * sizeof(float),
                        vertexBuffer[currentBuffer]->vertices.data());

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[currentBuffer]->vboId[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCounter * 2 * sizeof(float),
                        vertexBuffer[currentBuffer]->texcoords.data());

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer[currentBuffer]->vboId[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        vertexCounter * 4 * sizeof(unsigned char),
                        vertexBuffer[currentBuffer]->colors.data());

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glActiveTexture(GL_TEXTURE0);
        if (shader)
        {
        shader->Bind();
        shader->SetUniformMat4("mvp", viewMatrix.m);
        }

        unsigned int currentTex = 0xFFFFFFFFu;
        for (int i = 0, vertexOffset = 0; i < drawCounter; ++i)
        {
            // bind de textura só quando muda
            if (draws[i]->textureId != currentTex)
            {
                currentTex = draws[i]->textureId;
                glBindTexture(GL_TEXTURE_2D, currentTex);
            }

            const int mode = (draws[i]->mode == LINES) ? GL_LINES
                : (draws[i]->mode == TRIANGLES)
                ? GL_TRIANGLES
                : GL_TRIANGLES; // QUAD -> indices

            if (draws[i]->mode == LINES || draws[i]->mode == TRIANGLES)
            {
                glDrawArrays(mode, vertexOffset, draws[i]->vertexCount);
            }
            else
            { // QUAD
                const int firstIndex = (vertexOffset / 4) * 6;
                const int count = (draws[i]->vertexCount / 4) * 6;
                glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT,
                               (GLvoid *)(firstIndex * sizeof(unsigned int)));
            }

            vertexOffset += (draws[i]->vertexCount + draws[i]->vertexAlignment);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);

    }

    // reset do batch
    vertexCounter = 0;
    currentDepth = -1.0f;
    for (int i = 0; i < BATCH_DRAWCALLS; ++i)
    {
        draws[i]->mode = QUAD;
        draws[i]->vertexCount = 0;
        draws[i]->textureId = defaultTextureId;
    }
    drawCounter = 1;

    if (++currentBuffer >= bufferCount) currentBuffer = 0;
}

void RenderBatch::Line3D(float startX, float startY, float startZ, float endX,
                         float endY, float endZ)
{
    SetMode(LINES);
    Vertex3f(startX, startY, startZ);
    Vertex3f(endX, endY, endZ);
}

bool RenderBatch::CheckRenderBatchLimit(int vCount)
{
    bool overflow = false;


    if ((vertexCounter + vCount)>= (vertexBuffer[currentBuffer]->elementCount * 4))
    {
        overflow = true;

        // Store current primitive drawing mode and texture id
        int currentMode = draws[drawCounter - 1]->mode;
        int currentTexture = draws[drawCounter - 1]->textureId;

        Render();

        // Restore state of last batch so we can continue adding vertices
        draws[drawCounter - 1]->mode = currentMode;
        draws[drawCounter - 1]->textureId = currentTexture;
    }


    return overflow;
}


void RenderBatch::SetMode(int mode)
{
    if (draws[drawCounter - 1]->mode != mode)
    {
        if (draws[drawCounter - 1]->vertexCount > 0)
        {
            if (draws[drawCounter - 1]->mode == LINES)
                draws[drawCounter - 1]->vertexAlignment =
                    ((draws[drawCounter - 1]->vertexCount < 4)
                         ? draws[drawCounter - 1]->vertexCount
                         : draws[drawCounter - 1]->vertexCount % 4);
            else if (draws[drawCounter - 1]->mode == TRIANGLES)
                draws[drawCounter - 1]->vertexAlignment =
                    ((draws[drawCounter - 1]->vertexCount < 4)
                         ? 1
                         : (4 - (draws[drawCounter - 1]->vertexCount % 4)));
            else
                draws[drawCounter - 1]->vertexAlignment = 0;

            if (!CheckRenderBatchLimit(draws[drawCounter - 1]->vertexAlignment))
            {
                vertexCounter += draws[drawCounter - 1]->vertexAlignment;
                drawCounter++;
            }
        }

        if (drawCounter >= BATCH_DRAWCALLS) Render();

        draws[drawCounter - 1]->mode = mode;
        draws[drawCounter - 1]->vertexCount = 0;
        draws[drawCounter - 1]->textureId = defaultTextureId;
    }
}


void RenderBatch::BeginTransform(const Mat4 &transform)
{
    use_matrix = true;

    modelMatrix = transform;
}

void RenderBatch::SetMatrix(const Mat4 &matrix) { viewMatrix = matrix; }


void RenderBatch::EndTransform() { use_matrix = false; }

void RenderBatch::Vertex3f(float x, float y, float z)
{
    float tx = x;
    float ty = y;
    float tz = z;

    if (use_matrix)
    {
        tx = modelMatrix[0] * x + modelMatrix[4] * y + modelMatrix[8] * z+ modelMatrix[12];
        ty = modelMatrix[1] * x + modelMatrix[5] * y + modelMatrix[9] * z+ modelMatrix[13];
        tz = modelMatrix[2] * x + modelMatrix[6] * y + modelMatrix[10] * z+ modelMatrix[14];
    }

    if (vertexCounter > (vertexBuffer[currentBuffer]->elementCount * 4 - 4))
    {
        if ((draws[drawCounter - 1]->mode == LINES)
            && (draws[drawCounter - 1]->vertexCount % 2 == 0))
        {
            CheckRenderBatchLimit(2 + 1);
        }
        else if ((draws[drawCounter - 1]->mode == TRIANGLES)
                 && (draws[drawCounter - 1]->vertexCount % 3 == 0))
        {
            CheckRenderBatchLimit(3 + 1);
        }
        else if ((draws[drawCounter - 1]->mode == QUAD)
                 && (draws[drawCounter - 1]->vertexCount % 4 == 0))
        {
            CheckRenderBatchLimit(4 + 1);
        }
    }

    vertexBuffer[currentBuffer]->vertices[3 * vertexCounter] = tx;
    vertexBuffer[currentBuffer]->vertices[3 * vertexCounter + 1] = ty;
    vertexBuffer[currentBuffer]->vertices[3 * vertexCounter + 2] = tz;


    vertexBuffer[currentBuffer]->texcoords[2 * vertexCounter] = texcoordx;
    vertexBuffer[currentBuffer]->texcoords[2 * vertexCounter + 1] = texcoordy;


    vertexBuffer[currentBuffer]->colors[4 * vertexCounter] = colorr;
    vertexBuffer[currentBuffer]->colors[4 * vertexCounter + 1] = colorg;
    vertexBuffer[currentBuffer]->colors[4 * vertexCounter + 2] = colorb;
    vertexBuffer[currentBuffer]->colors[4 * vertexCounter + 3] = colora;

    vertexCounter++;
    draws[drawCounter - 1]->vertexCount++;
}


void RenderBatch::Vertex2f(float x, float y) { Vertex3f(x, y, currentDepth); }


void RenderBatch::TexCoord2f(float x, float y)
{
    texcoordx = x;
    texcoordy = y;
}

void RenderBatch::SetColor(const Color &color)
{
    colorr = color.r;
    colorg = color.g;
    colorb = color.b;
    colora = color.a;
}


unsigned char floatToUnsignedChar(float value)
{
    float normalizedValue = (value < 0.0f) ? 0.0f
        : (value > 1.0f)                   ? 1.0f
                                           : value;
    float scaledValue = normalizedValue * 255.0f;
    scaledValue = (scaledValue < 0) ? 0
        : (scaledValue > 255)       ? 255
                                    : scaledValue;
    return (unsigned char)scaledValue;
}

void RenderBatch::SetColor(float r, float g, float b)
{
    colorr = floatToUnsignedChar(r);
    colorg = floatToUnsignedChar(g);
    colorb = floatToUnsignedChar(b);
}

void RenderBatch::SetColor(u8 r, u8 g, u8 b, u8 a)
{
    colorr = r;
    colorg = g;
    colorb = b;
    colora = a;
}


void RenderBatch::SetAlpha(float a) { colora = floatToUnsignedChar(a); }

void RenderBatch::SetTexture(unsigned int id)
{
    if (id == 0)
    {
        if (vertexCounter >= vertexBuffer[currentBuffer]->elementCount * 4)
        {
            Render();
        }
    }
    else
    {
        if (draws[drawCounter - 1]->textureId != id)
        {
            if (draws[drawCounter - 1]->vertexCount > 0)
            {
                if (draws[drawCounter - 1]->mode == LINES)
                    draws[drawCounter - 1]->vertexAlignment =
                        ((draws[drawCounter - 1]->vertexCount < 4)
                             ? draws[drawCounter - 1]->vertexCount
                             : draws[drawCounter - 1]->vertexCount % 4);
                else if (draws[drawCounter - 1]->mode == TRIANGLES)
                    draws[drawCounter - 1]->vertexAlignment =
                        ((draws[drawCounter - 1]->vertexCount < 4)
                             ? 1
                             : (4 - (draws[drawCounter - 1]->vertexCount % 4)));
                else
                    draws[drawCounter - 1]->vertexAlignment = 0;

                if (!CheckRenderBatchLimit(
                        draws[drawCounter - 1]->vertexAlignment))
                {
                    vertexCounter += draws[drawCounter - 1]->vertexAlignment;
                    drawCounter++;
                }
            }

            if (drawCounter >= BATCH_DRAWCALLS) Render();

            draws[drawCounter - 1]->textureId = id;
            draws[drawCounter - 1]->vertexCount = 0;
        }
    }
}


void RenderBatch::Line2D(int startPosX, int startPosY, int endPosX, int endPosY)
{
    SetMode(LINES);
    Vertex2f((float)startPosX, (float)startPosY);
    Vertex2f((float)endPosX, (float)endPosY);
}

void RenderBatch::Line2D(const Vec2 &start, const Vec2 &end)
{
    SetMode(LINES);
    Vertex2f(start.x, start.y);
    Vertex2f(end.x, end.y);
}


void RenderBatch::Circle(int centerX, int centerY, float radius, bool fill)
{

    if (fill)
    {
        SetMode(TRIANGLES);

        float x = centerX;
        float y = centerY;
        float angle = 0.0f;
        float angleInc = 1.0f / radius;
        for (int i = 0; i < 360; i++)
        {
            Vertex2f(x, y);
            Vertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
            angle += angleInc;
            Vertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
        }
    }
    else
    {
        SetMode(LINES);
        float x = centerX;
        float y = centerY;
        float angle = 0.0f;
        float angleInc = 1.0f / radius;
        for (int i = 0; i < 360; i++)
        {
            Vertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
            angle += angleInc;
            Vertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
        }
    }
}

void RenderBatch::Rectangle(int posX, int posY, int width, int height,
                            bool fill)
{

    if (fill)
    {
        SetMode(TRIANGLES);


        float x = posX;
        float y = posY;


        Vertex2f(x, y);
        Vertex2f(x, y + height);
        Vertex2f(x + width, y);

        Vertex2f(x + width, y);
        Vertex2f(x, y + height);
        Vertex2f(x + width, y + height);
    }
    else
    {
        SetMode(LINES);


        Vertex2f(posX, posY);
        Vertex2f(posX + width, posY);

        Vertex2f(posX + width, posY);
        Vertex2f(posX + width, posY + height);

        Vertex2f(posX + width, posY + height);
        Vertex2f(posX, posY + height);

        Vertex2f(posX, posY + height);
        Vertex2f(posX, posY);
    }
}

void RenderBatch::Line3D(const Vec3 &start, const Vec3 &end)
{
    SetMode(LINES);
    Vertex3f(start.x, start.y, start.z);
    Vertex3f(end.x, end.y, end.z);
}


void RenderBatch::Box(const BoundingBox &box)
{
    SetMode(LINES);

    Line3D(box.min.x, box.min.y, box.min.z, box.max.x, box.min.y, box.min.z);
    Line3D(box.max.x, box.min.y, box.min.z, box.max.x, box.max.y, box.min.z);
    Line3D(box.max.x, box.max.y, box.min.z, box.min.x, box.max.y, box.min.z);
    Line3D(box.min.x, box.max.y, box.min.z, box.min.x, box.min.y, box.min.z);

    Line3D(box.min.x, box.min.y, box.max.z, box.max.x, box.min.y, box.max.z);
    Line3D(box.max.x, box.min.y, box.max.z, box.max.x, box.max.y, box.max.z);
    Line3D(box.max.x, box.max.y, box.max.z, box.min.x, box.max.y, box.max.z);
    Line3D(box.min.x, box.max.y, box.max.z, box.min.x, box.min.y, box.max.z);

    Line3D(box.min.x, box.min.y, box.min.z, box.min.x, box.min.y, box.max.z);
    Line3D(box.max.x, box.min.y, box.min.z, box.max.x, box.min.y, box.max.z);
    Line3D(box.max.x, box.max.y, box.min.z, box.max.x, box.max.y, box.max.z);
    Line3D(box.min.x, box.max.y, box.min.z, box.min.x, box.max.y, box.max.z);
}

void RenderBatch::Cube(const Vec3 &position, float w, float h, float d,
                       bool wire)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    if (wire)
    {
        SetMode(LINES);
        Line3D(x, y, z, x + w, y, z);
        Line3D(x + w, y, z, x + w, y + h, z);
        Line3D(x + w, y + h, z, x, y + h, z);
        Line3D(x, y + h, z, x, y, z);

        Line3D(x, y, z + d, x + w, y, z + d);
        Line3D(x + w, y, z + d, x + w, y + h, z + d);
        Line3D(x + w, y + h, z + d, x, y + h, z + d);
        Line3D(x, y + h, z + d, x, y, z + d);

        Line3D(x, y, z, x, y, z + d);
        Line3D(x + w, y, z, x + w, y, z + d);
        Line3D(x + w, y + h, z, x + w, y + h, z + d);
        Line3D(x, y + h, z, x, y + h, z + d);
    }
    else
    {
        SetMode(TRIANGLES);
        Vertex3f(x - w / 2, y - h / 2, z + d / 2); // Bottom Left
        Vertex3f(x + w / 2, y - h / 2, z + d / 2); // Bottom Right
        Vertex3f(x - w / 2, y + h / 2, z + d / 2); // Top Left

        Vertex3f(x + w / 2, y + h / 2, z + d / 2); // Top Right
        Vertex3f(x - w / 2, y + h / 2, z + d / 2); // Top Left
        Vertex3f(x + w / 2, y - h / 2, z + d / 2); // Bottom Right

        // Back face
        Vertex3f(x - w / 2, y - h / 2, z - d / 2); // Bottom Left
        Vertex3f(x - w / 2, y + h / 2, z - d / 2); // Top Left
        Vertex3f(x + w / 2, y - h / 2, z - d / 2); // Bottom Right

        Vertex3f(x + w / 2, y + h / 2, z - d / 2); // Top Right
        Vertex3f(x + w / 2, y - h / 2, z - d / 2); // Bottom Right
        Vertex3f(x - w / 2, y + h / 2, z - d / 2); // Top Left

        // Top face
        Vertex3f(x - w / 2, y + h / 2, z - d / 2); // Top Left
        Vertex3f(x - w / 2, y + h / 2, z + d / 2); // Bottom Left
        Vertex3f(x + w / 2, y + h / 2, z + d / 2); // Bottom Right

        Vertex3f(x + w / 2, y + h / 2, z - d / 2); // Top Right
        Vertex3f(x - w / 2, y + h / 2, z - d / 2); // Top Left
        Vertex3f(x + w / 2, y + h / 2, z + d / 2); // Bottom Right

        // Bottom face
        Vertex3f(x - w / 2, y - h / 2, z - d / 2); // Top Left
        Vertex3f(x + w / 2, y - h / 2, z + d / 2); // Bottom Right
        Vertex3f(x - w / 2, y - h / 2, z + d / 2); // Bottom Left

        Vertex3f(x + w / 2, y - h / 2, z - d / 2); // Top Right
        Vertex3f(x + w / 2, y - h / 2, z + d / 2); // Bottom Right
        Vertex3f(x - w / 2, y - h / 2, z - d / 2); // Top Left

        // Right face
        Vertex3f(x + w / 2, y - h / 2, z - d / 2); // Bottom Right
        Vertex3f(x + w / 2, y + h / 2, z - d / 2); // Top Right
        Vertex3f(x + w / 2, y + h / 2, z + d / 2); // Top Left

        Vertex3f(x + w / 2, y - h / 2, z + d / 2); // Bottom Left
        Vertex3f(x + w / 2, y - h / 2, z - d / 2); // Bottom Right
        Vertex3f(x + w / 2, y + h / 2, z + d / 2); // Top Left

        // Left face
        Vertex3f(x - w / 2, y - h / 2, z - d / 2); // Bottom Right
        Vertex3f(x - w / 2, y + h / 2, z + d / 2); // Top Left
        Vertex3f(x - w / 2, y + h / 2, z - d / 2); // Top Right

        Vertex3f(x - w / 2, y - h / 2, z + d / 2); // Bottom Left
        Vertex3f(x - w / 2, y + h / 2, z + d / 2); // Top Left
        Vertex3f(x - w / 2, y - h / 2, z - d / 2); // Bottom Right
    }
}


void RenderBatch::Sphere(const Vec3 &position, float radius, int rings,
                         int slices, bool wire)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    //  float const R = 1./(float)(rings-1);
    //   float const S = 1./(float)(slices-1);

    if (wire)
    {
        SetMode(LINES);


        for (int i = 0; i < rings; ++i)
        {
            float theta = i * M_PI / rings;
            for (int j = 0; j < slices; ++j)
            {
                float phi1 = j * 2 * M_PI / slices;
                float phi2 = (j + 1) * 2 * M_PI / slices;

                Vec3 v1 = Vec3(x + radius * sin(theta) * cos(phi1),
                               y + radius * sin(theta) * sin(phi1),
                               z + radius * cos(theta));

                Vec3 v2 = Vec3(x + radius * sin(theta) * cos(phi2),
                               y + radius * sin(theta) * sin(phi2),
                               z + radius * cos(theta));

                Line3D(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
            }
        }

        // Desenhar linhas verticais
        for (int j = 0; j < slices; ++j)
        {
            float phi = j * 2 * M_PI / slices;
            for (int i = 0; i < rings; ++i)
            {
                float theta1 = i * M_PI / rings;
                float theta2 = (i + 1) * M_PI / rings;

                Vec3 v1 = Vec3(x + radius * sin(theta1) * cos(phi),
                               y + radius * sin(theta1) * sin(phi),
                               z + radius * cos(theta1));

                Vec3 v2 = Vec3(x + radius * sin(theta2) * cos(phi),
                               y + radius * sin(theta2) * sin(phi),
                               z + radius * cos(theta2));

                Line3D(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
            }
        }
    }
    else
    {

        SetMode(TRIANGLES);
        for (int i = 0; i < rings; ++i)
        {
            for (int j = 0; j < slices; ++j)
            {
                // Calcular os v�rtices da esfera
                float theta1 = i * M_PI / rings;
                float theta2 = (i + 1) * M_PI / rings;
                float phi1 = j * 2 * M_PI / slices;
                float phi2 = (j + 1) * 2 * M_PI / slices;

                Vec3 v1 = Vec3(x + radius * sin(theta1) * cos(phi1),
                               y + radius * sin(theta1) * sin(phi1),
                               z + radius * cos(theta1));

                Vec3 v2 = Vec3(x + radius * sin(theta1) * cos(phi2),
                               y + radius * sin(theta1) * sin(phi2),
                               z + radius * cos(theta1));

                Vec3 v3 = Vec3(x + radius * sin(theta2) * cos(phi1),
                               y + radius * sin(theta2) * sin(phi1),
                               z + radius * cos(theta2));

                Vec3 v4 = Vec3(x + radius * sin(theta2) * cos(phi2),
                               y + radius * sin(theta2) * sin(phi2),
                               z + radius * cos(theta2));

                // Desenhar os tri�ngulos da esfera
                Vertex3f(v1.x, v1.y, v1.z);
                Vertex3f(v2.x, v2.y, v2.z);
                Vertex3f(v3.x, v3.y, v3.z);

                Vertex3f(v2.x, v2.y, v2.z);
                Vertex3f(v4.x, v4.y, v4.z);
                Vertex3f(v3.x, v3.y, v3.z);
            }
        }
    }
}

void RenderBatch::Cone(const Vec3 &position, float radius, float height,
                       int segments, bool wire)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    if (wire)
    {
        SetMode(LINES);

        // Desenhar linhas que formam a base do cone
        for (int i = 0; i < segments; ++i)
        {
            float theta1 = i * 2 * M_PI / segments;
            float theta2 = (i + 1) * 2 * M_PI / segments;

            Vec3 v1 =
                Vec3(x + radius * cos(theta1), y, z + radius * sin(theta1));

            Vec3 v2 =
                Vec3(x + radius * cos(theta2), y, z + radius * sin(theta2));

            Line3D(v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
        }

        // Desenhar linhas que conectam a base ao v�rtice do cone
        Vec3 vertex = Vec3(x, y + height, z);
        for (int i = 0; i < segments; ++i)
        {
            float theta = i * 2 * M_PI / segments;

            Vec3 base =
                Vec3(x + radius * cos(theta), y, z + radius * sin(theta));

            Line3D(base.x, base.y, base.z, vertex.x, vertex.y, vertex.z);
        }
    }
    else
    {
        SetMode(TRIANGLES);
        Vec3 vertex = Vec3(x, y + height, z);

        // Desenhar tri�ngulos para formar as faces do cone
        for (int i = 0; i < segments; ++i)
        {
            float theta1 = i * 2 * M_PI / segments;
            float theta2 = (i + 1) * 2 * M_PI / segments;

            Vec3 base1 =
                Vec3(x + radius * cos(theta1), y, z + radius * sin(theta1));

            Vec3 base2 =
                Vec3(x + radius * cos(theta2), y, z + radius * sin(theta2));

            // Tri�ngulo formado pela base do cone e o v�rtice
            Vertex3f(base1.x, base1.y, base1.z);
            Vertex3f(base2.x, base2.y, base2.z);
            Vertex3f(vertex.x, vertex.y, vertex.z);
        }
    }
}


void RenderBatch::Cylinder(const Vec3 &position, float radius, float height,
                           int segments, bool wire)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    if (wire)
    {
        for (int i = 0; i < segments; ++i)
        {
            float theta1 = i * 2 * M_PI / segments;
            float theta2 = (i + 1) * 2 * M_PI / segments;

            Vec3 base1 =
                Vec3(x + radius * cos(theta1), y, z + radius * sin(theta1));

            Vec3 base2 =
                Vec3(x + radius * cos(theta2), y, z + radius * sin(theta2));

            Line3D(base1.x, base1.y, base1.z, base2.x, base2.y, base2.z);
        }

        // Desenhar linhas que formam a base superior do cilindro
        for (int i = 0; i < segments; ++i)
        {
            float theta1 = i * 2 * M_PI / segments;
            float theta2 = (i + 1) * 2 * M_PI / segments;

            Vec3 top1 = Vec3(x + radius * cos(theta1), y + height,
                             z + radius * sin(theta1));

            Vec3 top2 = Vec3(x + radius * cos(theta2), y + height,
                             z + radius * sin(theta2));

            Line3D(top1.x, top1.y, top1.z, top2.x, top2.y, top2.z);
        }

        // Desenhar linhas que conectam a base inferior � superior
        for (int i = 0; i < segments; ++i)
        {
            float theta = i * 2 * M_PI / segments;

            Vec3 base =
                Vec3(x + radius * cos(theta), y, z + radius * sin(theta));

            Vec3 top = Vec3(x + radius * cos(theta), y + height,
                            z + radius * sin(theta));

            Line3D(base.x, base.y, base.z, top.x, top.y, top.z);
        }
    }
    else
    {
        SetMode(TRIANGLES);

        // Desenhar a base inferior do cilindro
        for (int i = 0; i < segments; ++i)
        {
            float theta1 = i * 2 * M_PI / segments;
            float theta2 = (i + 1) * 2 * M_PI / segments;

            Vec3 base1 =
                Vec3(x + radius * cos(theta1), y, z + radius * sin(theta1));

            Vec3 base2 =
                Vec3(x + radius * cos(theta2), y, z + radius * sin(theta2));

            Vertex3f(x, y, z); // Centro da base inferior
            Vertex3f(base1.x, base1.y, base1.z);
            Vertex3f(base2.x, base2.y, base2.z);
        }

        // Desenhar a base superior do cilindro
        for (int i = 0; i < segments; ++i)
        {
            float theta1 = i * 2 * M_PI / segments;
            float theta2 = (i + 1) * 2 * M_PI / segments;

            Vec3 top1 = Vec3(x + radius * cos(theta1), y + height,
                             z + radius * sin(theta1));

            Vec3 top2 = Vec3(x + radius * cos(theta2), y + height,
                             z + radius * sin(theta2));

            Vertex3f(x, y + height, z); // Centro da base superior
            Vertex3f(top2.x, top2.y, top2.z);
            Vertex3f(top1.x, top1.y, top1.z);
        }

        // Desenhar a superf�cie lateral do cilindro
        for (int i = 0; i < segments; ++i)
        {
            float theta1 = i * 2 * M_PI / segments;
            float theta2 = (i + 1) * 2 * M_PI / segments;

            Vec3 base1 =
                Vec3(x + radius * cos(theta1), y, z + radius * sin(theta1));

            Vec3 base2 =
                Vec3(x + radius * cos(theta2), y, z + radius * sin(theta2));

            Vec3 top1 = Vec3(x + radius * cos(theta1), y + height,
                             z + radius * sin(theta1));

            Vec3 top2 = Vec3(x + radius * cos(theta2), y + height,
                             z + radius * sin(theta2));

            // Tri�ngulo lateral inferior
            Vertex3f(base1.x, base1.y, base1.z);
            Vertex3f(base2.x, base2.y, base2.z);
            Vertex3f(top2.x, top2.y, top2.z);

            // Tri�ngulo lateral superior
            Vertex3f(base1.x, base1.y, base1.z);
            Vertex3f(top2.x, top2.y, top2.z);
            Vertex3f(top1.x, top1.y, top1.z);
        }
    }
}

void RenderBatch::Triangle(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3)
{
    SetMode(QUAD);

    Vertex3f(p1.x, p1.y, p1.z);
    Vertex3f(p2.x, p2.y, p2.z);
    Vertex3f(p3.x, p3.y, p3.z);
    Vertex3f(p1.x, p1.y, p1.z);
}

void RenderBatch::TriangleLines(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3)
{

    Line3D(p1.x, p1.y, p1.z, p2.x, p2.y, p2.z);
    Line3D(p2.x, p2.y, p2.z, p3.x, p3.y, p3.z);
    Line3D(p3.x, p3.y, p3.z, p1.x, p1.y, p1.z);
}
void RenderBatch::Triangle(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3,
                           const Vec2 &t1, const Vec2 &t2, const Vec2 &t3)
{
    SetMode(QUAD);

    TexCoord2f(t1.x, t1.y);
    Vertex3f(p1.x, p1.y, p1.z);

    TexCoord2f(t2.x, t2.y);
    Vertex3f(p2.x, p2.y, p2.z);

    TexCoord2f(t3.x, t3.y);
    Vertex3f(p3.x, p3.y, p3.z);

    TexCoord2f(t1.x, t1.y);
    Vertex3f(p1.x, p1.y, p1.z);
}

void RenderBatch::Grid(int slices, float spacing, bool axes)
{

    int halfSlices = slices / 2;

    for (int i = -halfSlices; i <= halfSlices; i++)
    {
        if (i == 0)
        {
            SetColor(0.5f, 0.5f, 0.5f);
        }
        else
        {
            SetColor(0.75f, 0.75f, 0.75f);
        }
        Line3D(i * spacing, 0, -halfSlices * spacing, i * spacing, 0,
               halfSlices * spacing);
        Line3D(-halfSlices * spacing, 0, i * spacing, halfSlices * spacing, 0,
               i * spacing);
    }
    if (axes)
    {
        SetColor(255, 0, 0);
        Line3D(0.0f, 0.5f, 0.0f, 1, 0.5f, 0);
        SetColor(0, 255, 0);
        Line3D(0.0f, 0.5f, 0.0f, 0, 1.5f, 0);
        SetColor(0, 0, 255);
        Line3D(0.0f, 0.5f, 0.0f, 0, 0.5f, 1);
    }
}

// void RenderBatch::Box(const BoundingBox &box, const Mat4 &transform)
// {
//     Vec3 edges[8];
//     box.GetEdges(edges);

//     for (int i = 0; i < 8; i++)
//     {
//         glm::vec3 vec =
//         transform*glm::vec4(glm::vec3(edges[i].x,edges[i].y,edges[i].z), 1.0f);
//         edges[i] = Vec3(vec.x, vec.y, vec.z);
//     }

//     SetMode(LINES);
//     Line3D(edges[5], edges[1]);
//     Line3D(edges[1], edges[3]);
//     Line3D(edges[3], edges[7]);
//     Line3D(edges[7], edges[5]);
//     Line3D(edges[0], edges[2]);
//     Line3D(edges[2], edges[6]);
//     Line3D(edges[6], edges[4]);
//     Line3D(edges[4], edges[0]);
//     Line3D(edges[1], edges[0]);
//     Line3D(edges[3], edges[2]);
//     Line3D(edges[7], edges[6]);
//     Line3D(edges[5], edges[4]);
// }

void RenderBatch::Quad(const Vec2 *coords, const Vec2 *texcoords)
{
    SetMode(QUAD);

    TexCoord2f(texcoords[0].x, texcoords[0].y);
    Vertex2f(coords[0].x, coords[0].y);

    TexCoord2f(texcoords[1].x, texcoords[1].y);
    Vertex2f(coords[1].x, coords[1].y);

    TexCoord2f(texcoords[2].x, texcoords[2].y);
    Vertex2f(coords[2].x, coords[2].y);

    TexCoord2f(texcoords[3].x, texcoords[3].y);
    Vertex2f(coords[3].x, coords[3].y);
}

void RenderBatch::SetTexture(Texture *texture)
{
    if (texture != nullptr)
    {

        SetTexture(texture->GetHandle());
    }
    else
    {
        SetTexture(defaultTextureId);
    }
}

void RenderBatch::Quad(Texture *texture, const Vec2 *coords,
                       const Vec2 *texcoords)
{

    if (texture != nullptr)
    {

        SetTexture(texture->GetHandle());
    }
    else
    {
        SetTexture(defaultTextureId);
    }
    Quad(coords, texcoords);
}

void RenderBatch::Quad(u32 texture, float x, float y, float width, float height)
{

    float left = 0;
    float right = 1;
    float top = 0;
    float bottom = 1;


    SetTexture(texture);


    float x1 = x;
    float y1 = y;
    float x2 = x;
    float y2 = y + height;
    float x3 = x + width;
    float y3 = y + height;
    float x4 = x + width;
    float y4 = y;

    Vec2 coords[4];
    Vec2 texcoords[4];


    coords[0].x = x1;
    coords[0].y = y1;
    coords[1].x = x2;
    coords[1].y = y2;
    coords[2].x = x3;
    coords[2].y = y3;
    coords[3].x = x4;
    coords[3].y = y4;

    texcoords[0].x = left;
    texcoords[0].y = top;
    texcoords[1].x = left;
    texcoords[1].y = bottom;
    texcoords[2].x = right;
    texcoords[2].y = bottom;
    texcoords[3].x = right;
    texcoords[3].y = top;


    Quad(coords, texcoords);
}

void RenderBatch::Quad(Texture *texture, float x, float y, float width,
                       float height)
{


    float left = 0;
    float right = 1;
    float top = 0;
    float bottom = 1;


    if (texture != nullptr)
    {

        SetTexture(texture->GetHandle());
    }
    else
    {
        SetTexture(defaultTextureId);
    }


    float x1 = x;
    float y1 = y;
    float x2 = x;
    float y2 = y + height;
    float x3 = x + width;
    float y3 = y + height;
    float x4 = x + width;
    float y4 = y;

    Vec2 coords[4];
    Vec2 texcoords[4];


    coords[0].x = x1;
    coords[0].y = y1;
    coords[1].x = x2;
    coords[1].y = y2;
    coords[2].x = x3;
    coords[2].y = y3;
    coords[3].x = x4;
    coords[3].y = y4;

    texcoords[0].x = left;
    texcoords[0].y = top;
    texcoords[1].x = left;
    texcoords[1].y = bottom;
    texcoords[2].x = right;
    texcoords[2].y = bottom;
    texcoords[3].x = right;
    texcoords[3].y = top;


    Quad(coords, texcoords);
}


void RenderBatch::Quad(Texture *texture, const FloatRect &src, float x,
                       float y, float width, float height)
{


    float left = 0;
    float right = 1;
    float top = 0;
    float bottom = 1;


    int widthTex = 1;
    int heightTex = 1;

    if (texture != nullptr)
    {
        widthTex = texture->GetWidth();
        heightTex = texture->GetHeight();
        SetTexture(texture->GetHandle());
    }


    left = (2.0f * src.x + 1.0f) / (2.0f * widthTex);
    right = left + (src.width * 2.0f - 2.0f) / (2.0f * widthTex);
    top = (2.0f * src.y + 1.0f) / (2 * heightTex);
    bottom = top + (src.height * 2.0f - 2.0f) / (2.0f * heightTex);


    float x1 = x;
    float y1 = y;
    float x2 = x;
    float y2 = y + height;
    float x3 = x + width;
    float y3 = y + height;
    float x4 = x + width;
    float y4 = y;

    Vec2 coords[4];
    Vec2 texcoords[4];


    coords[0].x = x1;
    coords[0].y = y1;
    coords[1].x = x2;
    coords[1].y = y2;
    coords[2].x = x3;
    coords[2].y = y3;
    coords[3].x = x4;
    coords[3].y = y4;

    texcoords[0].x = left;
    texcoords[0].y = top;
    texcoords[1].x = left;
    texcoords[1].y = bottom;
    texcoords[2].x = right;
    texcoords[2].y = bottom;
    texcoords[3].x = right;
    texcoords[3].y = top;


    Quad(coords, texcoords);
}

 