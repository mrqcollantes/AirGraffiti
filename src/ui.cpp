#include "ui.h"
#include "renderer.h"
#include "brush.h"
#include "canvas.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

UI::UI(){}

bool UI::Init(Renderer& renderer)
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    sdlRenderer = renderer.GetSDLRenderer();

    ImGuiIO& io = ImGui::GetIO();

    // No ini file: the panel is pinned every frame anyway, and we don't
    // want a collapsed state from a previous run to persist.
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForSDLRenderer(renderer.GetSDLWindow(), renderer.GetSDLRenderer()))
    {
        SDL_Log("ERROR: ImGui SDL3 initialization failed!");
        return false;
    }

    if (!ImGui_ImplSDLRenderer3_Init(renderer.GetSDLRenderer()))
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
    // Use io.DisplaySize, not Renderer::GetWindowSize() - it's the
    // exact space ImGui renders into, so the panel stays lined up.
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;

    const float panelHeight = PANEL_HEIGHT;

    // Pin the panel to the bottom of the window, and make it the full width of the window.
    ImGui::SetNextWindowPos(ImVec2(0, windowHeight), ImGuiCond_Always, ImVec2(0, 1));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, panelHeight), ImGuiCond_Always);

    ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("AirGraffiti", nullptr, panelFlags);

    // Color picker
    SDL_Color current = brush.GetColor();
    float colorArr[4] = {current.r / 255.0f, current.g / 255.0f, current.b / 255.0f, current.a / 255.0f};

    if (ImGui::ColorEdit4("Color", colorArr))
    {
        SDL_Color newColor = {
            static_cast<Uint8>(colorArr[0] * 255.0f),
            static_cast<Uint8>(colorArr[1] * 255.0f),
            static_cast<Uint8>(colorArr[2] * 255.0f),
            static_cast<Uint8>(colorArr[3] * 255.0f)};
        brush.SetColor(newColor);
    }

    // Brush size
    int size = brush.GetSize();
    if (ImGui::SliderInt("Brush Size", &size, 1, 64))
    {
        brush.SetSize(size);
    }

    ImGui::Separator();

    // Clear canvas
    if (ImGui::Button("Clear Canvas"))
    {
        canvas.Clear(renderer);
    }

    ImGui::End();
}

void UI::SubmitPointerPosition(float windowX, float windowY)
{
    ImGui::GetIO().AddMousePosEvent(windowX, windowY);
}

void UI::SubmitPointerButton(bool pressed)
{
    ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, pressed);
}

bool UI::WantsPointerCapture() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

void UI::GetDisplaySize(float& outWidth, float& outHeight) const
{
    ImGuiIO& io = ImGui::GetIO();
    outWidth = io.DisplaySize.x;
    outHeight = io.DisplaySize.y;
}

void UI::EndFrame()
{
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), sdlRenderer);
}

void UI::Shutdown()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}