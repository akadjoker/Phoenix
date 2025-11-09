

#include "Core.hpp"
#include "Input.hpp"
 

int screenWidth = 1024;
int screenHeight = 768;


static bool   s_showWindowA = true;
static bool   s_showWindowB = true;

static bool   s_chkA = true;
static bool   s_chkB = false;

static float  s_sliderF_A = 0.25f;
static float  s_sliderF_B = 0.75f;

static int    s_sliderI_A = 25;
static int    s_sliderI_B = 75;

static float  s_vSliderF  = 0.5f;
static int    s_vSliderI  = 50;

static int    s_clicksA   = 0;
static int    s_clicksB   = 0;

static bool cbTest1 = false;
static bool cbTest2 = true;

void DrawDemoUI(GUI& gui)
{
    // ============================
    // JANELA A
    // ============================
    if (gui.BeginWindow("Janela A", 60, 60, 420, 230, &s_showWindowA))
    {
        // separador no topo
        gui.Separator(0, 0, 404);

        // botão + contador
        if (gui.Button("Click A", 0, 10, 120, gui.GetTheme().buttonHeight))
            ++s_clicksA;
        gui.Text(130, 14, "Clicks: %d", s_clicksA);

        // checkbox + label
        gui.Checkbox("Ativar modo A", &s_chkA, 0, 50, gui.GetTheme().checkboxSize);

        // sliders horizontais
        gui.SliderFloat("Forca", &s_sliderF_A, 0.0f, 1.0f, 0, 90, 404, gui.GetTheme().sliderHeight);
        gui.SliderInt  ("Velocidade", &s_sliderI_A, 0, 100, 0, 120, 404, gui.GetTheme().sliderHeight);

        // um pequeno “status”
        gui.Label("Estado:", 0, 160);
        gui.Text(70, 160, "%s | Forca=%.2f | Vel=%d",
                 s_chkA ? "ON" : "OFF", s_sliderF_A, s_sliderI_A);

        
gui.Checkbox("Modo Turbo", &cbTest1, 0, 200, gui.GetTheme().checkboxSize);
gui.Checkbox("Ativar Física", &cbTest2, 0, 230, gui.GetTheme().checkboxSize);

gui.Text(0, 260, "Turbo: %s | Física: %s", cbTest1 ? "ON" : "OFF", cbTest2 ? "ON" : "OFF");

    }
    gui.EndWindow();

    // ============================
    // JANELA B
    // ============================
    if (gui.BeginWindow("Janela B", 520, 60, 420, 380, &s_showWindowB))
    {
        gui.Separator(0, 0, 404);

        // botão + contador
        if (gui.Button("Click B", 0, 10, 120, gui.GetTheme().buttonHeight))
            ++s_clicksB;
        gui.Text(130, 14, "Clicks: %d", s_clicksB);

        // checkbox
        gui.Checkbox("Ativar modo B", &s_chkB, 0, 50, gui.GetTheme().checkboxSize);

        // sliders horizontais
        gui.SliderFloat("Gravidade", &s_sliderF_B, 0.0f, 1.0f, 0, 90, 404, gui.GetTheme().sliderHeight);
        gui.SliderInt  ("Iteracoes", &s_sliderI_B, 0, 100, 0, 120, 404, gui.GetTheme().sliderHeight);

        // sliders verticais lado a lado
        // caixa dos sliders verticais (x=0..180) e legendas
        gui.Label("Vertical Float",  40, 160);
        gui.Label("Vertical Int",   250, 160);

        gui.SliderFloatVertical("vF", &s_vSliderF, 0.0f, 1.0f,  40, 190, 40, 100);
        gui.SliderIntVertical  ("vI", &s_vSliderI, 0,   100,   250, 190, 40, 100);

        // um pequeno “status”
        gui.Text(0, 290-24, "vF=%.2f | vI=%d", s_vSliderF, s_vSliderI);
    }
    gui.EndWindow();
}

