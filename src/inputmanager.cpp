#include "inputmanager.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui.h"

// The InputManager class handles user input events, including mouse movement and button presses.

InputManager::InputManager()
{
    quit = false;
}

// Updates the input state by polling SDL events and processing them.
void InputManager::Update()
{
    quit = false;

    SDL_Event event;

    // Update the previous pointer position before processing new events.
    pointer.previousX = pointer.x;
    pointer.previousY = pointer.y;

    while (SDL_PollEvent(&event))
    {
        // Always give SDL events to ImGui.
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                quit = true;
                break;

            case SDL_EVENT_MOUSE_MOTION:
                // Always track the physical mouse position.
                pointer.x = event.motion.x;
                pointer.y = event.motion.y;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT)
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