#include "pch.h"
#include "Batch.hpp"
#include "Color.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "GUI.hpp"
#include "Input.hpp"

Color LerpColor(const Color &a, const Color &b, float t)
{
    t = Clamp(t, 0.0f, 1.0f);
    return Color(
        static_cast<u8>(a.r + (b.r - a.r) * t),
        static_cast<u8>(a.g + (b.g - a.g) * t),
        static_cast<u8>(a.b + (b.b - a.b) * t),
        static_cast<u8>(a.a + (b.a - a.a) * t));
}

// ========================================
// GUI Implementation
// ========================================
GUI::GUI() {}
GUI::~GUI() { Release(); }

void GUI::Init(RenderBatch *batch, Font *font)
{
    m_batch = batch;
    m_font = font;
}

void GUI::Release()
{
    m_windows.clear();
    m_windowOrder.clear();
}

float GUI::GetWindowCenterX() const
{
    if (!m_currentWindow)
        return 0.0f;
    return m_currentWindow->bounds.width * 0.5f;
}

float GUI::GetWindowCenterY() const
{
    if (!m_currentWindow)
        return 0.0f;
    float contentHeight = m_currentWindow->bounds.height - m_theme.titleBarHeight;
    return m_theme.titleBarHeight + contentHeight * 0.5f;
}

float GUI::GetWindowContentWidth() const
{
    if (!m_currentWindow)
        return 0.0f;
    return m_currentWindow->bounds.width - m_theme.windowPadding * 2.0f;
}

float GUI::GetWindowContentHeight() const
{
    if (!m_currentWindow)
        return 0.0f;
    return m_currentWindow->bounds.height - m_theme.titleBarHeight - m_theme.windowPadding * 2.0f;
}

float GUI::AlignCenter(float itemWidth) const
{
    return GetWindowCenterX() - itemWidth * 0.5f;
}

float GUI::AlignRight(float itemWidth, float margin) const
{
    if (!m_currentWindow)
        return 0.0f;
    return m_currentWindow->bounds.width - itemWidth - margin;
}

float GUI::AlignLeft(float margin) const
{
    if (!m_currentWindow)
        return margin;
    return m_theme.windowPadding + margin;
}

// ========================================
// Frame Management
// ========================================

void GUI::BeginFrame()
{
    m_nextControlID = 0;
    m_activeControl = 0;
 
    m_mousePos = Input::GetMousePosition();
    
    
    
    
    ++m_frameCounter;
    m_widgetCounter = 0;
    
    // Soltou botão → limpa captura e termina drag
    if (Input::IsMouseReleased(MouseButton::LEFT))
    {
        m_activeID.clear();
        if (m_draggingWindow)
        {
            m_draggingWindow->isDragging = false;
            m_draggingWindow = nullptr;
        }
    }
    
    SortWindowsByZOrder();
    
    // Atualiza arrasto em progresso
    if (m_draggingWindow && m_draggingWindow->isDragging && Input::IsMouseDown(MouseButton::LEFT))
    {
        m_draggingWindow->bounds.x = m_mousePos.x - m_draggingWindow->dragOffset.x;
        m_draggingWindow->bounds.y = m_mousePos.y - m_draggingWindow->dragOffset.y;
        return;
    }
    
    // Descobre a janela topmost sob o rato
    WindowData *topWindow = nullptr;
    for (int i = static_cast<int>(m_windowOrder.size()) - 1; i >= 0; --i)
    {
        WindowData *w = m_windowOrder[i];
        if (!w->isOpen)
            continue;
        const FloatRect r = w->isMinimized
                                ? FloatRect(w->bounds.x, w->bounds.y, w->bounds.width, m_theme.titleBarHeight)
                                : w->bounds;
        if (IsPointInRect(m_mousePos, r))
        {
            topWindow = w;
            break;
        }
    }
    
    if (topWindow)
    {
        UpdateWindowInteraction(topWindow);
    }
    else if (Input::IsMousePressed(MouseButton::LEFT))
    {
        // Clique no vazio → limpa foco
        for (auto &p : m_windows)
            p.second.isFocused = false;
        m_focusedWindow = nullptr;
    }
}

 
void GUI::EndFrame()
{
    m_hotID.clear();
}

// ========================================
// Theme Management
// ========================================
void GUI::SetTheme(const GUITheme &theme) { m_theme = theme; }
GUITheme &GUI::GetTheme() { return m_theme; }

GUITheme GUI::DarkTheme() { return GUITheme(); }

GUITheme GUI::LightTheme()
{
    GUITheme t;
    t.windowBg = Color(240, 240, 245, 240);
    t.windowBorder = Color(180, 180, 185, 255);
    t.titleBarBg = Color(220, 220, 225, 255);
    t.titleBarActive = Color(200, 200, 210, 255);
    t.titleText = Color(20, 20, 20, 255);

    t.buttonBg = Color(220, 220, 225, 255);
    t.buttonHover = Color(200, 200, 210, 255);
    t.buttonActive = Color(100, 150, 255, 255);
    t.buttonText = Color(20, 20, 20, 255);

    t.labelText = Color(40, 40, 40, 255);
    t.separatorColor = Color(180, 180, 185, 255);
    return t;
}

GUITheme GUI::BlueTheme()
{
    GUITheme t;
    t.windowBg = Color(25, 35, 45, 240);
    t.titleBarBg = Color(30, 50, 70, 255);
    t.titleBarActive = Color(40, 70, 100, 255);
    t.buttonBg = Color(40, 60, 80, 255);
    t.buttonHover = Color(50, 80, 110, 255);
    t.buttonActive = Color(70, 120, 180, 255);
    return t;
}

// ========================================
// Utility
// ========================================
bool GUI::IsWindowHovered() const
{
    if (!m_currentWindow)
        return false;
    return IsPointInRect(m_mousePos, m_currentWindow->bounds);
}

bool GUI::IsWindowFocused() const
{
    if (!m_currentWindow)
        return false;
    return m_currentWindow->isFocused;
}

void GUI::SetNextWindowPos(float x, float y)
{
    m_nextWindowPos = Vec2(x, y);
    m_hasNextWindowPos = true;
}
void GUI::SetNextWindowSize(float width, float height)
{
    m_nextWindowSize = Vec2(width, height);
    m_hasNextWindowSize = true;
}

// Helpers
FloatRect GUI::MakeContentRect(float x, float y, float w, float h) const
{
    if (!m_currentWindow)
        return FloatRect();
    float ox = m_currentWindow->bounds.x + m_theme.windowPadding;
    float oy = m_currentWindow->bounds.y + m_theme.titleBarHeight + m_theme.windowPadding - m_currentWindow->scrollOffset.y;
    return FloatRect(ox + x, oy + y, w, h);
}

Vec2 GUI::MakeContentPos(float x, float y) const
{
    if (!m_currentWindow)
        return Vec2();
    float ox = m_currentWindow->bounds.x + m_theme.windowPadding;
    float oy = m_currentWindow->bounds.y + m_theme.titleBarHeight + m_theme.windowPadding - m_currentWindow->scrollOffset.y;
    return Vec2(ox + x, oy + y);
}

