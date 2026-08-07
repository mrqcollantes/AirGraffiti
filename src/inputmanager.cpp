#include "inputmanager.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui.h"

InputManager::InputManager()
{
    quit = false;
}

void InputManager::Update()
{
    SDL_Event event;

    pointer.previousX = pointer.x;
    pointer.previousY = pointer.y;

    ImGuiIO& io = ImGui::GetIO();

    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            quit = true;
            break;

        case SDL_EVENT_MOUSE_MOTION:
            if (!io.WantCaptureMouse)
            {
                pointer.x = event.motion.x;
                pointer.y = event.motion.y;
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT && !io.WantCaptureMouse)
            {
                pointer.drawing = true;
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                pointer.drawing = false;
            }
            break;
        }
    }
}

bool InputManager::ShouldQuit() const
{
    return quit;
}

Pointer& InputManager::GetPointer()
{
    return pointer;
}