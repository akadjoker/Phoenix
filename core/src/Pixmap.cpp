#include "Pixmap.hpp"
#include "stb_image.h"
#include "stb_image_write.h"

Pixmap::Pixmap()
{

    width = 0;
    height = 0;
    components = 0;
    pixels = nullptr;
}

Pixmap::~Pixmap()
{
    if (pixels)
    {
        free(pixels);
    }
}

Pixmap::Pixmap(const Pixmap &image, const IntRect &crop)
{
    width = crop.width;
    height = crop.height;
    components = image.components;
    pixels = (unsigned char *)malloc(width * height * components);
    for (int y = (int)crop.y, offsetSize = 0; y < (int)(crop.y + crop.height); y++)
    {
        memcpy(pixels + offsetSize, ((unsigned char *)image.pixels) + (y * image.width + (int)crop.x) * components, (int)crop.width * components);
        offsetSize += ((int)crop.width * components);
    }
}

Pixmap::Pixmap(int w, int h, int components)
{

    width = w;
    height = h;
    this->components = components;
    pixels = (unsigned char *)malloc(w * h * components);
}

Pixmap::Pixmap(int w, int h, int components, unsigned char *data)
{

    pixels = (unsigned char *)malloc(w * h * components);
    width = w;
    height = h;
    this->components = components;
    memcpy(pixels, data, w * h * components);
}

void Pixmap::Clear()
{
    if (pixels)
    {
        for (int i = 0; i < width * height * components; i++)
        {
            pixels[i] = 0;
        }
    }
}

