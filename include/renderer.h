#pragma once

#include <SDL3/SDL.h>
#include <string>

class Renderer
{
    public:
        Renderer();
        ~Renderer();
        bool Init(const std::string& title, int width, int height);
        void Shutdown();
        void BeginFrame();
        void EndFrame();
        void DrawPoint(int x, int y, SDL_Color color);
        void DrawLine(int x1, int y1, int x2, int y2, SDL_Color color);
        void GetWindowSize(int& outWidth, int& outHeight);
        SDL_Renderer* GetSDLRenderer();
        SDL_Window* GetSDLWindow();
    private:
        SDL_Window* window;
        SDL_Renderer* renderer;
        int width;
        int height;
};