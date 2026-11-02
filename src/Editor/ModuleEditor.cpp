#include "ModuleEditor.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRenderer3D.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "PrimitiveGenerator.h"
#include "Log.h"
#include "ModuleInput.h"
#include <SDL3/SDL.h>
#include <assimp/version.h>
#include <glm/glm.hpp>
#include <cfloat>
#include "ModuleCamera3D.h"
#include "ModuleImporter.h"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif
#include <string>
#include <vector>
#include <cstdio>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <IL/il.h>
#include <glad/glad.h>

// --- DATOS DEL PROYECTO ---
static const std::string REPO_URL = "https://github.com/Katy-9/Engine";
static const char* ENGINE_VERSION = "0.1";

struct TeamMember { const char* name; const char* github; };
static const TeamMember TEAM[] = {
    { "Queralt Escutia", "Keral06" },
    { "Sofía Giner", "Katy-9" },
    { "Joel Martínez", "Jowey7"}
};

// ImGuiCond_FirstUseEver: solo la primera vez; si el usuario la mueve, se respeta.
static void PlaceWindow(float x, float y, float w, float h) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float top = ImGui::GetFrameHeight();
    const float areaW = vp->Size.x;
    const float areaH = vp->Size.y - top;

    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + x * areaW, vp->Pos.y + top + y * areaH), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w * areaW, h * areaH), ImGuiCond_Always);
}

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

    io.IniFilename = nullptr;
    App->input->AddEventListener([](const SDL_Event& e) { ImGui_ImplSDL3_ProcessEvent(&e); });

    return true;
}

update_status ModuleEditor::PreUpdate(float dt) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    App->input->uiCapturesMouse = ImGui::GetIO().WantCaptureMouse;
    App->input->uiCapturesKeyboard = ImGui::GetIO().WantTextInput;
    return UPDATE_CONTINUE;
}

update_status ModuleEditor::Update(float dt) {
    update_status ret = DrawMainMenuBar();

    if (showConsole) DrawConsole();
    if (showHierarchy) DrawHierarchy();
    if (showInspector) DrawInspector();
    if (showAboutWindow) DrawAboutWindow();
    if (showConfigurationWindow) DrawConfiguration();
    if (App->scene->selected != nullptr && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            App->scene->DestroyGameObject(App->scene->selected);
        }
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
            GameObject* clonedObj = App->scene->selected->Clone();
            App->scene->selected = clonedObj;
            LOG("GameObject duplicated: %s", clonedObj->name.c_str());
        }
    }
    return ret;
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

update_status ModuleEditor::DrawMainMenuBar() {
    update_status ret = UPDATE_CONTINUE;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) { ret = UPDATE_STOP; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::BeginMenu("3D Object")) {
                GameObject* root = App->scene->GetRoot();
                if (ImGui::MenuItem("Cube"))     App->scene->selected = PrimitiveGenerator::CreateCube(root);
                if (ImGui::MenuItem("Sphere"))   App->scene->selected = PrimitiveGenerator::CreateSphere(20, 20, root);
                if (ImGui::MenuItem("Plane"))    App->scene->selected = PrimitiveGenerator::CreatePlane(root);
                if (ImGui::MenuItem("Cone"))     App->scene->selected = PrimitiveGenerator::CreateCone(20, root);
                ImGui::Separator();
                if (ImGui::MenuItem("Pyramid"))  App->scene->selected = PrimitiveGenerator::CreatePyramid(root);
                if (ImGui::MenuItem("Cylinder")) App->scene->selected = PrimitiveGenerator::CreateCylinder(20, root);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &showInspector);
            ImGui::MenuItem("Console", nullptr, &showConsole);
            ImGui::MenuItem("Configuration", nullptr, &showConfigurationWindow);
            ImGui::MenuItem("About", nullptr, &showAboutWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("GitHub Documentation")) SDL_OpenURL((REPO_URL + "/docs").c_str());
            if (ImGui::MenuItem("Report a bug")) SDL_OpenURL((REPO_URL + "/issues").c_str());
            if (ImGui::MenuItem("Download latest")) SDL_OpenURL((REPO_URL + "/releases").c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("About Kuroko Engine")) showAboutWindow = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    return ret;
}