void Pixmap::SetPixel(u32 x, u32 y, u8 r, u8 g, u8 b, u8 a)
{

    if (x >= (u32)width || y >= (u32)height)
        return;

    if (components == 1)
    {
        Vec3 coln((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f);
        unsigned char gray = (unsigned char)((coln.x * 0.299f + coln.y * 0.587f + coln.z * 0.114f) * 255.0f);
        ((u8 *)pixels)[y * width + x] = gray;
    }
    else if (components == 2)
    {
        Vec3 coln((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f);
        u8 gray = (u8)((coln.x * 0.299f + coln.y * 0.587f + coln.z * 0.114f) * 255.0f);

        ((u8 *)pixels)[(y * width + x) * 2] = gray;
        ((u8 *)pixels)[(y * width + x) * 2 + 1] = a;
    }
    else if (components == 3)
    {

        ((u8 *)pixels)[(y * width + x) * 3] = r;
        ((u8 *)pixels)[(y * width + x) * 3 + 1] = g;
        ((u8 *)pixels)[(y * width + x) * 3 + 2] = b;
    }
    else if (components == 4)
    {
        ((u8 *)pixels)[(y * width + x) * 4] = r;
        ((u8 *)pixels)[(y * width + x) * 4 + 1] = g;
        ((u8 *)pixels)[(y * width + x) * 4 + 2] = b;
        ((u8 *)pixels)[(y * width + x) * 4 + 3] = a;
    }
}

void Pixmap::SetPixel(u32 x, u32 y, u32 rgba)
{

    if (x >= (u32)width || y >= (u32)height)
        return;

    u8 r = rgba;
    u8 g = rgba >> 8;
    u8 b = rgba >> 16;
    u8 a = rgba >> 24;
    SetPixel(x, y, r, g, b, a);
}

u32 Pixmap::GetPixel(u32 x, u32 y) const
{

    if (x >= (u32)width || y >= (u32)height)
        return 0;

    if (components == 1)
    {
        return pixels[y * width + x];
    }
    else if (components == 2)
    {
        return pixels[(y * width + x) * 2] | (pixels[(y * width + x) * 2 + 1] << 8);
    }
    else if (components == 3)
    {
        return pixels[(y * width + x) * 3] | (pixels[(y * width + x) * 3 + 1] << 8) | (pixels[(y * width + x) * 3 + 2] << 16);
    }
    else if (components == 4)
    {
        return pixels[(y * width + x) * 4] | (pixels[(y * width + x) * 4 + 1] << 8) | (pixels[(y * width + x) * 4 + 2] << 16) | (pixels[(y * width + x) * 4 + 3] << 24);
    }

    return 0;
}

Color Pixmap::GetPixelColor(u32 x, u32 y) const
{

    Color color = Color::BLACK;

    if ((x < (u32)width) && (y < (u32)height))
    {

        if (components == 1)
        {
            color.r = (u8)((u8 *)pixels)[y * width + x];
            color.g = (u8)((u8 *)pixels)[y * width + x];
            color.b = (u8)((u8 *)pixels)[y * width + x];
            color.a = 255;
        }
        else if (components == 2)
        {
            color.r = (u8)((u8 *)pixels)[(y * width + x) * 2];
            color.g = (u8)((u8 *)pixels)[(y * width + x) * 2];
            color.b = (u8)((u8 *)pixels)[(y * width + x) * 2];
            color.a = (u8)((u8 *)pixels)[(y * width + x) * 2 + 1];
        }
        else if (components == 3)
        {
            color.r = (u8)((u8 *)pixels)[(y * width + x) * 3];
            color.g = (u8)((u8 *)pixels)[(y * width + x) * 3 + 1];
            color.b = (u8)((u8 *)pixels)[(y * width + x) * 3 + 2];
            color.a = 255;
        }
        else if (components == 4)
        {
            color.r = (u8)((u8 *)pixels)[(y * width + x) * 4];
            color.g = (u8)((u8 *)pixels)[(y * width + x) * 4 + 1];
            color.b = (u8)((u8 *)pixels)[(y * width + x) * 4 + 2];
            color.a = (u8)((u8 *)pixels)[(y * width + x) * 4 + 3];
        }
    }
    return color;
}

void Pixmap::Fill(u8 r, u8 g, u8 b, u8 a)
{
    for (u32 y = 0; y < (u32)height; y++)
    {
        for (u32 x = 0; x < (u32)width; x++)
        {
            SetPixel(x, y, r, g, b, a);
        }
    }
}

void Pixmap::Fill(u32 rgba)
{
    for (u32 y = 0; y < (u32)height; y++)
    {
        for (u32 x = 0; x < (u32)width; x++)
        {
            SetPixel(x, y, rgba);
        }
    }
}

bool Pixmap::Load(const char *file_name)
{

    pixels = stbi_load(file_name, &width, &height, &components, 0);

    if (pixels == nullptr)
    {
        LogError("Failed to load image: %s", file_name);
        return false;
    }

    // Log(0, "PIXMAP: Load image: %s (%d,%d) bpp:%d", file_name, width, height, components);

    return true;
}

bool Pixmap::LoadFromMemory(const unsigned char *buffer, unsigned int bytesRead)
{

    pixels = stbi_load_from_memory(buffer, bytesRead, &width, &height, &components, 0);
    if (pixels == nullptr)
    {
        LogError("Failed to load image from memory");
        return false;
    }
    return true;
}

bool Pixmap::Save(const char *file_name)
{
    if (pixels == nullptr)
    {
        LogError("Failed to save image: %s", file_name);
        return false;
    }

    if (Utils::IsFileExtension(file_name, ".bmp"))
        return stbi_write_bmp(file_name, width, height, components, pixels);
    else if (Utils::IsFileExtension(file_name, ".tga"))
        return stbi_write_tga(file_name, width, height, components, pixels);
    else if (Utils::IsFileExtension(file_name, ".png"))
        return stbi_write_png(file_name, width, height, components, pixels, 0);
    else
    {
        LogError("Failed to save image: %s ", file_name);
        return false;
    }
    return false;
}

void Pixmap::FlipVertical()
{
    if (pixels == nullptr)
    {
        LogError("Failed to flip image");
        return;
    }
    int rowSize = width * components;
    unsigned char *row = (unsigned char *)malloc(rowSize);
    for (int y = 0; y < height / 2; y++)
    {
        unsigned char *src = ((unsigned char *)pixels) + y * rowSize;
        unsigned char *dst = ((unsigned char *)pixels) + (height - y - 1) * rowSize;
        memcpy(row, src, rowSize);
        memcpy(src, dst, rowSize);
        memcpy(dst, row, rowSize);
    }
    free(row);
}

void Pixmap::FlipHorizontal()
{
    if (pixels == nullptr)
    {
        LogError("Failed to flip image");
        return;
    }
    int rowSize = width * components;
    unsigned char *row = (unsigned char *)malloc(rowSize);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width / 2; x++)
        {
            unsigned char *src = ((unsigned char *)pixels) + y * rowSize + x * components;
            unsigned char *dst = ((unsigned char *)pixels) + y * rowSize + (width - x - 1) * components;
            memcpy(row, src, components);
            memcpy(src, dst, components);
            memcpy(dst, row, components);
        }
    }
    free(row);
}

