#pragma once

#include <SDL3/SDL.h>

#include "renderer.h"
#include "canvas.h"
#include "brush.h"
#include "inputmanager.h"
#include "ui.h"
#include "wiimotemanager.h"

class Application
{
    public:
        Application();

        bool Init();
        void Run();
        void Shutdown();

    private:
        bool running = false;

        Renderer renderer;
        Canvas canvas;
        Brush brush;
        InputManager input;
        UI ui;
        WiiMoteManager wiimoteManager;

        bool previousIRValid = false;
        int previousIRX = 0;
        int previousIRY = 0;

        bool irWasActive = false;
        bool isUIPress = false;

        void Update();
        void Render();
};