bool GUI::IsPointInRect(const Vec2 &p, const FloatRect &r) const
{
    return p.x >= r.x && p.x <= r.x + r.width &&
           p.y >= r.y && p.y <= r.y + r.height;
}

WidgetState GUI::GetWidgetState(const FloatRect &rect, const std::string &widgetID)
{
    if (IsPointInRect(m_mousePos, rect))
    {
        m_hotID = widgetID;
        m_lastWidgetID = widgetID;
        if (m_activeID == widgetID)
            return WidgetState::Active;
        return WidgetState::Hovered;
    }
    if (m_activeID == widgetID)
        return WidgetState::Active;
    return WidgetState::Normal;
}

void GUI::DrawRectFilled(const FloatRect &rect, const Color &color)
{
    if (!m_batch)
        return;
    m_batch->SetColor(color);
    m_batch->Rectangle((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, true);
}

void GUI::DrawRectOutline(const FloatRect &rect, const Color &color, float /*thickness*/)
{
    if (!m_batch)
        return;
    m_batch->SetColor(color);
    m_batch->Rectangle((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, false);
}

void GUI::DrawText(const char *text, float x, float y, const Color &color)
{
    if (!m_font)
        return;
    m_font->SetColor(color);
    m_font->Print(text, x, y);
}

float GUI::GetTextWidth(const char *text)
{
    if (!m_font)
        return 0.0f;
    return m_font->GetTextWidth(text);
}

float GUI::GetTextHeight(const char *text)
{
    if (!m_font)
        return 0.0f;
    return m_font->GetTextHeight(text);
}

std::string GUI::GenerateID(const char *label)
{
    const char *win = (m_currentWindow ? m_currentWindow->id.c_str() : "");
    char buf[256];

    std::snprintf(buf, sizeof(buf), "%s##%s_%d", label, win, m_widgetCounter++);
    return std::string(buf);
}

bool GUI::IsMouseInWindow() const
{
    if (!m_currentWindow)
        return false;
    return IsPointInRect(m_mousePos, m_currentWindow->bounds);
}

GUI::Control *GUI::AddControl()
{
   u32 id = GetNextControlID();
   auto it = m_controls.find(id);
   if (it == m_controls.end())
   {
       Control c;
       c.id = id;
       c.active = false;
       c.ftag = 0.0f;
       c.itag = 0;
       c.changed = false;
       m_controls[id] = c;
       return &m_controls[id];
   }
   return &it->second;
}


 

u32 GUI::GetNextControlID()
{
    return m_nextControlID++;
}

// ========================================
// Window Management
// ========================================
bool GUI::BeginWindow(const char *title, float x, float y, float w, float h, bool *open)
{
    return BeginWindow(title, FloatRect(x, y, w, h), open);
}

bool GUI::BeginWindow(const char *title, const FloatRect &bounds, bool *open)
{
   


    m_widgetCounter = 0;
    std::string id = title;
    WindowData *window = GetOrCreateWindow(id);

    // Next-window settings
    if (m_hasNextWindowPos)
    {
        window->bounds.x = m_nextWindowPos.x;
        window->bounds.y = m_nextWindowPos.y;
        m_hasNextWindowPos = false;
    }
    if (m_hasNextWindowSize)
    {
        window->bounds.width = m_nextWindowSize.x;
        window->bounds.height = m_nextWindowSize.y;
        m_hasNextWindowSize = false;
    }
    if (m_windowFlagsSet)
    {
        window->canClose = m_nextCanClose;
        window->canMinimize = m_nextCanMinimize;
        window->canResize = m_nextCanResize;
        window->canMove = m_nextCanMove;
        m_windowFlagsSet = false;
    }

    // Primeira vez
    if (window->bounds.width == 0.0f)
        window->bounds = bounds;

    // Close flag externo
    if (open)
    {
        if (!(*open))
            window->isOpen = false;
        *open = window->isOpen;
        if (!window->isOpen)
            return false;
    }
    else if (!window->isOpen)
    {
        return false;
    }

    m_currentWindow = window;

    // Desenhar janela
    RenderWindow(window, title);

    // Define origem e largura útil (para quem quiser defaults)
    if (!window->isMinimized)
    {
        m_cursorPos.x = window->bounds.x + m_theme.windowPadding;
        m_cursorPos.y = window->bounds.y + m_theme.titleBarHeight + m_theme.windowPadding - window->scrollOffset.y;
        m_maxItemWidth = window->bounds.width - m_theme.windowPadding * 2.0f;
    }

    return !window->isMinimized;
}

void GUI::EndWindow()
{
    if (!m_currentWindow)
        return;
    m_currentWindow->contentHeight =
        m_cursorPos.y - (m_currentWindow->bounds.y + m_theme.titleBarHeight + m_theme.windowPadding) + m_currentWindow->scrollOffset.y;
    m_currentWindow = nullptr;
}

void GUI::SetWindowFlags(bool canClose, bool canMinimize, bool canResize, bool canMove)
{
    m_nextCanClose = canClose;
    m_nextCanMinimize = canMinimize;
    m_nextCanResize = canResize;
    m_nextCanMove = canMove;
    m_windowFlagsSet = true;
}

WindowData *GUI::GetOrCreateWindow(const std::string &id)
{
    auto it = m_windows.find(id);
    if (it == m_windows.end())
    {
        WindowData w;
        w.id = id;
        w.zOrder = static_cast<int>(m_windows.size());
        m_windows[id] = w;
        m_windowOrder.push_back(&m_windows[id]);
        return &m_windows[id];
    }
    return &it->second;
}

WindowData *GUI::GetCurrentWindow() { return m_currentWindow; }

void GUI::UpdateWindowInteraction(WindowData *window)
{
    if (!window)
        return;

    FloatRect titleBarRect(window->bounds.x, window->bounds.y, window->bounds.width, m_theme.titleBarHeight);
    float buttonX = window->bounds.x + window->bounds.width - m_theme.titleBarHeight;

    FloatRect closeButtonRect(buttonX, window->bounds.y, m_theme.titleBarHeight, m_theme.titleBarHeight);

    FloatRect minimizeButtonRect;
    if (window->canClose && window->canMinimize)
        minimizeButtonRect = FloatRect(buttonX - m_theme.titleBarHeight, window->bounds.y, m_theme.titleBarHeight, m_theme.titleBarHeight);
    else if (window->canMinimize)
        minimizeButtonRect = closeButtonRect;

    const FloatRect interactionRect = window->isMinimized
                                          ? FloatRect(window->bounds.x, window->bounds.y, window->bounds.width, m_theme.titleBarHeight)
                                          : window->bounds;

    const bool mouseInTitleBar = IsPointInRect(m_mousePos, titleBarRect);
    const bool mouseInWindow = IsPointInRect(m_mousePos, interactionRect);

    // Botões
    if (window->canClose && Input::IsMousePressed(MouseButton::LEFT) && IsPointInRect(m_mousePos, closeButtonRect))
    {
        window->isOpen = false;
        return;
    }
    if (window->canMinimize && Input::IsMousePressed(MouseButton::LEFT) && IsPointInRect(m_mousePos, minimizeButtonRect))
    {
        window->isMinimized = !window->isMinimized;
        return;
    }

    // Foco apenas se o clique foi nesta janela
    if (Input::IsMousePressed(MouseButton::LEFT) && mouseInWindow)
    {
        for (auto &p : m_windows)
            p.second.isFocused = false;
        window->isFocused = true;
        m_focusedWindow = window;
        window->zOrder = 1000 + m_frameCounter;
    }

    // Drag
    if (window->canMove && mouseInTitleBar && m_activeID.empty() && Input::IsMousePressed(MouseButton::LEFT))
    {
        window->isDragging = true;
        m_draggingWindow = window;
        window->dragOffset = Vec2(m_mousePos.x - window->bounds.x, m_mousePos.y - window->bounds.y);
    }
}

void GUI::RenderWindow(WindowData *window, const char *title)
{
    if (!window || !m_batch)
        return;

    // Minimizada → só titlebar
    if (window->isMinimized)
    {
        FloatRect r(window->bounds.x, window->bounds.y, window->bounds.width, m_theme.titleBarHeight);
        Color titleBarColor = window->isFocused ? m_theme.titleBarActive : m_theme.titleBarBg;
        DrawRectFilled(r, titleBarColor);
        DrawRectOutline(r, m_theme.windowBorder, m_theme.windowBorderSize);

        float titleX = window->bounds.x + m_theme.windowPadding;
        float titleY = window->bounds.y + (m_theme.titleBarHeight - GetTextHeight(title)) * 0.5f;
        DrawText(title, titleX, titleY, m_theme.titleText);

        float buttonX = window->bounds.x + window->bounds.width - m_theme.titleBarHeight;

        // Close
        if (window->canClose)
        {
            FloatRect closeButtonRect(buttonX, window->bounds.y, m_theme.titleBarHeight, m_theme.titleBarHeight);
            Color c = IsPointInRect(m_mousePos, closeButtonRect) ? m_theme.buttonHover : titleBarColor;
            DrawRectFilled(closeButtonRect, c);

            float p = 8.0f;
            m_batch->SetColor(m_theme.titleText);
            m_batch->Line2D(Vec2(closeButtonRect.x + p, closeButtonRect.y + p),
                            Vec2(closeButtonRect.x + closeButtonRect.width - p, closeButtonRect.y + closeButtonRect.height - p));
            m_batch->Line2D(Vec2(closeButtonRect.x + closeButtonRect.width - p, closeButtonRect.y + p),
                            Vec2(closeButtonRect.x + p, closeButtonRect.y + closeButtonRect.height - p));
            buttonX -= m_theme.titleBarHeight;
        }

        // Minimize (ícone expand)
        if (window->canMinimize)
        {
            FloatRect minimizeButtonRect(buttonX, window->bounds.y, m_theme.titleBarHeight, m_theme.titleBarHeight);
            Color c = IsPointInRect(m_mousePos, minimizeButtonRect) ? m_theme.buttonHover : titleBarColor;
            DrawRectFilled(minimizeButtonRect, c);

            float p = 8.0f;
            m_batch->SetColor(m_theme.titleText);
            FloatRect icon(minimizeButtonRect.x + p, minimizeButtonRect.y + p,
                           minimizeButtonRect.width - 2 * p, minimizeButtonRect.height - 2 * p);
            m_batch->Rectangle((int)icon.x, (int)icon.y, (int)icon.width, (int)icon.height, false);
        }
        return;
    }

    // Fundo da janela
    DrawRectFilled(window->bounds, m_theme.windowBg);
    DrawRectOutline(window->bounds, m_theme.windowBorder, m_theme.windowBorderSize);

    // Title bar
    FloatRect titleBarRect(window->bounds.x, window->bounds.y, window->bounds.width, m_theme.titleBarHeight);
    Color titleBarColor = window->isFocused ? m_theme.titleBarActive : m_theme.titleBarBg;
    DrawRectFilled(titleBarRect, titleBarColor);

    float titleX = window->bounds.x + m_theme.windowPadding;
    float titleY = window->bounds.y + (m_theme.titleBarHeight - GetTextHeight(title)) * 0.5f;
    DrawText(title, titleX, titleY, m_theme.titleText);

    float buttonX = window->bounds.x + window->bounds.width - m_theme.titleBarHeight;

    // Close
    if (window->canClose)
    {
        FloatRect closeButtonRect(buttonX, window->bounds.y, m_theme.titleBarHeight, m_theme.titleBarHeight);
        Color c = IsPointInRect(m_mousePos, closeButtonRect) ? m_theme.buttonHover : titleBarColor;
        DrawRectFilled(closeButtonRect, c);

        float p = 8.0f;
        m_batch->SetColor(m_theme.titleText);
        m_batch->Line2D(Vec2(closeButtonRect.x + p, closeButtonRect.y + p),
                        Vec2(closeButtonRect.x + closeButtonRect.width - p, closeButtonRect.y + closeButtonRect.height - p));
        m_batch->Line2D(Vec2(closeButtonRect.x + closeButtonRect.width - p, closeButtonRect.y + p),
                        Vec2(closeButtonRect.x + p, closeButtonRect.y + closeButtonRect.height - p));
        buttonX -= m_theme.titleBarHeight;
    }

    // Minimize
    if (window->canMinimize)
    {
        FloatRect minimizeButtonRect(buttonX, window->bounds.y, m_theme.titleBarHeight, m_theme.titleBarHeight);
        Color c = IsPointInRect(m_mousePos, minimizeButtonRect) ? m_theme.buttonHover : titleBarColor;
        DrawRectFilled(minimizeButtonRect, c);

        float p = 8.0f;
        m_batch->SetColor(m_theme.titleText);
        float lineY = minimizeButtonRect.y + minimizeButtonRect.height - p - 2.0f;
        m_batch->Line2D(Vec2(minimizeButtonRect.x + p, lineY),
                        Vec2(minimizeButtonRect.x + minimizeButtonRect.width - p, lineY));
    }
}

void GUI::SortWindowsByZOrder()
{
    std::sort(m_windowOrder.begin(), m_windowOrder.end(),
              [](WindowData *a, WindowData *b)
              { return a->zOrder < b->zOrder; });
}

void GUI::BringCurrentWindowToFront()
{
    if (!m_currentWindow)
        return;
    m_currentWindow->isFocused = true;
    m_currentWindow->zOrder = 1000 + m_frameCounter;
    SortWindowsByZOrder();
}

// ========================================
// Widgets (posicionamento explícito)
// ========================================
bool GUI::Button(const char *label, float x, float y, float w, float h)
{
    GUI::Control *c = AddControl();
    if (!c)
        return false;
    m_activeControl = c->id;
    FloatRect rect = MakeContentRect(x, y, w, h);
    bool isHovered = IsPointInRect(m_mousePos, rect) && m_activeControl == c->id;
    if ( isHovered && Input::IsMousePressed(MouseButton::LEFT))
    {
      
        c->changed = true;
        c->active = true;
        
    } else 
    {
        c->changed = false;
        c->active = false;
    }

    Color bg = m_theme.buttonBg;
    if (c->active && m_activeControl == c->id)
        bg = m_theme.buttonActive;
    else if (isHovered)
        bg = m_theme.buttonHover;

    DrawRectFilled(rect, bg);
    DrawRectOutline(rect, m_theme.windowBorder, 1.0f);

    float tw = GetTextWidth(label);
    float th = GetTextHeight(label);
    DrawText(label, rect.x + (rect.width - tw) * 0.5f,
             rect.y + (rect.height - th) * 0.5f, m_theme.buttonText);
    return c->changed;
}

bool GUI::Checkbox(const char *label, bool *value, float x, float y, float size)
{
     GUI::Control *c = AddControl();
    if (!c)
        return false;
    m_activeControl = c->id;

    FloatRect box = MakeContentRect(x, y, size, size);
    Color boxColor = m_theme.checkboxBg;
     bool isHovered = IsPointInRect(m_mousePos, box) && m_activeControl == c->id;
 

 
    if ( isHovered && Input::IsMousePressed(MouseButton::LEFT))
    {
        *value = !*value;
        c->changed = true;

       
    }
    if ( isHovered )
        boxColor = LerpColor(m_theme.checkboxBg, m_theme.buttonHover, 0.5f);

    DrawRectFilled(box, boxColor);
    DrawRectOutline(box, m_theme.checkboxBorder, 1.0f);

    if (*value)
    {
        float p = 3.0f;
        DrawRectFilled(FloatRect(box.x + p, box.y + p, box.width - 2 * p, box.height - 2 * p), m_theme.checkboxCheck);
    }

    float ly = box.y + (box.height - GetTextHeight(label)) * 0.5f;
    DrawText(label, box.x + box.width + m_theme.itemSpacing, ly, m_theme.labelText);
    return c->changed;
}

bool GUI::SliderFloat(const char *label, float *value, float min, float max,
                      float x, float y, float w, float h)
{

    GUI::Control *c = AddControl();
    if (!c)
        return false;
    m_activeControl = c->id;
    FloatRect rect = MakeContentRect(x, y, w, h);
    float lw = GetTextWidth(label) + m_theme.itemSpacing * 2.0f;
    float vw = GetTextWidth("9999.99") + m_theme.itemSpacing * 2.0f;
    float sw = std::max(50.0f, rect.width - lw - vw);
    FloatRect slider(rect.x + lw, rect.y, sw, rect.height);

    bool isHovered = IsPointInRect(m_mousePos, slider) ;
 

   
    
    if ( isHovered && Input::IsMousePressed(MouseButton::LEFT))
    {
        
        c->active = true;
        float t = Clamp((m_mousePos.x - slider.x) / slider.width, 0.0f, 1.0f);
        *value = min + t * (max - min);
        c->changed = true;
    } 

    if (Input::IsMouseReleased(MouseButton::LEFT))
    {
        c->active = false;
    } 

    if (c->active && Input::IsMouseDown(MouseButton::LEFT))
    {

        float t = Clamp((m_mousePos.x - slider.x) / slider.width, 0.0f, 1.0f);
        float nv = min + t * (max - min);
        if (nv != *value)
        {
            *value = nv;
            c->changed = true;
        }
    }

    DrawRectFilled(slider, m_theme.sliderBg);
    DrawRectOutline(slider, m_theme.windowBorder, 1.0f);

    float t = Clamp((*value - min) / (max - min), 0.0f, 1.0f);
    float fw = t * slider.width;
    if (fw > 0)
        DrawRectFilled(FloatRect(slider.x, slider.y, fw, slider.height), m_theme.sliderFill);

    float handleW = 12.0f;
    float hx = Clamp(slider.x + fw - handleW * 0.5f, slider.x, slider.x + slider.width - handleW);
    FloatRect handle(hx, slider.y - 2.0f, handleW, slider.height + 4.0f);
    Color hc = (isHovered  && m_activeControl == c->id) ? m_theme.sliderHandleHover : m_theme.sliderHandle;
    DrawRectFilled(handle, hc);
    DrawRectOutline(handle, m_theme.windowBorder, 1.0f);

    DrawText(label, rect.x, rect.y + (rect.height - GetTextHeight(label)) * 0.5f, m_theme.labelText);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", *value);
    DrawText(buf, slider.x + slider.width + m_theme.itemSpacing,
             rect.y + (rect.height - GetTextHeight(buf)) * 0.5f, m_theme.labelText);

    return c->changed;
}

bool GUI::SliderInt(const char *label, int *value, int min, int max,float x, float y, float w, float h)
{
    GUI::Control *c = AddControl();
    if (!c)
        return false;
    m_activeControl = c->id;
    FloatRect rect = MakeContentRect(x, y, w, h);
    float lw = GetTextWidth(label) + m_theme.itemSpacing * 2.0f;
    float vw = GetTextWidth("999999") + m_theme.itemSpacing * 2.0f;
    float sw = std::max(50.0f, rect.width - lw - vw);
    FloatRect slider(rect.x + lw, rect.y, sw, rect.height);

 

    bool isHovered = IsPointInRect(m_mousePos, slider) && m_activeControl == c->id;
 

    if ( isHovered && Input::IsMousePressed(MouseButton::LEFT))
    {
        float t = Clamp((m_mousePos.x - slider.x) / slider.width, 0.0f, 1.0f);
        int nv = min + int(t * (max - min) + 0.5f);
        if (nv != *value)
        {
            *value = nv;
            c->changed = true;
        }
        c->active = true;
    } 

    if (c->active && Input::IsMouseReleased(MouseButton::LEFT))
    {
        c->active = false;
    } 
    

 
    if (c->active && Input::IsMouseDown(MouseButton::LEFT))
    {
        float t = Clamp((m_mousePos.x - slider.x) / slider.width, 0.0f, 1.0f);
        int nv = min + int(t * (max - min) + 0.5f);
        if (nv != *value)
        {
            *value = nv;
            c->changed = true;
        }
    }

    DrawRectFilled(slider, m_theme.sliderBg);
    DrawRectOutline(slider, m_theme.windowBorder, 1.0f);

    float t = Clamp(float(*value - min) / float(max - min), 0.0f, 1.0f);
    float fw = t * slider.width;
    if (fw > 0)
        DrawRectFilled(FloatRect(slider.x, slider.y, fw, slider.height), m_theme.sliderFill);

    float handleW = 12.0f;
    float hx = Clamp(slider.x + fw - handleW * 0.5f, slider.x, slider.x + slider.width - handleW);
    FloatRect handle(hx, slider.y - 2.0f, handleW, slider.height + 4.0f);
    Color hc = (isHovered  && m_activeControl == c->id) ? m_theme.sliderHandleHover : m_theme.sliderHandle;
    DrawRectFilled(handle, hc);
    DrawRectOutline(handle, m_theme.windowBorder, 1.0f);

    DrawText(label, rect.x, rect.y + (rect.height - GetTextHeight(label)) * 0.5f, m_theme.labelText);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", *value);
    DrawText(buf, slider.x + slider.width + m_theme.itemSpacing,
             rect.y + (rect.height - GetTextHeight(buf)) * 0.5f, m_theme.labelText);

    return c->changed;
}

bool GUI::SliderFloatVertical(const char *label, float *value, float min, float max,
                              float x, float y, float w, float h)
{

     GUI::Control *c = AddControl();
    if (!c)
        return false;
    m_activeControl = c->id;

    FloatRect rect = MakeContentRect(x, y, w, h);

    // label por cima
    DrawText(label, rect.x + (rect.width - GetTextWidth(label)) * 0.5f,
             rect.y - (GetTextHeight(label) + m_theme.itemSpacing), m_theme.labelText);

   
    bool isHovered = IsPointInRect(m_mousePos, rect) && m_activeControl == c->id;
    if ( isHovered && Input::IsMousePressed(MouseButton::LEFT))
    {
        float t = Clamp((rect.y + rect.height - m_mousePos.y) / rect.height, 0.0f, 1.0f);
        float nv = min + t * (max - min);
        if (nv != *value)
        {
            *value = nv;
            c->changed = true;
        }
        c->active = true;
    }
    if (c->active && Input::IsMouseReleased(MouseButton::LEFT))
    {
        c->active = false;
    } 
    
  
    if (c->active && Input::IsMouseDown(MouseButton::LEFT))
    {
        float t = Clamp((rect.y + rect.height - m_mousePos.y) / rect.height, 0.0f, 1.0f);
        float nv = min + t * (max - min);
        if (nv != *value)
        {
            *value = nv;
            c->changed = true;
        }
    }

    DrawRectFilled(rect, m_theme.sliderBg);
    DrawRectOutline(rect, m_theme.windowBorder, 1.0f);

    float t = Clamp((*value - min) / (max - min), 0.0f, 1.0f);
    float fh = t * rect.height;
    if (fh > 0)
        DrawRectFilled(FloatRect(rect.x, rect.y + rect.height - fh, rect.width, fh), m_theme.sliderFill);

    float handleH = 12.0f;
    float hy = Clamp(rect.y + rect.height - fh - handleH * 0.5f, rect.y, rect.y + rect.height - handleH);
    FloatRect handle(rect.x - 2.0f, hy, rect.width + 4.0f, handleH);
        Color hc = (isHovered  && m_activeControl == c->id) ? m_theme.sliderHandleHover : m_theme.sliderHandle;
    DrawRectFilled(handle, hc);
    DrawRectOutline(handle, m_theme.windowBorder, 1.0f);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", *value);
    DrawText(buf, rect.x + (rect.width - GetTextWidth(buf)) * 0.5f,
             rect.y + rect.height + m_theme.itemSpacing, m_theme.labelText);
    return c->changed;
}

bool GUI::SliderIntVertical(const char *label, int *value, int min, int max,
                            float x, float y, float w, float h)
{

      GUI::Control *c = AddControl();
    if (!c)
        return false;
    m_activeControl = c->id;


    FloatRect rect = MakeContentRect(x, y, w, h);

    DrawText(label, rect.x + (rect.width - GetTextWidth(label)) * 0.5f,
             rect.y - (GetTextHeight(label) + m_theme.itemSpacing), m_theme.labelText);

    bool isHovered = IsPointInRect(m_mousePos, rect) && m_activeControl == c->id;

    if ( isHovered && Input::IsMousePressed(MouseButton::LEFT))
    {
        float t = Clamp((rect.y + rect.height - m_mousePos.y) / rect.height, 0.0f, 1.0f);
        int nv = min + int(t * (max - min) + 0.5f);
        if (nv != *value)
        {
            *value = nv;
            c->changed = true;
        }
        c->active = true;
        
    }
    if (c->active && Input::IsMouseReleased(MouseButton::LEFT))
    {
        c->active = false;
    } 
    
    
 
    if (c->active && Input::IsMouseDown(MouseButton::LEFT))
    {
        float t = Clamp((rect.y + rect.height - m_mousePos.y) / rect.height, 0.0f, 1.0f);
        int nv = min + int(t * (max - min) + 0.5f);
        if (nv != *value)
        {
            *value = nv;
            c->changed = true;
        }
    }

    DrawRectFilled(rect, m_theme.sliderBg);
    DrawRectOutline(rect, m_theme.windowBorder, 1.0f);

    float t = Clamp(float(*value - min) / float(max - min), 0.0f, 1.0f);
    float fh = t * rect.height;
    if (fh > 0)
        DrawRectFilled(FloatRect(rect.x, rect.y + rect.height - fh, rect.width, fh), m_theme.sliderFill);

    float handleH = 12.0f;
    float hy = Clamp(rect.y + rect.height - fh - handleH * 0.5f, rect.y, rect.y + rect.height - handleH);
    FloatRect handle(rect.x - 2.0f, hy, rect.width + 4.0f, handleH);
        Color hc = (isHovered  && m_activeControl == c->id) ? m_theme.sliderHandleHover : m_theme.sliderHandle;
    DrawRectFilled(handle, hc);
    DrawRectOutline(handle, m_theme.windowBorder, 1.0f);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", *value);
    DrawText(buf, rect.x + (rect.width - GetTextWidth(buf)) * 0.5f,
             rect.y + rect.height + m_theme.itemSpacing, m_theme.labelText);
    return c->changed;
}

void GUI::Label(const char *text, float x, float y)
{
    Vec2 p = MakeContentPos(x, y);
    DrawText(text, p.x, p.y, m_theme.labelText);
}

void GUI::LabelColored(const char *text, const Color &color, float x, float y)
{
    Vec2 p = MakeContentPos(x, y);
    DrawText(text, p.x, p.y, color);
}

void GUI::Text(float x, float y, const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Label(buf, x, y);
}

void GUI::Separator(float x, float y, float w)
{
    DrawRectFilled(MakeContentRect(x, y, w, 2.0f), m_theme.separatorColor);
}

// ========================================
// ColorPicker
// ========================================

static void RGBtoHSV(float r, float g, float b, float &h, float &s, float &v)
{
    float cmax = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    float cmin = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    float delta = cmax - cmin;

    v = cmax;
    s = (cmax > 0.0f) ? (delta / cmax) : 0.0f;

    if (delta > 0.0f)
    {
        if (r >= cmax)
            h = (g - b) / delta;
        else if (g >= cmax)
            h = 2.0f + (b - r) / delta;
        else
            h = 4.0f + (r - g) / delta;
        h *= 60.0f;
        if (h < 0)
            h += 360.0f;
    }
    else
    {
        h = 0.0f;
    }
}

// Helper: HSV to RGB
static void HSVtoRGB(float h, float s, float v, float &r, float &g, float &b)
{
    if (s <= 0.0f)
    {
        r = g = b = v;
        return;
    }

    float hh = h / 60.0f;
    int i = (int)hh;
    float ff = hh - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - (s * ff));
    float t = v * (1.0f - (s * (1.0f - ff)));

    switch (i % 6)
    {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
        r = v;
        g = p;
        b = q;
        break;
    }
}

static Color GetHueColor(float hue)
{
    float r, g, b;
    HSVtoRGB(hue, 1.0f, 1.0f, r, g, b);
    return Color((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), 255);
}
bool GUI::ColorPicker(const char *label, Color *color, float x, float y, float w)
{
    if (!color)
        return false;

    std::string id = GenerateID(label);
    bool changed = false;
    float spacing = m_theme.itemSpacing;

    // Label
    if (label && label[0] != '\0')
    {
        Label(label, x, y);
        y += GetTextHeight(label) + spacing;
    }

    // Convert current color to HSV
    float h, s, v;
    RGBtoHSV(color->r / 255.0f, color->g / 255.0f, color->b / 255.0f, h, s, v);

    // Store HSV per widget (para manter estado)
    static std::unordered_map<std::string, Vec3> hsvCache;
    if (hsvCache.find(id) == hsvCache.end())
    {
        hsvCache[id] = Vec3(h, s, v);
    }
    Vec3 &hsv = hsvCache[id];

    // ========================================
    // SV Box (Saturation/Value gradient)
    // ========================================
    float boxSize = w;
    FloatRect svBox = MakeContentRect(x, y, boxSize, boxSize);

    // Desenha gradient (simplificado - 4x4 grid de cores)
    Color hueColor = GetHueColor(hsv.x);
    int gridSize = 32;
    float cellW = svBox.width / gridSize;
    float cellH = svBox.height / gridSize;

    for (int iy = 0; iy < gridSize; ++iy)
    {
        for (int ix = 0; ix < gridSize; ++ix)
        {
            float saturation = ix / (float)(gridSize - 1);
            float value = 1.0f - (iy / (float)(gridSize - 1));

            float r, g, b;
            HSVtoRGB(hsv.x, saturation, value, r, g, b);

            FloatRect cell{
                svBox.x + ix * cellW,
                svBox.y + iy * cellH,
                cellW + 1, // +1 para evitar gaps
                cellH + 1};

            DrawRectFilled(cell, Color((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), 255));
        }
    }

    DrawRectOutline(svBox, m_theme.colorPickerBorder, 2.0f);

    // Cursor na posição atual (s, v)
    float cursorX = svBox.x + hsv.y * svBox.width;
    float cursorY = svBox.y + (1.0f - hsv.z) * svBox.height;

    // Círculo do cursor (ou quadrado)
    DrawRectOutline(FloatRect{cursorX - 5, cursorY - 5, 10, 10}, Color(255, 255, 255, 255), 2.0f);
    DrawRectOutline(FloatRect{cursorX - 4, cursorY - 4, 8, 8}, Color(0, 0, 0, 255), 1.0f);

    // Interação com SV box
    bool svHovered = IsPointInRect(m_mousePos, svBox);
    static std::string activeSVBox;

    if (svHovered && Input::IsMousePressed(MouseButton::LEFT))
    {
        activeSVBox = id;
    }

    if (activeSVBox == id && Input::IsMouseDown(MouseButton::LEFT))
    {
        float sx = (m_mousePos.x - svBox.x) / svBox.width;
        float sy = (m_mousePos.y - svBox.y) / svBox.height;

        if (sx < 0)
            sx = 0;
        if (sx > 1)
            sx = 1;
        if (sy < 0)
            sy = 0;
        if (sy > 1)
            sy = 1;

        hsv.y = sx;        // saturation
        hsv.z = 1.0f - sy; // value

        // Convert back to RGB
        float r, g, b;
        HSVtoRGB(hsv.x, hsv.y, hsv.z, r, g, b);
        color->r = (uint8_t)(r * 255);
        color->g = (uint8_t)(g * 255);
        color->b = (uint8_t)(b * 255);
        changed = true;
    }

    if (!Input::IsMouseDown(MouseButton::LEFT))
    {
        activeSVBox.clear();
    }

    y += boxSize + spacing * 2;

    // ========================================
    // Hue Slider (rainbow bar)
    // ========================================
    float hueBarH = 20;
    FloatRect hueBar = MakeContentRect(x, y, boxSize, hueBarH);

    // Desenha rainbow gradient
    int hueSteps = 60;
    float stepW = hueBar.width / hueSteps;

    for (int i = 0; i < hueSteps; ++i)
    {
        float hue = (i / (float)hueSteps) * 360.0f;
        Color hCol = GetHueColor(hue);

        FloatRect step{hueBar.x + i * stepW, hueBar.y, stepW + 1, hueBar.height};
        DrawRectFilled(step, hCol);
    }

    DrawRectOutline(hueBar, m_theme.colorPickerBorder, 2.0f);

    // Cursor do hue
    float hueX = hueBar.x + (hsv.x / 360.0f) * hueBar.width;
    DrawRectFilled(FloatRect{hueX - 2, hueBar.y - 2, 4, hueBar.height + 4}, Color(255, 255, 255, 255));
    DrawRectOutline(FloatRect{hueX - 2, hueBar.y - 2, 4, hueBar.height + 4}, Color(0, 0, 0, 255), 1.0f);

    // Interação com hue bar
    bool hueHovered = IsPointInRect(m_mousePos, hueBar);
    static std::string activeHueBar;

    if (hueHovered && Input::IsMousePressed(MouseButton::LEFT))
    {
        activeHueBar = id;
    }

    if (activeHueBar == id && Input::IsMouseDown(MouseButton::LEFT))
    {
        float hx = (m_mousePos.x - hueBar.x) / hueBar.width;
        if (hx < 0)
            hx = 0;
        if (hx > 1)
            hx = 1;

        hsv.x = hx * 360.0f; // hue

        // Convert back to RGB
        float r, g, b;
        HSVtoRGB(hsv.x, hsv.y, hsv.z, r, g, b);
        color->r = (uint8_t)(r * 255);
        color->g = (uint8_t)(g * 255);
        color->b = (uint8_t)(b * 255);
        changed = true;
    }

    if (!Input::IsMouseDown(MouseButton::LEFT))
    {
        activeHueBar.clear();
    }

    y += hueBarH + spacing * 2;

    // ========================================
    // Preview Box
    // ========================================
    FloatRect preview = MakeContentRect(x, y, boxSize, 40);
    DrawRectFilled(preview, *color);
    DrawRectOutline(preview, m_theme.colorPickerBorder, 2.0f);

    // Sync cache
    hsvCache[id] = hsv;

    return changed;
}

bool GUI::TextInput(const char *label, char *buffer, size_t bufferSize,
                    float x, float y, float w, float h)
{
    if (!buffer || bufferSize == 0)
        return false;

    if (h <= 0)
        h = m_theme.textInputHeight;

    GUI::Control* control =  AddControl();
    if (!control) 
        return false;
    FloatRect rect = MakeContentRect(x, y, w, h);

 
    bool hovered = IsPointInRect(m_mousePos, rect);
  
 
 
    if (Input::IsMousePressed(MouseButton::LEFT) && hovered )
    {        
        control->active = !control->active;
    }
  
 
 
  

 
    

    // Inicializa cursor no final do texto
    size_t textLen = strlen(buffer);
    if (control->itag > (int)textLen)
        control->itag = (int)textLen;
    if (control->itag < 0)
        control->itag = 0;

    // Input de texto quando ativo
    if (control->active )
    {
        int ch = Input::GetCharPressed();
        if (ch != 0)
        {

            if (ch >= 32 && ch < 127)
            {   if (textLen < bufferSize - 1)
                {
                    for (size_t i = textLen; i > (size_t)control->itag; --i)
                    {
                        buffer[i] = buffer[i - 1];
                    }
                    buffer[control->itag] = (char)ch;
                    buffer[textLen + 1] = '\0';
                    control->itag++;
                    control->changed = true;
                }
            }
        }

        // Backspace
        if (Input::IsKeyPressed(KeyCode::KEY_BACKSPACE))
        {
            if (control->itag > 0 && textLen > 0)
            {
                for (size_t i = control->itag - 1; i < textLen; ++i)
                {
                    buffer[i] = buffer[i + 1];
                }
                control->itag--;
                control->changed = true;
            }
        }

        // Delete
        if (Input::IsKeyPressed(KeyCode::KEY_DELETE))
        {
            if (control->itag < (int)textLen)
            {
                for (size_t i = control->itag; i < textLen; ++i)
                {
                    buffer[i] = buffer[i + 1];
                }
                control->changed = true;
            }
        }

        // Left arrow
        if (Input::IsKeyPressed(KeyCode::KEY_LEFT))
        {
            if (control->itag > 0)
                control->itag--;
        }

        // Right arrow
        if (Input::IsKeyPressed(KeyCode::KEY_RIGHT))
        {
            if (control->itag < (int)textLen)
                control->itag++;
        }

        // Home
        if (Input::IsKeyPressed(KeyCode::KEY_HOME))
        {
            control->itag = 0;
        }

        // End
        if (Input::IsKeyPressed(KeyCode::KEY_END))
        {
            control->itag = (int)textLen;
        }

        // Enter para desativar (opcional)
        if (Input::IsKeyPressed(KeyCode::KEY_ENTER) || Input::IsKeyPressed(KeyCode::KEY_KP_ENTER))
        {
            m_activeID.clear();
            control->active  = false;
        }
    }

    // Background
    Color bgColor = m_theme.textInputBg;
    if (hovered)
        bgColor = Color(40, 40, 50, 255);  

    DrawRectFilled(rect, bgColor);

    // Border
    Color borderColor = control->active  ? m_theme.buttonActive : m_theme.textInputBorder;
    DrawRectOutline(rect, borderColor, control->active  ? 2.0f : 1.0f);

    // Text
    float textX = rect.x + 6;
    float textY = rect.y + (rect.height - GetTextHeight(buffer)) * 0.5f;
    DrawText(buffer, textX, textY, m_theme.textInputText);
 
    if (control->active )
    {

        control->ftag += Device::Instance().GetFrameTime();  
        if (control->ftag > 1.0f)
            control->ftag = 0.0f;

        bool showCursor = (control->ftag < 0.5f);

        if (showCursor)
        {
            // Calcula posição X do cursor
            std::string textBeforeCursor(buffer, control->itag);
            float cursorX = textX + GetTextWidth(textBeforeCursor.c_str());

            // Linha do cursor
            DrawRectFilled(FloatRect{cursorX, rect.y + 4, 2, rect.height - 8}, m_theme.textInputCursor);
        }
       
    }

    return control->changed;
}

// ========================================
// ProgressBar
// ========================================
void GUI::ProgressBar(float progress, float x, float y, float w, float h,
                      Orientation orient, const char *overlay)
{
    // Clamp progress 0-1
    if (progress < 0.0f)
        progress = 0.0f;
    if (progress > 1.0f)
        progress = 1.0f;

    FloatRect bgRect = MakeContentRect(x, y, w, h);

    // Background
    DrawRectFilled(bgRect, m_theme.progressBarBg);
    DrawRectOutline(bgRect, m_theme.windowBorder);

    // Fill
    if (progress > 0.0f)
    {
        FloatRect fillRect = bgRect;

        if (orient == Orientation::Horizontal)
        {
            fillRect.width = bgRect.width * progress;
        }
        else
        {
            // Vertical cresce de baixo para cima
            float fillHeight = bgRect.height * progress;
            fillRect.y = bgRect.y + bgRect.height - fillHeight;
            fillRect.height = fillHeight;
        }

        DrawRectFilled(fillRect, m_theme.progressBarFill);
    }

    // Overlay text
    if (overlay)
    {
        float textW = GetTextWidth(overlay);
        float textH = GetTextHeight(overlay);
        float textX = bgRect.x + (bgRect.width - textW) * 0.5f;
        float textY = bgRect.y + (bgRect.height - textH) * 0.5f;
        DrawText(overlay, textX, textY, m_theme.labelText);
    }
}

bool GUI::ListBox(const char *label, int *selectedIndex, const char **items, int itemCount,
                  float x, float y, float w, float h, int visibleItems)
{
    if (!selectedIndex || !items || itemCount <= 0)
        return false;

    std::string id = GenerateID(label);

    // Label
    if (label && label[0] != '\0')
    {
        Label(label, x, y);
        y += GetTextHeight(label) + m_theme.itemSpacing;
    }

    FloatRect boxRect = MakeContentRect(x, y, w, h);

    // Background
    DrawRectFilled(boxRect, m_theme.listBoxBg);
    DrawRectOutline(boxRect, m_theme.listBoxBorder, 2.0f);

    // Calcula item height
    float itemH = h / visibleItems;
    if (itemCount < visibleItems)
    {
        itemH = h / itemCount;
    }

    // Scroll offset
    static std::unordered_map<std::string, float> scrollOffsets;
    float &scrollOffset = scrollOffsets[id];

    // Limita scroll
    float maxScroll = (itemCount - visibleItems) * itemH;
    if (maxScroll < 0)
        maxScroll = 0;
    if (scrollOffset < 0)
        scrollOffset = 0;
    if (scrollOffset > maxScroll)
        scrollOffset = maxScroll;

    bool boxHovered = IsPointInRect(m_mousePos, boxRect);
    bool changed = false;
    bool isScrooling = false;

    // Flag para bloquear items durante drag
    static std::string draggedScrollbar;
    static float dragStartY = 0;
    static float dragStartOffset = 0;

    // Draw items
    int firstVisible = (int)(scrollOffset / itemH);
    int lastVisible = firstVisible + visibleItems;
    if (lastVisible > itemCount)
        lastVisible = itemCount;

    // Scrollbar
    if (itemCount > visibleItems)
    {
        float scrollbarX = boxRect.x + boxRect.width - m_theme.scrollbarSize;
        float scrollbarH = boxRect.height;

        // Track
        FloatRect trackRect{scrollbarX, boxRect.y, m_theme.scrollbarSize, scrollbarH};
        DrawRectFilled(trackRect, m_theme.sliderBg);

        // Handle
        float handleHeight = (visibleItems / (float)itemCount) * scrollbarH;
        if (handleHeight < 20)
            handleHeight = 20;

        float handleY = boxRect.y + (scrollOffset / maxScroll) * (scrollbarH - handleHeight);
        FloatRect handleRect{scrollbarX, handleY, m_theme.scrollbarSize, handleHeight};

        bool handleHovered = IsPointInRect(m_mousePos, handleRect);
        Color handleColor = handleHovered ? m_theme.sliderHandleHover : m_theme.sliderHandle;
        DrawRectFilled(handleRect, handleColor);

        // Drag scrollbar
        if (handleHovered && Input::IsMousePressed(MouseButton::LEFT))
        {
            draggedScrollbar = id;
            dragStartY = m_mousePos.y;
            dragStartOffset = scrollOffset;
        }

        if (draggedScrollbar == id && Input::IsMouseDown(MouseButton::LEFT))
        {
            float deltaY = m_mousePos.y - dragStartY;
            float deltaScroll = (deltaY / (scrollbarH - handleHeight)) * maxScroll;
            scrollOffset = dragStartOffset + deltaScroll;
            isScrooling = true;
        }

        if (!Input::IsMouseDown(MouseButton::LEFT))
        {
            draggedScrollbar.clear();
        }
    }

    for (int i = firstVisible; i < lastVisible; ++i)
    {
        float itemY = boxRect.y + (i - firstVisible) * itemH;

        // Largura do item = largura da box - scrollbar (se existir)
        float itemWidth = boxRect.width;
        if (itemCount > visibleItems)
        {
            itemWidth -= m_theme.scrollbarSize;
        }

        FloatRect itemRect{boxRect.x, itemY, itemWidth, itemH};

        bool itemHovered = IsPointInRect(m_mousePos, itemRect) && boxHovered && !isScrooling;
        bool isSelected = (i == *selectedIndex);

        Color itemBg = m_theme.listBoxBg;
        if (isSelected)
            itemBg = m_theme.listBoxItemSelected;
        else if (itemHovered)
            itemBg = m_theme.listBoxItemHover;

        DrawRectFilled(itemRect, itemBg);

        // Text
        float textX = itemRect.x + 8;
        float textY = itemRect.y + (itemRect.height - GetTextHeight(items[i])) * 0.5f;
        DrawText(items[i], textX, textY, m_theme.buttonText);

        // Click to select
        if (itemHovered && Input::IsMousePressed(MouseButton::LEFT))
        {
            *selectedIndex = i;
            changed = true;
        }
    }

    return changed;
}

// ========================================
// Dropdown/Combo
// ========================================
bool GUI::Dropdown(const char *label, int *selectedIndex, const char **items, int itemCount,
                   float x, float y, float w, float h)
{
    if (!m_currentWindow || !selectedIndex || !items || itemCount <= 0)
        return false;

    std::string id = GenerateID(label);
    FloatRect rect = MakeContentRect(x, y, w, h);

    bool isOpen = (m_activeID == id);
    bool buttonHovered = IsPointInRect(m_mousePos, rect);

    // Toggle no botão principal
    if (buttonHovered && Input::IsMousePressed(MouseButton::LEFT))
    {

        m_activeID = id;
        isOpen = true;

        BringCurrentWindowToFront();
    }

    Color bgColor = m_theme.dropdownBg;
    if (buttonHovered)
        bgColor = m_theme.dropdownHover; // ou Lerp(inputBg, buttonHover, 0.4f)
    if (isOpen)
        bgColor = m_theme.buttonActive;

    DrawRectFilled(rect, bgColor);
    DrawRectOutline(rect, m_theme.dropdownBorder); // ou windowBorder

    int idx = (*selectedIndex >= 0 && *selectedIndex < itemCount) ? *selectedIndex : 0;
    const char *currentText = items[idx] ? items[idx] : "Select...";
    float textY = rect.y + (rect.height - GetTextHeight(currentText)) * 0.5f;
    DrawText(currentText, rect.x + 8.0f, textY, m_theme.buttonText);

    const char *arrow = isOpen ? "^" : "v";
    DrawText(arrow, rect.x + rect.width - GetTextWidth(arrow) - 8.0f, textY, m_theme.buttonText);

    bool changed = false;

    if (isOpen)
    {
        const float itemH = std::max(h, m_theme.dropdownHeight);
        FloatRect listRect(rect.x, rect.y + rect.height + 2.0f, rect.width, itemH * itemCount);

        DrawRectFilled(listRect, m_theme.windowBg);
        DrawRectOutline(listRect, m_theme.windowBorder, 1.0f);

        for (int i = 0; i < itemCount; ++i)
        {
            FloatRect itemRect(listRect.x, listRect.y + i * itemH, listRect.width, itemH);
            bool itemHovered = IsPointInRect(m_mousePos, itemRect);

            Color itemBg = m_theme.dropdownBg;
            if (i == idx)
                itemBg = m_theme.buttonActive;
            else if (itemHovered)
                itemBg = m_theme.dropdownItemHover;

            DrawRectFilled(itemRect, itemBg);
            DrawRectOutline(itemRect, m_theme.dropdownBorder);

            const char *txt = items[i] ? items[i] : "";
            float itY = itemRect.y + (itemRect.height - GetTextHeight(txt)) * 0.5f;
            DrawText(txt, itemRect.x + 8.0f, itY, m_theme.buttonText);

            if (itemHovered)
            {
                if (i != *selectedIndex)
                {
                    *selectedIndex = i;
                    changed = true;
                }

                //  isOpen = false;
            }
        }

        if (Input::IsMousePressed(MouseButton::LEFT) &&
            !IsPointInRect(m_mousePos, rect) && !IsPointInRect(m_mousePos, listRect))
        {
            m_activeID.clear();
            isOpen = false;
        }
    }

    return changed;
}

// ========================================
// RadioButton
// ========================================
bool GUI::RadioButton(const char *label, int *selected, int value, float x, float y, float size)
{
    if (!selected)
        return false;

    if (size <= 0)
        size = m_theme.radioSize;

    std::string id = GenerateID(label);
    FloatRect circleRect = MakeContentRect(x, y, size, size);

    WidgetState state = GetWidgetState(circleRect, id);
    bool isSelected = (*selected == value);

    // Background circle
    Color bgColor = m_theme.radioBg;
    if (state == WidgetState::Hovered)
        bgColor = m_theme.buttonHover;

    DrawRectFilled(circleRect, bgColor); // Usa rect, mas podes fazer círculo se quiseres
    DrawRectOutline(circleRect, m_theme.radioBorder);

    // Inner dot if selected
    if (isSelected)
    {
        FloatRect dotRect = circleRect;
        float margin = size * 0.25f;
        dotRect.x += margin;
        dotRect.y += margin;
        dotRect.width -= margin * 2;
        dotRect.height -= margin * 2;
        DrawRectFilled(dotRect, m_theme.radioCheck);
    }

    // Label
    float labelX = circleRect.x + circleRect.width + 8;
    float labelY = circleRect.y + (circleRect.height - GetTextHeight(label)) * 0.5f;
    DrawText(label, labelX, labelY, m_theme.labelText);

    // Click
    if (state == WidgetState::Hovered && Input::IsMousePressed(MouseButton::LEFT))
    {
        *selected = value;
        return true;
    }

    return false;
}