void Pixmap::DrawLine(int x1, int y1, int x2, int y2, const Color &color)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (true)
    {
        SetPixel(x1, y1, color.r, color.g, color.b, color.a);
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void Pixmap::Tint(u8 r, u8 g, u8 b)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Color c = GetPixelColor(x, y);
            c.r = (c.r * r) / 255;
            c.g = (c.g * g) / 255;
            c.b = (c.b * b) / 255;
            SetPixel(x, y, c.r, c.g, c.b, c.a);
        }
    }
}

Pixmap* Pixmap::ConvertToRGBA() const
{
    if (components == 4) return new Pixmap(this->width, this->height, this->components, this->pixels);
    
    Pixmap* result = new Pixmap(width, height, 4);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Color c = GetPixelColor(x, y);
            result->SetPixel(x, y, c.r, c.g, c.b, c.a);
        }
    }
    return result;
}

 
Pixmap* Pixmap::Resize(int newWidth, int newHeight ) const
{
    Pixmap* result = new Pixmap(newWidth, newHeight, components);
    
    for (int y = 0; y < newHeight; y++)
    {
        for (int x = 0; x < newWidth; x++)
        {
            int srcX = (x * width) / newWidth;
            int srcY = (y * height) / newHeight;
            
            u32 pixel = GetPixel(srcX, srcY);
            result->SetPixel(x, y, pixel);
        }
    }
    return result;
}

void Pixmap::CopyRegion(const Pixmap &source, const IntRect &srcRect, int dstX, int dstY)
{
    // Validação
    if (!source.pixels || !pixels) return;
    if (source.components != components) return;
    
    int srcX = srcRect.x;
    int srcY = srcRect.y;
    int srcW = srcRect.width;
    int srcH = srcRect.height;
    
    // Clamp source rect
    if (srcX < 0) { srcW += srcX; dstX -= srcX; srcX = 0; }
    if (srcY < 0) { srcH += srcY; dstY -= srcY; srcY = 0; }
    if (srcX + srcW > source.width) srcW = source.width - srcX;
    if (srcY + srcH > source.height) srcH = source.height - srcY;
    
    // Copy pixel by pixel
    for (int y = 0; y < srcH; y++)
    {
        for (int x = 0; x < srcW; x++)
        {
            int sx = srcX + x;
            int sy = srcY + y;
            int dx = dstX + x;
            int dy = dstY + y;
            
            // Check destination bounds
            if (dx >= 0 && dx < width && dy >= 0 && dy < height)
            {
                Color c = source.GetPixelColor(sx, sy);
                SetPixel(dx, dy, c.r, c.g, c.b, c.a);
            }
        }
    }
}

void Pixmap::ReplaceColor(const Color &from, const Color &to, float threshold)
{
    if (!pixels) return;
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Color c = GetPixelColor(x, y);
            
            // Calcular diferença de cor
            float dr = abs(c.r - from.r) / 255.0f;
            float dg = abs(c.g - from.g) / 255.0f;
            float db = abs(c.b - from.b) / 255.0f;
            float da = abs(c.a - from.a) / 255.0f;
            
            float diff = (dr + dg + db + da) / 4.0f;
            
            if (diff <= threshold)
            {
                SetPixel(x, y, to.r, to.g, to.b, to.a);
            }
        }
    }
}

void Pixmap::SetColorKey(const Color &key, float threshold)
{
    if (!pixels) return;
    
    // Se não tem alpha, converter para RGBA
    if (components != 4 && components != 2)
    {
        LogWarning("SetColorKey requires alpha channel");
        return;
    }
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Color c = GetPixelColor(x, y);
            
            // Calcular distância da cor key
            float dr = abs(c.r - key.r) / 255.0f;
            float dg = abs(c.g - key.g) / 255.0f;
            float db = abs(c.b - key.b) / 255.0f;
            
            float diff = (dr + dg + db) / 3.0f;
            
            if (diff <= threshold)
            {
                // Tornar transparente
                SetPixel(x, y, c.r, c.g, c.b, 0);
            }
        }
    }
}

