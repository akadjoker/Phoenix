
#include "pch.h"
#include "Device.hpp"
#include "Utils.hpp"
#include "Driver.hpp"
#include "glad/glad.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

extern "C" const char *__lsan_default_suppressions()
{
    return "leak:libSDL2\n"
           "leak:SDL_DBus\n";
}



double GetTime() { return static_cast<double>(SDL_GetTicks()) / 1000; }

//*************************************************************************************************
// Device
//*************************************************************************************************

Device &Device::Instance()
{
    static Device instance;
    return instance;
}
Device *Device::InstancePtr() { return &Instance(); }

Device::Device() : m_width(0), m_height(0)
{
    LogInfo("[DEVICE] Initialized.");

    m_shouldclose = false;
    m_window = NULL;
    m_context = NULL;
    m_current = 0;
    m_previous = 0;
    m_update = 0;
    m_draw = 0;
    m_frame = 0;
    m_target = 0;
    m_ready = false;
    m_is_resize = false;
}

Device::~Device()
{
    LogInfo("[DEVICE] Destroyed.");
    Close();
}

bool Device::Create(int width, int height, const char *title, bool vzync, u16 monitorIndex)
{

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        LogError("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return false;
    }

    m_current = 0;
    m_previous = 0;
    m_update = 0;
    m_draw = 0;
    m_frame = 0;

    SetTargetFPS(vzync ? 60 : 1000);
    m_closekey = 256;
    m_width = width;
    m_height = height;

    m_current = GetTime();
    m_draw = m_current - m_previous;
    m_previous = m_current;
    m_frame = m_update + m_draw;

    // // Atributos de contexto antes de criar a janela
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    const int majorVersion = 3;
    const int minorVersion = 2; // pede 3.2 para ter debug core (altera para 3.1 para nao ter)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);

    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    

    // Formato de framebuffer
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    // MSAA
    int sampleCount = 1; // mete >0 para ativar
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, sampleCount);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, sampleCount > 0 ? 1 : 0);



    int numDisplays = SDL_GetNumVideoDisplays();
    LogInfo("Num Displays: %d", numDisplays);
     for (int i = 0; i < numDisplays; i++) 
     {
        const char* displayName = SDL_GetDisplayName(i);
        LogInfo("Display: %d - %s", i, displayName);
    }

    if (monitorIndex > numDisplays)
    {
        monitorIndex = 0;
    }
 

    // Criação
    m_window = SDL_CreateWindow(
        title,
          SDL_WINDOWPOS_CENTERED_DISPLAY(monitorIndex), 
        SDL_WINDOWPOS_CENTERED_DISPLAY(monitorIndex), 
         width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!m_window)
    {
        LogError("Window! %s", SDL_GetError());
        return false;
    }

    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context)
    {
        LogError("Context! %s", SDL_GetError());
        return false;
    }

    // VSync
    SDL_GL_SetSwapInterval(vzync ? 1 : 0);

    // glad (GLES)
    if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        LogError("Failed to load GLES with glad");
        return false;
    }

    // Verificar versão carregada (glad 0.1.x)
    if (!(GLAD_GL_ES_VERSION_3_1))
    { // ou 3_2 se pediste 3.2
        LogError("OpenGL ES 3.1 não suportado pelo driver");
        return false;
    }

    LogInfo("[DEVICE] Vendor  : %s", (const char *)glGetString(GL_VENDOR));
    LogInfo("[DEVICE] Renderer: %s", (const char *)glGetString(GL_RENDERER));
    LogInfo("[DEVICE] Version : %s", (const char *)glGetString(GL_VERSION));
    LogInfo("[DEVICE] GLSL ES : %s",
            (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));

    GLfloat maxAniso = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);

    LogInfo("[DEVICE] Anisotropy: %f", maxAniso);

    //    glEnable(GL_MULTISAMPLE);

    Driver::Instance();

    m_ready = true;

    return true;
}

void Device::Wait(float ms) { SDL_Delay(ms); }

