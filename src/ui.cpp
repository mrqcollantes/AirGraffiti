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
    ImGuiIO& io = ImGui::GetIO();

    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;

    const float panelHeight = PANEL_HEIGHT;

    // Pin the panel to the bottom of the window and make it full width.
    ImGui::SetNextWindowPos(ImVec2(0, windowHeight), ImGuiCond_Always, ImVec2(0, 1));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, panelHeight), ImGuiCond_Always);
    ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("AirGraffiti", nullptr, panelFlags);
    ImGui::Text("SIZE");
    ImGui::SameLine();

    struct BrushSizeOption
    {
        const char* label;
        int size;
    };

    const BrushSizeOption sizes[] =
    {
        {"SMALL",  8},
        {"MEDIUM", 16},
        {"LARGE",  32},
        {"XL",     64}
    };

    for (const auto& option : sizes)
    {
        bool selected = (brush.GetSize() == option.size);

        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (ImGui::Button(option.label, ImVec2(125.0f, 50.0f)))
        {
            brush.SetSize(option.size);
        }

        if (selected)
        {
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
    }

    // Space between SIZE and COLOR
    ImGui::Dummy(ImVec2(15.0f, 0.0f));
    ImGui::SameLine();

    // COLOR
    ImGui::Text("COLOR");
    ImGui::SameLine();

    struct ColorOption
    {
        const char* id;
        SDL_Color color;
    };

    const ColorOption colors[] =
    {
        {"##color_black",  {0,   0,   0,   255}},
        {"##color_white",  {255, 255, 255, 255}},
        {"##color_red",    {255, 0,   0,   255}},
        {"##color_orange", {255, 128, 0,   255}},
        {"##color_yellow", {255, 220, 0,   255}},
        {"##color_green",  {0,   200, 70,  255}},
        {"##color_cyan",   {0,   210, 255, 255}},
        {"##color_blue",   {30,  100, 255, 255}},
        {"##color_purple", {150, 60,  255, 255}},
        {"##color_pink",   {255, 60, 170, 255}}
    };

    SDL_Color currentColor = brush.GetColor();

    for (const auto& option : colors)
    {
        bool selected =
            currentColor.r == option.color.r &&
            currentColor.g == option.color.g &&
            currentColor.b == option.color.b &&
            currentColor.a == option.color.a;

        ImVec4 colorVec(
            option.color.r / 255.0f,
            option.color.g / 255.0f,
            option.color.b / 255.0f,
            option.color.a / 255.0f
        );

        if (selected)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 3.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f );
        }

        if (ImGui::ColorButton(option.id, colorVec,
            ImGuiColorEditFlags_NoPicker |
            ImGuiColorEditFlags_NoTooltip,
            ImVec2(50.0f, 50.0f)))
        {
            brush.SetColor(option.color);
            brush.SetMode(Brush::Mode::Spray);
        }

        ImGui::PopStyleVar();

        if (selected)
        {
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
    }

    // Space between COLOR and TOOL
    ImGui::Dummy(ImVec2(15.0f, 0.0f));
    ImGui::SameLine();

    // TOOL
    ImGui::Text("TOOL");
    ImGui::SameLine();

    bool spraySelected = brush.GetMode() == Brush::Mode::Spray;
    bool eraserSelected = brush.GetMode() == Brush::Mode::Eraser;

    if (spraySelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    if (ImGui::Button("SPRAY", ImVec2(175.0f, 50.0f)))
    {
        brush.SetMode(Brush::Mode::Spray);
    }

    if (spraySelected)
    {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    if (eraserSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    if (ImGui::Button("ERASER", ImVec2(175.0f, 50.0f)))
    {
        brush.SetMode(Brush::Mode::Eraser);
    }

    if (eraserSelected)
    {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    if (ImGui::Button("CLEAR CANVAS", ImVec2(175.0f, 50.0f)))
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