int main()
{

    Device &device = Device::Instance();

    if (!device.Create(screenWidth, screenHeight, "Game", true, 1))
    {
        return 1;
    }
    Driver &driver = Driver::Instance();
    driver.SetClearDepth(1.0f);
    driver.SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    RenderBatch batch;
    batch.Init();

    Font font;
    font.SetBatch(&batch);
    font.LoadDefaultFont();

    float lastX{0};
    float lastY{0};

    CameraFree camera(45.0f, (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
    camera.setPosition(0.0f, 0.5f, 10.0f);

    bool firstMouse{true};
    float mouseSensitivity{0.8f};

    GUI gui;
    gui.Init(&batch, &font);
    Input::Init();
    float val1, val2, val3 = 0.0f;

    while (device.IsRunning())
    {

        float dt = device.GetFrameTime();
        const float SPEED = 12.0f * dt;
        
        Input::Update();
 

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                device.SetShouldClose(true);
                break;
            case SDL_WINDOWEVENT:
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    screenWidth = event.window.data1;
                    screenHeight = event.window.data2;
                    driver.SetViewPort(0, 0, screenWidth, screenHeight);
                    camera.setAspectRatio((float)screenWidth / (float)screenHeight);
                }
            }
            case SDL_MOUSEBUTTONDOWN:
                Input::OnMouseDown(event.button);
                break;
            case SDL_MOUSEBUTTONUP:
                Input::OnMouseUp(event.button);
                break;
            case SDL_MOUSEMOTION:
                Input::OnMouseMove(event.motion);
                break;
            case SDL_MOUSEWHEEL:
                Input::OnMouseWheel(event.wheel);
                break;
            case SDL_KEYDOWN:
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    device.SetShouldClose(true);
                    break;
                }
                Input::OnKeyDown(event.key);
                break;
            }
            case SDL_KEYUP:
                Input::OnKeyUp(event.key);
                break;
            case SDL_TEXTINPUT:     
                {
                    Input::OnTextInput(event.text);
                    break;
                }
            }
        }

        int xposIn, yposIn;
        u32 IsMouseDown = SDL_GetMouseState(&xposIn, &yposIn);

        if (IsMouseDown & SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            float xpos = static_cast<float>(xposIn);
            float ypos = static_cast<float>(yposIn);

            if (firstMouse)
            {
                lastX = xpos;
                lastY = ypos;
                firstMouse = false;
            }

            float xoffset = xpos - lastX;
            float yoffset = ypos - lastY;

            lastX = xpos;
            lastY = ypos;
            // fpsCamera.MouseLook(xoffset, yoffset);

            camera.rotate(yoffset * mouseSensitivity, xoffset * mouseSensitivity);
        }
        else
        {
            firstMouse = true;
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W])
            camera.move(SPEED);
        if (state[SDL_SCANCODE_S])
            camera.move(-SPEED);
        if (state[SDL_SCANCODE_A])
            camera.strafe(-SPEED);
        if (state[SDL_SCANCODE_D])
            camera.strafe(SPEED);

        camera.update(1.0f);
        const Mat4 &view = camera.getViewMatrix();
        const Mat4 &proj = camera.getProjectionMatrix();

        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

        batch.SetMatrix(ortho);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        gui.BeginFrame();

 
        DrawDemoUI(gui);

static float volume = 50.0f;
static float brightness = 50.0f;
static bool fullscreen = true;

        // Settings window com labels à esquerda e controlos à direita
gui.BeginWindow("Settings", 100, 100, 400, 300);
 

// // ColorPicker
// Color myColor = Color(255, 0, 0, 255);
// if (gui.ColorPicker("Pick Color", &myColor, 20, 20)) {
//     // cor mudou!
// }

// static Color myColor = Color(255, 100, 50, 255);
// if (gui.ColorPicker("Advanced", &myColor, 20, 100, 200)) 
// {
  
// }

static char textBuffer[256] = "Hello World";

if (gui.TextInput("Name", textBuffer, sizeof(textBuffer), 20, 50, 200)) {
    // texto mudou!
    LogInfo("Text: %s", textBuffer);
}
 
// const char *options[] = {"Option 1", "Option 2", "Option 3"};
// static int selected = 0;
// if (gui.Dropdown("Choose", &selected, options, 3, 20, 20, 200, 24)) 
// {
//    LogInfo(" slecte %s %d",options[selected],selected);
// }

//  const char *items[] = {"Item 1", "Item 2", "Item 3", "Item 4", "Item 5", 
//                        "Item 6", "Item 7", "Item 8", "Item 9", "Item 10"};
// static int selected = 0;

// if (gui.ListBox("My List", &selected, items, 10, 20, 50, 200, 150, 5)) {
    
//     LogInfo(" slecte %s %d",items[selected],selected);
// }

// // Botões centrados no fundo
// float btnY = gui.GetWindowContentHeight() - 30;
// float btnWidth = 100;
// float spacing = 10;
// float totalWidth = btnWidth * 2 + spacing;
// float startX = gui.AlignCenter(totalWidth);

// gui.Button("Apply", startX, btnY, btnWidth, 24);
// gui.Button(items[selected], startX + btnWidth + spacing, btnY, btnWidth, 24);

gui.EndWindow();




        gui.EndFrame();

        batch.SetColor(255, 255, 255);

        font.Print(10, 10, "Fps :%d", device.GetFPS());


 

        batch.Render();

        device.Flip();
 

    }

    font.Release();
    batch.Release();

    MeshManager::Instance().UnloadAll();
    ShaderManager::Instance().UnloadAll();
    TextureManager::Instance().UnloadAll();
    device.Close();

    return 0;
}
