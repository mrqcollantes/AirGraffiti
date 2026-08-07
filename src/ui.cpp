#include "ui.h"
#include "renderer.h"
#include "brush.h"
#include "canvas.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

UI::UI()
{
}

bool UI::Init(Renderer& renderer)
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    sdlRenderer = renderer.GetSDLRenderer();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(
            renderer.GetSDLWindow(),
            renderer.GetSDLRenderer()))
    {
        SDL_Log("ERROR: ImGui SDL3 initialization failed!");
        return false;
    }

    if (!ImGui_ImplSDLRenderer3_Init(
            renderer.GetSDLRenderer()))
    {
        SDL_Log("ERROR: ImGui SDL Renderer initialization failed!");
        return false;
    }

    SDL_Log("ImGui initialized successfully!");

    return true;
}

void UI::BeginFrame()
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void UI::Draw(Brush& brush, Canvas& canvas, Renderer& renderer)
{
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 160), ImGuiCond_FirstUseEver);

    ImGui::Begin("AirGraffiti");

    // --- Color picker ---
    SDL_Color current = brush.GetColor();
    float colorArr[4] = {
        current.r / 255.0f,
        current.g / 255.0f,
        current.b / 255.0f,
        current.a / 255.0f
    };

    if (ImGui::ColorEdit4("Color", colorArr))
    {
        SDL_Color newColor = {
            static_cast<Uint8>(colorArr[0] * 255.0f),
            static_cast<Uint8>(colorArr[1] * 255.0f),
            static_cast<Uint8>(colorArr[2] * 255.0f),
            static_cast<Uint8>(colorArr[3] * 255.0f)
        };
        brush.SetColor(newColor);
    }

    // --- Brush size ---
    int size = brush.GetSize();
    if (ImGui::SliderInt("Brush Size", &size, 1, 64))
    {
        brush.SetSize(size);
    }

    ImGui::Separator();

    // --- Clear canvas ---
    if (ImGui::Button("Clear Canvas"))
    {
        canvas.Clear(renderer);
    }

    ImGui::End();
}

void UI::EndFrame()
{
    ImGui::Render();

    ImGui_ImplSDLRenderer3_RenderDrawData(
        ImGui::GetDrawData(),
        sdlRenderer
    );
}

void UI::Shutdown()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    ImGui::DestroyContext();
}