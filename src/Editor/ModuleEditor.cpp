#include "ModuleEditor.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRenderer3D.h"
#include "Log.h"

// --- AÑADIDO AQUI ---
#include <string>
#include <vector>
// --------------------

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

ModuleEditor::ModuleEditor(Application* app) : Module(app) {}
ModuleEditor::~ModuleEditor() {}

bool ModuleEditor::Init() {
    LOG("Initializing ImGui Editor");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(App->window->window, App->renderer3D->context);
    ImGui_ImplOpenGL3_Init("#version 460");

    return true;
}

update_status ModuleEditor::PreUpdate(float dt) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    return UPDATE_CONTINUE;
}

update_status ModuleEditor::Update(float dt) {
    DrawMainMenuBar();
    DrawConsole();

    // Descomenta la siguiente linea si quieres ver una ventana de demo de ImGui con todo lo que puede hacer:
    // ImGui::ShowDemoWindow(); 

    return UPDATE_CONTINUE;
}

update_status ModuleEditor::PostUpdate(float dt) {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return UPDATE_CONTINUE;
}

bool ModuleEditor::CleanUp() {
    LOG("Destroying ImGui Editor");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return true;
}

void ModuleEditor::DrawMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) { /* Lo conectaremos luego */ }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About Kuroko Engine")) { /* Info */ }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ModuleEditor::DrawConsole() {
    ImGui::Begin("Console (Engine Logs)");

    // Leemos la variable global donde el motor guarda los logs
    extern std::vector<std::string> g_EngineLog;

    for (const std::string& log : g_EngineLog) {
        ImGui::TextUnformatted(log.c_str());
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::End();
}