void Pixmap::DrawRect(int x, int y, int w, int h, const Color &color, bool fill)
{
    if (!pixels) return;
    
    if (fill)
    {
        // Preencher retângulo
        for (int dy = 0; dy < h; dy++)
        {
            for (int dx = 0; dx < w; dx++)
            {
                int px = x + dx;
                int py = y + dy;
                
                if (px >= 0 && px < width && py >= 0 && py < height)
                {
                    SetPixel(px, py, color.r, color.g, color.b, color.a);
                }
            }
        }
    }
    else
    {
        // Desenhar apenas bordas
        // Linha superior
        for (int dx = 0; dx < w; dx++)
        {
            if (x + dx >= 0 && x + dx < width && y >= 0 && y < height)
                SetPixel(x + dx, y, color.r, color.g, color.b, color.a);
        }
        
        // Linha inferior
        for (int dx = 0; dx < w; dx++)
        {
            if (x + dx >= 0 && x + dx < width && y + h - 1 >= 0 && y + h - 1 < height)
                SetPixel(x + dx, y + h - 1, color.r, color.g, color.b, color.a);
        }
        
        // Linha esquerda
        for (int dy = 0; dy < h; dy++)
        {
            if (x >= 0 && x < width && y + dy >= 0 && y + dy < height)
                SetPixel(x, y + dy, color.r, color.g, color.b, color.a);
        }
        
        // Linha direita
        for (int dy = 0; dy < h; dy++)
        {
            if (x + w - 1 >= 0 && x + w - 1 < width && y + dy >= 0 && y + dy < height)
                SetPixel(x + w - 1, y + dy, color.r, color.g, color.b, color.a);
        }
    }
}

void Pixmap::DrawCircle(int cx, int cy, int radius, const Color &color, bool fill)
{
    if (!pixels) return;
    
    if (fill)
    {
        // Círculo preenchido
        int radiusSq = radius * radius;
        
        for (int y = -radius; y <= radius; y++)
        {
            for (int x = -radius; x <= radius; x++)
            {
                if (x * x + y * y <= radiusSq)
                {
                    int px = cx + x;
                    int py = cy + y;
                    
                    if (px >= 0 && px < width && py >= 0 && py < height)
                    {
                        SetPixel(px, py, color.r, color.g, color.b, color.a);
                    }
                }
            }
        }
    }
    else
    {
        // Círculo outline (Bresenham)
        int x = 0;
        int y = radius;
        int d = 3 - 2 * radius;
        
        auto drawCirclePoints = [&](int px, int py)
        {
            if (px >= 0 && px < width && py >= 0 && py < height)
                SetPixel(px, py, color.r, color.g, color.b, color.a);
        };
        
        while (x <= y)
        {
            // Desenhar 8 pontos simétricos
            drawCirclePoints(cx + x, cy + y);
            drawCirclePoints(cx - x, cy + y);
            drawCirclePoints(cx + x, cy - y);
            drawCirclePoints(cx - x, cy - y);
            drawCirclePoints(cx + y, cy + x);
            drawCirclePoints(cx - y, cy + x);
            drawCirclePoints(cx + y, cy - x);
            drawCirclePoints(cx - y, cy - x);
            
            if (d < 0)
            {
                d = d + 4 * x + 6;
            }
            else
            {
                d = d + 4 * (x - y) + 10;
                y--;
            }
            x++;
        }
    }
}

