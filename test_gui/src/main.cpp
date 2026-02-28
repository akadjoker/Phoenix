

#include "Core.hpp"

int screenWidth = 1024;
int screenHeight = 768;

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

    GUI gui;
    gui.Init(&batch, &font);

    while (device.Run())
    {

        float dt = device.GetFrameTime();

        if (device.IsResize())
        {
            driver.SetViewPort(0, 0, device.GetWidth(), device.GetHeight());
        }

        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

        driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);

        batch.SetMatrix(ortho);
        driver.SetDepthTest(false);
        driver.SetBlendEnable(true);
        driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

        batch.SetColor(255, 255, 255);
        //
        font.Print(10, 10, "Fps :%d", device.GetFPS());

        gui.BeginFrame();

        // Stats window
        gui.BeginWindow("Stats", screenWidth - 260, 10, 260, 170);
        gui.Text(10, 10, "FPS %d Delta: %.2f ms", device.GetFPS(), dt);

        int drawCalls = driver.GetCountDrawCall();
        int meshs = driver.GetCountMesh();
        int meshBuffers = driver.GetCountMeshBuffer();
        int vertices = driver.GetCountVertex();
        int triangles = driver.GetCountTriangle();
        int textures = driver.GetCountTextures();
        int shaders = driver.GetCountPrograms();

        gui.Text(10, 30, "Draw Calls: %d", drawCalls);
        gui.Text(10, 50, "Mesh: %d    Buffers: %d", meshs, meshBuffers);

        gui.Text(10, 70, "Vertices: %d Triangles: %d", vertices, triangles);
        gui.Text(10, 90, "Textures: %d Shaders: %d", textures, shaders);
 

        gui.EndWindow();

        static bool demoOpen = true;
        static bool check1 = false;
        static float fvalue = 0.5f;
        static int ivalue = 10;
        static float fvalueV = 0.25f;
        static int ivalueV = 3;
        static char textBuf[128] = "Teste de input";

        if (demoOpen)
        {
            gui.BeginWindow("Demo Widgets", 40, 40, 360, 450, &demoOpen);

            // Label
            gui.Label("Label normal", 10, 10);

            // Checkbox
            if (gui.Checkbox("Ativar opcao", &check1, 10, 40, 20))
            {
                printf("Checkbox mudou: %d\n", check1);
            }

            gui.Separator(10, 70, 300);

            // Slider Float
            gui.Label("Slider Float:", 10, 90);
            if (gui.SliderFloat("", &fvalue, 0.0f, 1.0f, 120, 90, 200, 20))
            {
                printf("Slider float: %.3f\n", fvalue);
            }

            // Slider Int
            gui.Label("Slider Int:", 10, 130);
            gui.SliderInt("", &ivalue, 0, 20, 120, 130, 200, 20);

            gui.Separator(10, 170, 300);

            // Vertical sliders
            gui.Label("Float", 10, 190);
            gui.SliderFloatVertical("", &fvalueV, 0.0f, 1.0f, 10, 220, 25, 120);

            gui.Label("Int", 100, 190);
            gui.SliderIntVertical("", &ivalueV, 0, 80, 60, 220, 25, 120);

            gui.Separator(10, 370, 300);

            // TextInput
            gui.Label("Input texto:", 10, 390);
            gui.TextInput("", textBuf, sizeof(textBuf), 120, 385, 200, 24);

            gui.EndWindow();
        }

 

            static bool uiDemoOpen = true;

            static Color demoColor(255, 128, 64, 255);

            static float demoProgress = 0.0f;

            static const char *listItems[] = {
                "Item 1",
                "Item 2",
                "Item 3",
                "Item 4",
                "Item 5",
            };
            static int listSelected = 0;

            static const char *comboItems[] = {
                "Opção A",
                "Opção B",
                "Opção C"};
            static int comboSelected = 0;

            static int radioSelected = 0; // 0,1,2 para 3 radio buttons

            // =============================
            //   Janela de teste de widgets
            // =============================
       if (uiDemoOpen)
        {
           
                gui.BeginWindow("UI Demo", 20, 200, 420, 460, &uiDemoOpen);

                // 1) ColorPicker
                gui.Label("Color Picker", 10, 10);
                gui.ColorPicker("", &demoColor, 10, 30, 150);



                // 2) ProgressBar (horizontal)
                gui.Label("Progresso", 10, 130);
                demoProgress += dt * 0.25f; // só para animar (0–1)
                if (demoProgress > 1.0f)
                    demoProgress = 0.0f;
                gui.ProgressBar(demoProgress, 250, 150, 120, 20, Orientation::Horizontal, "A carregar...");

                // 3) ListBox
    
                gui.ListBox("",
                            &listSelected,
                            listItems,
                            (int)(sizeof(listItems) / sizeof(listItems[0])),
                            10, 280,
                            180, 120,
                            4);

                // 5) RadioButtons
                gui.Label("Modo", 210, 250);
                gui.RadioButton("Modo A", &radioSelected, 0, 210, 270, 16);
                gui.RadioButton("Modo B", &radioSelected, 1, 210, 295, 16);
                gui.RadioButton("Modo C", &radioSelected, 2, 210, 320, 16);
                // 4) Dropdown / Combo
                gui.Label("Dropdown", 210, 20);
                gui.Dropdown("",
                             &comboSelected,
                             comboItems,
                             (int)(sizeof(comboItems) / sizeof(comboItems[0])),
                             210, 50,
                             180, 24);


                gui.EndWindow();
        }
            
        static int g_themeIndex = 0; 
        // =============================
//   Theme Window
// =============================
gui.BeginWindow("Theme", 20, 10, 180, 140);

gui.Label("Escolher tema:", 10, 10);

 
if (gui.RadioButton("Dark", &g_themeIndex, 0, 10, 30, 14))
{
    gui.SetTheme(GUI::DarkTheme());
}

 
if (gui.RadioButton("Light", &g_themeIndex, 1, 10, 55, 14))
{
    gui.SetTheme(GUI::LightTheme());
}

 
if (gui.RadioButton("Blue", &g_themeIndex, 2, 10, 80, 14))
{
    gui.SetTheme(GUI::BlueTheme());
}

gui.EndWindow();

//driver.SetCulling(CullMode::None);

 
static bool  extraWindowOpen   = true;
static bool  toggleMusic       = true;
static bool  togglePostProcess = false;
if (extraWindowOpen)
{
    gui.BeginWindow("UI Extra Demo", 20, 220, 280, 180, &extraWindowOpen);

    gui.SeparatorText("Toggles", 10, 10, 260);
    gui.ToggleSwitch("Música",       &toggleMusic,       10, 30, 50, 18);
    gui.ToggleSwitch("Post-process", &togglePostProcess, 10, 60, 50, 18);

    gui.SeparatorText("Loading", 10, 95, 260);
    gui.Label("A carregar assets...", 10, 115);

    gui.Spinner(230, 118, 10.0f);

    gui.EndWindow();
}


            gui.EndFrame();

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