int Device::GetFPS(void)
{
#define FPS_CAPTURE_FRAMES_COUNT 30   // 30 captures
#define FPS_AVERAGE_TIME_SECONDS 0.5f // 500 millisecondes
#define FPS_STEP (FPS_AVERAGE_TIME_SECONDS / FPS_CAPTURE_FRAMES_COUNT)

    static int index = 0;
    static float history[FPS_CAPTURE_FRAMES_COUNT] = {0};
    static float average = 0, last = 0;
    float fpsFrame = GetFrameTime();

    if (fpsFrame == 0)
        return 0;

    if ((GetTime() - last) > FPS_STEP)
    {
        last = (float)GetTime();
        index = (index + 1) % FPS_CAPTURE_FRAMES_COUNT;
        average -= history[index];
        history[index] = fpsFrame / FPS_CAPTURE_FRAMES_COUNT;
        average += history[index];
    }

    return (int)roundf(1.0f / average);
}

void Device::SetTargetFPS(int fps)
{
    if (fps < 1)
        m_target = 0.0;
    else
        m_target = 1.0 / (double)fps;
}

float Device::GetFrameTime(void) { return (float)m_frame; }

double Device::GetTime(void) { return (double)SDL_GetTicks() / 1000.0; }

u32 Device::GetTicks(void) { return SDL_GetTicks(); }

bool Device::Run()
{
    if (!m_ready)
        return false;

    m_current = GetTime(); // Number of elapsed seconds since InitTimer()
    m_update = m_current - m_previous;
    m_previous = m_current;

    SDL_Event event;

    while (SDL_PollEvent(&event) != 0)
    {

        switch (event.type)
        {
        case SDL_QUIT:
        {
            m_shouldclose = true;
            break;
        }
        case SDL_WINDOWEVENT:
        {
            switch (event.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
            {
                m_width = event.window.data1;
                m_height = event.window.data2;
                // glViewport(0, 0, m_width, m_height);
                m_is_resize = true;

                break;
            }
            }
            break;
        }
        case SDL_KEYDOWN:
        {

            // LogWarning("[DEVICE] Key %d
            // %d",Keyboard::toKey(event.key.keysym.scancode),m_closekey);
            // if (Keyboard::toKey(event.key.keysym.scancode) == m_closekey)
            // {
            //     m_shouldclose = true;
            //     break;
            // }

            // Keyboard::setKeyState(event.key.keysym.scancode, true);

            break;
        }

        case SDL_KEYUP:
        {

            // Keyboard::setKeyState(event.key.keysym.scancode, false);
            break;
        }
        break;
        case SDL_MOUSEBUTTONDOWN:
        {

            int btn = event.button.button - 1;
            if (btn == 2)
                btn = 1;
            else if (btn == 1)
                btn = 2;
            // Mouse::setMouseButton(btn, true);
        }
        break;
        case SDL_MOUSEBUTTONUP:
        {
            int btn = event.button.button - 1;
            if (btn == 2)
                btn = 1;
            else if (btn == 1)
                btn = 2;
            // Mouse::setMouseButton(btn, false);

            break;
        }
        case SDL_MOUSEMOTION:
        {
            // Mouse::setMousePosition(event.motion.x, event.motion.y,
            //                        event.motion.xrel, event.motion.yrel);
            break;
        }

        case SDL_MOUSEWHEEL:
        {
            //  Mouse::setMouseWheel(event.wheel.x, event.wheel.y);
            break;
        }
        }
    }

    return !m_shouldclose;
}



int Device::PollEvents( SDL_Event *event)
{
    if (!m_ready)
        return false;
    m_current = GetTime(); // Number of elapsed seconds since InitTimer()
    m_update = m_current - m_previous;
    m_previous = m_current;
    return SDL_PollEvent(event);

}


void Device::Close()
{
    if (!m_ready)
        return;

    m_ready = false;

    SDL_GL_DeleteContext(m_context);
    SDL_DestroyWindow(m_window);

    m_window = NULL;
    LogInfo("[DEVICE] closed!");
    SDL_Quit();
}

void Device::Flip()
{

    SDL_GL_SwapWindow(m_window);

    m_current = GetTime();
    m_draw = m_current - m_previous;
    m_previous = m_current;
    m_frame = m_update + m_draw;

    // Wait for some milliseconds...
    if (m_frame < m_target)
    {
        Wait((float)(m_target - m_frame) * 1000.0f);

        m_current = GetTime();
        double waitTime = m_current - m_previous;
        m_previous = m_current;

        m_frame += waitTime; // Total frame time: update + draw + wait
    }

    m_is_resize = false;
}