// Versão simples - desenha pixmap completo
void Pixmap::DrawPixmap(const Pixmap &source, int x, int y)
{
    if (!pixels || !source.pixels) return;
    
    for (int sy = 0; sy < source.height; sy++)
    {
        for (int sx = 0; sx < source.width; sx++)
        {
            int dx = x + sx;
            int dy = y + sy;
            
            if (dx >= 0 && dx < width && dy >= 0 && dy < height)
            {
                Color c = source.GetPixelColor(sx, sy);
                
                // Se source tem alpha, fazer blend
                if (source.components == 4 || source.components == 2)
                {
                    if (c.a == 255)
                    {
                        SetPixel(dx, dy, c.r, c.g, c.b, c.a);
                    }
                    else if (c.a > 0)
                    {
                        // Alpha blend
                        Color dst = GetPixelColor(dx, dy);
                        float alpha = c.a / 255.0f;
                        
                        u8 r = (u8)(c.r * alpha + dst.r * (1.0f - alpha));
                        u8 g = (u8)(c.g * alpha + dst.g * (1.0f - alpha));
                        u8 b = (u8)(c.b * alpha + dst.b * (1.0f - alpha));
                        
                        SetPixel(dx, dy, r, g, b, dst.a);
                    }
                }
                else
                {
                    SetPixel(dx, dy, c.r, c.g, c.b, 255);
                }
            }
        }
    }
}

// Versão com source rect
void Pixmap::DrawPixmap(const Pixmap &source, int x, int y, const IntRect &srcRect)
{
    if (!pixels || !source.pixels) return;
    
    int srcX = srcRect.x;
    int srcY = srcRect.y;
    int srcW = srcRect.width;
    int srcH = srcRect.height;
    
    // Clamp source rect
    if (srcX < 0) { srcW += srcX; x -= srcX; srcX = 0; }
    if (srcY < 0) { srcH += srcY; y -= srcY; srcY = 0; }
    if (srcX + srcW > source.width) srcW = source.width - srcX;
    if (srcY + srcH > source.height) srcH = source.height - srcY;
    
    for (int sy = 0; sy < srcH; sy++)
    {
        for (int sx = 0; sx < srcW; sx++)
        {
            int dx = x + sx;
            int dy = y + sy;
            
            if (dx >= 0 && dx < width && dy >= 0 && dy < height)
            {
                Color c = source.GetPixelColor(srcX + sx, srcY + sy);
                
                // Alpha blend se necessário
                if (source.components == 4 || source.components == 2)
                {
                    if (c.a == 255)
                    {
                        SetPixel(dx, dy, c.r, c.g, c.b, c.a);
                    }
                    else if (c.a > 0)
                    {
                        Color dst = GetPixelColor(dx, dy);
                        float alpha = c.a / 255.0f;
                        
                        u8 r = (u8)(c.r * alpha + dst.r * (1.0f - alpha));
                        u8 g = (u8)(c.g * alpha + dst.g * (1.0f - alpha));
                        u8 b = (u8)(c.b * alpha + dst.b * (1.0f - alpha));
                        
                        SetPixel(dx, dy, r, g, b, dst.a);
                    }
                }
                else
                {
                    SetPixel(dx, dy, c.r, c.g, c.b, 255);
                }
            }
        }
    }
}

// Box Blur
Pixmap* Pixmap::ApplyBlur(int radius) const
{
    if (!pixels || radius < 1) return nullptr;
    
    Pixmap* result = new Pixmap(width, height, components);
    
    int kernelSize = radius * 2 + 1;
    int kernelArea = kernelSize * kernelSize;
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int r = 0, g = 0, b = 0, a = 0;
            
            for (int ky = -radius; ky <= radius; ky++)
            {
                for (int kx = -radius; kx <= radius; kx++)
                {
                    int sx = x + kx;
                    int sy = y + ky;
                    
                    // Clamp aos limites
                    sx = (sx < 0) ? 0 : (sx >= width ? width - 1 : sx);
                    sy = (sy < 0) ? 0 : (sy >= height ? height - 1 : sy);
                    
                    Color c = GetPixelColor(sx, sy);
                    r += c.r;
                    g += c.g;
                    b += c.b;
                    a += c.a;
                }
            }
            
            result->SetPixel(x, y, r / kernelArea, g / kernelArea, 
                           b / kernelArea, a / kernelArea);
        }
    }
    
    return result;
}

