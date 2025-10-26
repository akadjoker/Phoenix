

#include "Core.hpp"
 

int screenWidth = 1024;
int screenHeight = 768;




 
int main()
{

    Device &device = Device::Instance();

    if (!device.Create(screenWidth, screenHeight, "Game", true,1))
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


    while (device.IsRunning())
    {

        float dt = device.GetFrameTime();
        const float SPEED = 12.0f * dt;

        SDL_Event event;
        while (device.PollEvents(&event))
        {
            if (event.type == SDL_QUIT)
            {
                device.SetShouldClose(true);
                break;
            }else 
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                device.SetShouldClose(true);
                break;
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    screenWidth = event.window.data1;
                    screenHeight = event.window.data2;
    
                    driver.SetViewPort(0, 0, screenWidth, screenHeight);
                    camera.setAspectRatio((float)screenWidth / (float)screenHeight);
                    break;
                }
            break;
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
             //fpsCamera.MouseLook(xoffset, yoffset);

             camera.rotate(yoffset * mouseSensitivity,xoffset * mouseSensitivity);
      
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
        const Mat4 &mvp = proj * view;

        const Mat4 ortho = Mat4::Ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
        

       driver.Clear(CLEAR_COLOR | CLEAR_DEPTH);


       batch.SetMatrix(mvp);
       driver.SetDepthTest(true);
       driver.SetBlendEnable(false);

       batch.Grid(10, 1.0f, true);

       batch.Render();
 


       batch.SetMatrix(ortho);
       driver.SetDepthTest(false);
       driver.SetBlendEnable(true);
       driver.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

       batch.SetColor(255, 255, 255);
//
       font.Print(10,10,"Fps :%d",device.GetFPS());

         {
        FloatRect bounds(100, 200, 400, 100);
        
        // Centro
        font.PrintAligned("Centered Text", bounds, TextAlign::CENTER, TextVAlign::MIDDLE);
        
        // Direita
        font.PrintAligned("Right Aligned", bounds, TextAlign::RIGHT, TextVAlign::TOP);
        
        // Esquerda (padrão)
        font.PrintAligned("Left Aligned", bounds, TextAlign::LEFT, TextVAlign::BOTTOM);
    }

       {
        const char* longText = "This is a very long text that needs to be wrapped "
                              "to fit within a specific width. The word wrapping "
                              "system will automatically break lines at word boundaries.";
        
        // Wrapping simples
        font.PrintWrapped(longText, 100, 300, 400.0f);
        
        // Wrapping com estilo
        FloatRect textBox(100, 400, 400, 200);
        TextStyle style;
        style.fontSize = 16;
        style.lineSpacing = 5;
        style.align = TextAlign::JUSTIFY;
        style.valign = TextVAlign::TOP;
        
        font.PrintWrapped(longText, textBox, style);
    }
    {
        // Sombra
        font.PrintWithShadow("Text with Shadow", 100, 500,
                            Color::WHITE, Color::BLACK, Vec2(2, 2));
        
        // Contorno
        font.PrintWithOutline("Outlined Text", 100, 550,
                             Color::YELLOW, Color::BLACK, 2.0f);
        
        // Ambos (usando TextStyle)
        TextStyle fancyStyle;
        fancyStyle.color = Color::WHITE;
        fancyStyle.fontSize = 24;
        fancyStyle.enableShadow = true;
        fancyStyle.shadowColor.Set(0, 0, 0, 128);
        fancyStyle.shadowOffset = Vec2(3, 3);
        fancyStyle.enableOutline = true;
        fancyStyle.outlineColor = Color::BLACK;
        fancyStyle.outlineThickness = 1.5f;
        
        font.Print("Fancy Text!", 100, 600, fancyStyle);
    }

     {
        const char* text = "Measure this text";
        
        Vec2 size = font.GetTextSize(text);
        float width = font.GetTextWidth(text);
        float height = font.GetTextHeight(text);
        
        // Métricas detalhadas
        TextMetrics metrics = font.MeasureText(text, 300.0f);
        
        // Desenhar caixa ao redor do texto
        float x = 200, y = 700;
        font.Print(text, x, y);
        
        // Debug: desenhar bounding box
        batch.SetColor(255, 0, 0, 128);
        batch.Rectangle(x, y, size.x, size.y, false);
    }

      {
        const char* multiline = "Line 1\nLine 2\nLine 3";
        
        font.SetFontSize(18);
        font.SetLineSpacing(5); // Espaçamento extra entre linhas
        font.Print(multiline, 400, 100);
    }

    {
        // Definir região de clipping
        font.SetClip(500, 200, 200, 50);
        font.EnableClip(true);
        
        font.Print("This text will be clipped to the defined region", 500, 200);
        
        font.EnableClip(false);
    }

        // ========================================
    {
        // Título
        TextStyle titleStyle;
        titleStyle.fontSize = 32;
        titleStyle.color.Set(255, 215, 0, 255); // Gold
        titleStyle.enableOutline = true;
        titleStyle.outlineColor = Color::BLACK;
        titleStyle.outlineThickness = 2.0f;
        
        // Subtítulo
        TextStyle subtitleStyle;
        subtitleStyle.fontSize = 20;
        subtitleStyle.color.Set(200, 200, 200, 255);
        subtitleStyle.enableShadow = true;
        subtitleStyle.shadowOffset = Vec2(1, 1);
        
        // Corpo de texto
        TextStyle bodyStyle;
        bodyStyle.fontSize = 16;
        bodyStyle.color = Color::WHITE;
        bodyStyle.spacing = 1.5f;
        bodyStyle.lineSpacing = 3.0f;
        
        font.Print("Game Title", 300, 50, titleStyle);
        font.Print("Chapter 1: The Beginning", 300, 90, subtitleStyle);
        font.Print("Regular game text goes here...", 300, 120, bodyStyle);
    }

      {
        // Menu
        struct MenuItem
        {
            const char* text;
            FloatRect bounds;
            bool selected;
        };
        
        MenuItem items[] = {
            {"New Game", FloatRect(300, 200, 200, 40), false},
            {"Continue", FloatRect(300, 250, 200, 40), true},
            {"Options", FloatRect(300, 300, 200, 40), false},
            {"Exit", FloatRect(300, 350, 200, 40), false}
        };
        
        for (const auto& item : items)
        {
            TextStyle menuStyle;
            menuStyle.fontSize = 22;
            menuStyle.align = TextAlign::CENTER;
            menuStyle.valign = TextVAlign::MIDDLE;
            
            if (item.selected)
            {
                menuStyle.color = Color::YELLOW;
                menuStyle.enableOutline = true;
                menuStyle.outlineColor = Color::BLACK;
                menuStyle.outlineThickness = 1.5f;
            }
            else
            {
                menuStyle.color = Color::WHITE;
            }
            
            font.PrintAligned(item.text, item.bounds,
                            menuStyle.align, menuStyle.valign);
        }
    }

      {
        FloatRect dialogBox(50, 600, 700, 150);
        
        // Fundo do diálogo
        batch.SetColor(0, 0, 0, 200);
        batch.Rectangle(dialogBox.x, dialogBox.y, 
                       dialogBox.width, dialogBox.height, true);
        
        // Nome do personagem
        TextStyle nameStyle;
        nameStyle.fontSize = 20;
        nameStyle.color = Color::CYAN;
        nameStyle.enableOutline = true;
        nameStyle.outlineColor = Color::BLACK;
        
        font.Print("Hero:", dialogBox.x + 10, dialogBox.y + 10, nameStyle);
        
        // Texto do diálogo com wrapping
        const char* dialogText = "This is a long dialogue that will automatically "
                                "wrap to fit within the dialog box. The text "
                                "rendering system handles this automatically!";
        
        FloatRect textArea(dialogBox.x + 10, dialogBox.y + 40,
                          dialogBox.width - 20, dialogBox.height - 50);
        
        TextStyle dialogStyle;
        dialogStyle.fontSize = 16;
        dialogStyle.color = Color::WHITE;
        dialogStyle.lineSpacing = 3;
        
        font.PrintWrapped(dialogText, textArea, dialogStyle);
    }
    
    {
        FloatRect tooltipBounds(400, 500, 250, 80);
        
        // Fundo semi-transparente
        batch.SetColor(0, 0, 0, 180);
        batch.Rectangle(tooltipBounds.x, tooltipBounds.y,
                       tooltipBounds.width, tooltipBounds.height, true);
        
        // Borda
        batch.SetColor(255, 255, 255, 255);
        batch.Rectangle(tooltipBounds.x, tooltipBounds.y,
                       tooltipBounds.width, tooltipBounds.height, false);
        
        // Texto centralizado
        TextStyle tooltipStyle;
        tooltipStyle.fontSize = 14;
        tooltipStyle.color = Color::WHITE;
        tooltipStyle.align = TextAlign::CENTER;
        tooltipStyle.valign = TextVAlign::MIDDLE;
        
        FloatRect textArea(tooltipBounds.x + 10, tooltipBounds.y + 10,
                          tooltipBounds.width - 20, tooltipBounds.height - 20);
        
        font.PrintWrapped("Press [E] to interact with this object", 
                         textArea, tooltipStyle);
    }


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
