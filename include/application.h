#pragma once

#include <SDL3/SDL.h>
#include "renderer.h"
#include "canvas.h"
#include "brush.h"
#include "inputmanager.h"
#include "ui.h"
#include "wiimote.h"

class Application
{
    public:
        Application();
        bool Init();
        void Run();
        void Shutdown();
    private:
        bool running;
        Renderer renderer;
        Canvas canvas;
        Brush brush;
        InputManager input;
        UI ui;
        WiiMote wiimote;
        void Update();
        void Render();
};