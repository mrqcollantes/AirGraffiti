#pragma once

#include <SDL3/SDL.h>

class Renderer;
class Brush;
class Canvas;

class UI
{
    public:
        UI();
        bool Init(Renderer& renderer);
        void BeginFrame();
        void Draw(Brush& brush, Canvas& canvas, Renderer& renderer);
        void EndFrame();
        void Shutdown();

        // Feeds a synthetic pointer (e.g. Wii Remote IR position) into
        // ImGui as if it were a mouse, in window pixel coordinates.
        void SubmitPointerPosition(float windowX, float windowY);

        // Feeds a synthetic left mouse button state into ImGui. Call
        // only on real press/release edges, not every frame.
        void SubmitPointerButton(bool pressed);

        // Returns true if ImGui wants to capture mouse input, e.g. when the
        // pointer is over the pinned bottom panel.
        bool WantsPointerCapture() const;

        // Returns the height of the pinned bottom panel in window pixels. Use this to
        // determine whether a synthetic pointer is over the panel or not.
        static constexpr float PANEL_HEIGHT = 130.0f;

        // Returns the current display size in window pixels, as reported by ImGui.
        void GetDisplaySize(float& outWidth, float& outHeight) const;
    private:
        SDL_Renderer* sdlRenderer = nullptr;
};