void ModuleEditor::DrawAboutWindow() {
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::Begin("About Kuroko Engine", &showAboutWindow, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Kuroko Engine v%s", ENGINE_VERSION);
    ImGui::Text("Motor 3D desarrollado como proyecto universitario.");
    ImGui::Separator();

    ImGui::Text("Equipo:");
    for (const TeamMember& member : TEAM) {
        ImGui::BulletText("%s (github.com/%s)", member.name, member.github);
    }
    ImGui::Separator();

    ImGui::Text("Librerias usadas:");
    ImGui::BulletText("SDL3");
    ImGui::BulletText("OpenGL (GLAD)");
    ImGui::BulletText("Dear ImGui");
    ImGui::BulletText("Assimp");
    ImGui::BulletText("DevIL");
    ImGui::BulletText("GLM");
    ImGui::Separator();

    ImGui::Text("Licencia MIT");
    ImGui::End();
}

void ModuleEditor::DrawHierarchy() {
    PlaceWindow(0.00f, 0.00f, 0.20f, 0.70f);
    ImGui::Begin("Hierarchy", &showHierarchy);
    if (App->scene != nullptr && App->scene->GetRoot() != nullptr) {
        DrawHierarchyNode(App->scene->GetRoot());
    }
    ImGui::End();

}

void ModuleEditor::DrawHierarchyNode(GameObject* go) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (App->scene->selected == go) flags |= ImGuiTreeNodeFlags_Selected;
    if (go->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    bool nodeOpen = ImGui::TreeNodeEx(go, flags, "%s", go->name.c_str());
    if (ImGui::IsItemClicked()) {
        App->scene->selected = go;
    }

    //delete
    if (ImGui::BeginPopupContextItem()) {
        App->scene->selected = go; 
        if (ImGui::MenuItem("Delete")) {
            App->scene->DestroyGameObject(go);
        }
        ImGui::EndPopup();
    }
    if (nodeOpen && !go->children.empty()) {
        for (GameObject* child : go->children) {
            DrawHierarchyNode(child);
        }
        ImGui::TreePop();
    }
}

void ModuleEditor::DrawConsole() {
    PlaceWindow(0.00f, 0.70f, 0.75f, 0.30f);
    ImGui::Begin("Console (Engine Logs)", &showConsole);
    extern std::vector<std::string> g_EngineLog;
    for (const std::string& log : g_EngineLog) {
        ImGui::TextUnformatted(log.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::End();
}

void ModuleEditor::DrawInspector() {
    PlaceWindow(0.75f, 0.00f, 0.25f, 0.55f);
    ImGui::Begin("Inspector", &showInspector);
    GameObject* selectedGameObject = App->scene->selected;

    if (selectedGameObject != nullptr) {
        ImGui::Checkbox("##Active", &selectedGameObject->active);
        ImGui::SameLine();

        char nameBuffer[256];
        snprintf(nameBuffer, sizeof(nameBuffer), "%s", selectedGameObject->name.c_str());
        if (ImGui::InputText("##Name", nameBuffer, 256)) {
            selectedGameObject->name = nameBuffer;
        }

        ImGui::Separator();

        ComponentTransform* transform = selectedGameObject->GetComponent<ComponentTransform>(ComponentType::TRANSFORM);
        if (transform != nullptr) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Position", &transform->position[0], 0.1f);
                if (ImGui::DragFloat3("Rotation", &transform->eulerRotation[0], 0.5f)) {
                    transform->UpdateRotationFromEuler();
                }
                ImGui::DragFloat3("Scale", &transform->scale[0], 0.1f);
            }
        }

        ComponentMesh* mesh = selectedGameObject->GetComponent<ComponentMesh>(ComponentType::MESH);
        if (mesh != nullptr) {
            if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Vertices: %d", mesh->numVertices);
                ImGui::Text("Indices: %d", mesh->numIndices);
                ImGui::Text("Triangles: %d", mesh->numIndices / 3);
                ImGui::Checkbox("Wireframe", &mesh->showWireframe);
                ImGui::Checkbox("Vertex Normals", &mesh->showVertexNormals);
                ImGui::Checkbox("Face Normals", &mesh->showFaceNormals);
            }
        }

        ComponentMaterial* material = selectedGameObject->GetComponent<ComponentMaterial>(ComponentType::MATERIAL);
        if (material != nullptr) {
            if (ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Path: %s", material->texturePath.c_str());
                ImGui::Text("Size: %dx%d", material->textureWidth, material->textureHeight);
                if (material->textureID != 0) {
                    ImGui::Image((ImTextureID)(intptr_t)material->textureID, ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));
                }
                ImGui::ColorEdit4("Color Tint", material->colorTint);
                ImGui::Checkbox("Use Checker Texture", &material->useCheckerTexture);
            }
        }
    }
    else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a GameObject to inspect.");
    }

    ImGui::End();
}