// Gaussian Blur (mais suave)
Pixmap* Pixmap::ApplyGaussianBlur(int radius) const
{
    if (!pixels || radius < 1) return nullptr;
    
    Pixmap* result = new Pixmap(width, height, components);
    
    // Kernel gaussiano 3x3 simples
    float kernel[3][3] = {
        {1.0f/16, 2.0f/16, 1.0f/16},
        {2.0f/16, 4.0f/16, 2.0f/16},
        {1.0f/16, 2.0f/16, 1.0f/16}
    };
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float r = 0, g = 0, b = 0, a = 0;
            
            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int sx = x + kx;
                    int sy = y + ky;
                    
                    sx = (sx < 0) ? 0 : (sx >= width ? width - 1 : sx);
                    sy = (sy < 0) ? 0 : (sy >= height ? height - 1 : sy);
                    
                    Color c = GetPixelColor(sx, sy);
                    float weight = kernel[ky + 1][kx + 1];
                    
                    r += c.r * weight;
                    g += c.g * weight;
                    b += c.b * weight;
                    a += c.a * weight;
                }
            }
            
            result->SetPixel(x, y, (u8)r, (u8)g, (u8)b, (u8)a);
        }
    }
    
    return result;
}

// Sharpen
Pixmap* Pixmap::ApplySharpen() const
{
    if (!pixels) return nullptr;
    
    Pixmap* result = new Pixmap(width, height, components);
    
    // Kernel de sharpening
    float kernel[3][3] = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float r = 0, g = 0, b = 0;
            
            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int sx = x + kx;
                    int sy = y + ky;
                    
                    sx = (sx < 0) ? 0 : (sx >= width ? width - 1 : sx);
                    sy = (sy < 0) ? 0 : (sy >= height ? height - 1 : sy);
                    
                    Color c = GetPixelColor(sx, sy);
                    float weight = kernel[ky + 1][kx + 1];
                    
                    r += c.r * weight;
                    g += c.g * weight;
                    b += c.b * weight;
                }
            }
            
            // Clamp valores
            r = (r < 0) ? 0 : (r > 255 ? 255 : r);
            g = (g < 0) ? 0 : (g > 255 ? 255 : g);
            b = (b < 0) ? 0 : (b > 255 ? 255 : b);
            
            Color original = GetPixelColor(x, y);
            result->SetPixel(x, y, (u8)r, (u8)g, (u8)b, original.a);
        }
    }
    
    return result;
}

// Edge Detection (Sobel)
Pixmap* Pixmap::ApplyEdgeDetection() const
{
    if (!pixels) return nullptr;
    
    Pixmap* result = new Pixmap(width, height, components);
    
    // Sobel kernels
    float sobelX[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    
    float sobelY[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float gx = 0, gy = 0;
            
            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int sx = x + kx;
                    int sy = y + ky;
                    
                    sx = (sx < 0) ? 0 : (sx >= width ? width - 1 : sx);
                    sy = (sy < 0) ? 0 : (sy >= height ? height - 1 : sy);
                    
                    Color c = GetPixelColor(sx, sy);
                    // Converter para grayscale
                    float gray = c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
                    
                    gx += gray * sobelX[ky + 1][kx + 1];
                    gy += gray * sobelY[ky + 1][kx + 1];
                }
            }
            
            float magnitude = sqrt(gx * gx + gy * gy);
            magnitude = (magnitude > 255) ? 255 : magnitude;
            
            u8 edge = (u8)magnitude;
            result->SetPixel(x, y, edge, edge, edge, 255);
        }
    }
    
    return result;
}

// Emboss
Pixmap* Pixmap::ApplyEmboss() const
{
    if (!pixels) return nullptr;
    
    Pixmap* result = new Pixmap(width, height, components);
    
    // Kernel de emboss
    float kernel[3][3] = {
        {-2, -1,  0},
        {-1,  1,  1},
        { 0,  1,  2}
    };
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float r = 0, g = 0, b = 0;
            
            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int sx = x + kx;
                    int sy = y + ky;
                    
                    sx = (sx < 0) ? 0 : (sx >= width ? width - 1 : sx);
                    sy = (sy < 0) ? 0 : (sy >= height ? height - 1 : sy);
                    
                    Color c = GetPixelColor(sx, sy);
                    float weight = kernel[ky + 1][kx + 1];
                    
                    r += c.r * weight;
                    g += c.g * weight;
                    b += c.b * weight;
                }
            }
            
            // Adicionar 128 para centrar o resultado
            r = r + 128;
            g = g + 128;
            b = b + 128;
            
            r = (r < 0) ? 0 : (r > 255 ? 255 : r);
            g = (g < 0) ? 0 : (g > 255 ? 255 : g);
            b = (b < 0) ? 0 : (b > 255 ? 255 : b);
            
            Color original = GetPixelColor(x, y);
            result->SetPixel(x, y, (u8)r, (u8)g, (u8)b, original.a);
        }
    }
    
    return result;
}
 
