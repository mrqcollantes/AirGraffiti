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

        // Previous IR point state for drawing lines between points
        bool previousIRValid = false;
        int previousIRX = 0;
        int previousIRY = 0;

        void Update();
        void Render();
};