void ModuleEditor::DrawConfiguration() {
    PlaceWindow(0.75f, 0.55f, 0.25f, 0.45f);
    ImGui::Begin("Configuration", &showConfigurationWindow);

    if (ImGui::CollapsingHeader("Application", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Kuroko Engine v%s", ENGINE_VERSION);

        const std::vector<float>& fps = App->fps_history;
        if (!fps.empty()) {
            char title[32];
            snprintf(title, sizeof(title), "Framerate %.1f", fps.back());
            ImGui::PlotHistogram("##framerate", fps.data(), (int)fps.size(), 0, title, 0.0f, FLT_MAX, ImVec2(310, 100));
        }
    }

    if (ImGui::CollapsingHeader("Memory", ImGuiTreeNodeFlags_DefaultOpen)) {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            ImGui::Text("Working set: %.1f MB", pmc.WorkingSetSize / (1024.0 * 1024.0));
            ImGui::Text("Peak working set: %.1f MB", pmc.PeakWorkingSetSize / (1024.0 * 1024.0));
            ImGui::Text("Pagefile usage: %.1f MB", pmc.PagefileUsage / (1024.0 * 1024.0));
        }
#else
        ImGui::TextUnformatted("Process memory info is only available on Windows");
#endif
        ImGui::Text("System RAM: %d MB", SDL_GetSystemRAM());
    }

    if (ImGui::CollapsingHeader("Hardware", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("CPUs: %d", SDL_GetNumLogicalCPUCores());
        ImGui::Text("GPU: %s", (const char*)glGetString(GL_RENDERER));
        ImGui::Text("Vendor: %s", (const char*)glGetString(GL_VENDOR));
    }

    if (ImGui::CollapsingHeader("Software", ImGuiTreeNodeFlags_DefaultOpen)) {
        const int sdl = SDL_GetVersion();
        const int il = ilGetInteger(IL_VERSION_NUM);   // 180 = 1.8.0
        ImGui::Text("SDL: %d.%d.%d", SDL_VERSIONNUM_MAJOR(sdl), SDL_VERSIONNUM_MINOR(sdl), SDL_VERSIONNUM_MICRO(sdl));
        ImGui::Text("OpenGL: %s", (const char*)glGetString(GL_VERSION));
        ImGui::Text("GLSL: %s", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
        ImGui::Text("DevIL: %d.%d.%d", il / 100, (il / 10) % 10, il % 10);
        ImGui::Text("Assimp: %d.%d.%d", aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionPatch());
        ImGui::Text("ImGui: %s", ImGui::GetVersion());
        ImGui::Text("GLM: %d.%d.%d", GLM_VERSION_MAJOR, GLM_VERSION_MINOR, GLM_VERSION_PATCH);
    }

    if (ImGui::CollapsingHeader("Window")) {
        ImGui::Text("Size: %d x %d", App->window->width, App->window->height);
        if (ImGui::Checkbox("Fullscreen", &App->window->fullscreen)) {
            App->window->SetFullscreen(App->window->fullscreen);
        }
    }

    if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Checkbox("VSync", &App->renderer3D->vsync)) {
            App->renderer3D->SetVSync(App->renderer3D->vsync);
        }
        ImGui::Checkbox("Depth test", &App->renderer3D->depthTest);
        ImGui::Checkbox("Cull face", &App->renderer3D->cullFace);
        ImGui::Checkbox("Wireframe (all objects)", &App->renderer3D->wireframe);
        ImGui::Separator();
        ImGui::Checkbox("Grid", &App->renderer3D->showGrid);
        ImGui::Checkbox("Axis", &App->renderer3D->showAxis);
        ImGui::Checkbox("Bounding boxes (AABB)", &App->renderer3D->showAABB);
    }

    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::SliderFloat("Move speed", &App->camera->baseSpeed, 1.0f, 200.0f);
        ImGui::SliderFloat("Mouse sensitivity", &App->camera->turnSpeed, 0.05f, 1.0f);
        ImGui::SliderFloat("Field of view", &App->camera->fov, 30.0f, 120.0f);
        ImGui::Text("Position: %.1f, %.1f, %.1f", App->camera->Position.x, App->camera->Position.y, App->camera->Position.z);
    }

    if (ImGui::CollapsingHeader("Input")) {
        float mx, my;
        SDL_GetMouseState(&mx, &my);
        ImGui::Text("Mouse: %.0f, %.0f", mx, my);
        ImGui::Text("Wheel: %.1f", App->input->mouse_wheel);
        ImGui::Text("UI captures mouse: %s", App->input->uiCapturesMouse ? "yes" : "no");
    }

    if (ImGui::CollapsingHeader("Textures")) {
        ImGui::Text("Textures loaded: %d", (int)App->importer->GetLoadedTextureCount());
    }

    ImGui::End();
}