Pixmap* Pixmap::Crop(const IntRect &rect) const
{
    if (!pixels) 
    {
        LogError("Cannot crop: pixels is null");
        return nullptr;
    }
    
    // Validar e ajustar rect aos limites da imagem
    int x = rect.x;
    int y = rect.y;
    int w = rect.width;
    int h = rect.height;
    
    // Clamp aos limites
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x >= width || y >= height)
    {
        LogWarning("Crop rect is outside image bounds");
        return nullptr;
    }
    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;
    
    // Verificar se o rect resultante é válido
    if (w <= 0 || h <= 0)
    {
        LogWarning("Invalid crop rect: width=%d, height=%d", w, h);
        return nullptr;
    }
    
    // Criar nova pixmap
    Pixmap* result = new Pixmap(w, h, components);
    if (!result || !result->pixels)
    {
        LogError("Failed to create cropped pixmap");
        if (result) delete result;
        return nullptr;
    }
    
    // Copiar dados linha por linha (otimizado)
    int bytesPerRow = w * components;
    
    for (int row = 0; row < h; row++)
    {
        unsigned char* srcRow = pixels + ((y + row) * width + x) * components;
        unsigned char* dstRow = result->pixels + row * bytesPerRow;
        memcpy(dstRow, srcRow, bytesPerRow);
    }
    
    return result;
}
 
Pixmap* Pixmap::Crop(int x, int y, int w, int h) const
{
    IntRect rect;
    rect.x = x;
    rect.y = y;
    rect.width = w;
    rect.height = h;
    
    return Crop(rect);
}

 
Pixmap* Pixmap::CropExtended(const IntRect &rect, bool fillTransparent) const
{
    if (!pixels) 
    {
        LogError("Cannot crop: pixels is null");
        return nullptr;
    }
    
    int x = rect.x;
    int y = rect.y;
    int w = rect.width;
    int h = rect.height;
    
    if (w <= 0 || h <= 0)
    {
        LogWarning("Invalid crop dimensions: width=%d, height=%d", w, h);
        return nullptr;
    }
    
    // Criar nova pixmap
    Pixmap* result = new Pixmap(w, h, components);
    if (!result || !result->pixels)
    {
        LogError("Failed to create cropped pixmap");
        if (result) delete result;
        return nullptr;
    }
    
    // Preencher com cor padrão
    if (fillTransparent && (components == 4 || components == 2))
    {
        result->Clear(); // Preencher com transparente (0,0,0,0)
    }
    else
    {
        result->Fill(0, 0, 0, 255); // Preencher com preto opaco
    }
    
    // Calcular região de overlap (área que realmente existe na imagem fonte)
    int srcStartX = std::max(0, x);
    int srcStartY = std::max(0, y);
    int srcEndX = std::min(width, x + w);
    int srcEndY = std::min(height, y + h);
    
    // Se não há overlap, retornar imagem vazia preenchida
    if (srcStartX >= srcEndX || srcStartY >= srcEndY)
    {
        return result;
    }
    
    // Calcular offset no destino
    int dstOffsetX = srcStartX - x;
    int dstOffsetY = srcStartY - y;
    
    // Dimensões da área a copiar
    int copyWidth = srcEndX - srcStartX;
    int copyHeight = srcEndY - srcStartY;
    
    // Copiar linha por linha (otimizado)
    int bytesPerRow = copyWidth * components;
    
    for (int row = 0; row < copyHeight; row++)
    {
        unsigned char* srcRow = pixels + ((srcStartY + row) * width + srcStartX) * components;
        unsigned char* dstRow = result->pixels + ((dstOffsetY + row) * w + dstOffsetX) * components;
        memcpy(dstRow, srcRow, bytesPerRow);
    }
    
    return result;
}