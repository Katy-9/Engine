#include "ModuleEditor.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRenderer3D.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "Log.h"

#include <string>
#include <vector>
#include <cstdio> // Para snprintf
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
    update_status ret = DrawMainMenuBar();

    DrawConsole();
    DrawHierarchy();
    DrawInspector();

    if (showAboutWindow) {
        DrawAboutWindow();
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
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("GitHub Repository")) { system("start https://github.com/tu-usuario/tu-repo"); }
            if (ImGui::MenuItem("About Kuroko Engine")) { showAboutWindow = !showAboutWindow; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    return ret;
}

void ModuleEditor::DrawAboutWindow() {
    ImGui::Begin("About Kuroko Engine", &showAboutWindow);
    ImGui::Text("Kuroko Engine v0.1");
    ImGui::Text("Desarrollado por: [Tu Nombre]");
    ImGui::Separator();
    ImGui::Text("Librerias usadas:");
    ImGui::BulletText("SDL3");
    ImGui::BulletText("OpenGL 4.6 (GLAD)");
    ImGui::BulletText("ImGui");
    ImGui::BulletText("Assimp");
    ImGui::BulletText("DevIL & GLM");
    ImGui::Separator();
    ImGui::Text("Licencia MIT");
    ImGui::End();
}

void ModuleEditor::DrawHierarchy() {
    ImGui::Begin("Hierarchy");
    if (App->scene != nullptr && App->scene->GetRoot() != nullptr) {
        DrawHierarchyNode(App->scene->GetRoot());
    }
    ImGui::End();
}

void ModuleEditor::DrawHierarchyNode(GameObject* go) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (selectedGameObject == go) flags |= ImGuiTreeNodeFlags_Selected;
    if (go->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    bool nodeOpen = ImGui::TreeNodeEx(go, flags, go->name.c_str());
    if (ImGui::IsItemClicked()) {
        selectedGameObject = go;
    }

    if (nodeOpen && !go->children.empty()) {
        for (GameObject* child : go->children) {
            DrawHierarchyNode(child);
        }
        ImGui::TreePop();
    }
}

void ModuleEditor::DrawConsole() {
    ImGui::Begin("Console (Engine Logs)");
    extern std::vector<std::string> g_EngineLog;
    for (const std::string& log : g_EngineLog) {
        ImGui::TextUnformatted(log.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::End();
}

void ModuleEditor::DrawInspector() {
    ImGui::Begin("Inspector");

    if (selectedGameObject != nullptr) {
        ImGui::Checkbox("##Active", &selectedGameObject->active);
        ImGui::SameLine();

        // --- ARREGLADO EL MANEJO DE STRINGS PARA IMGUI ---
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
                ImGui::ColorEdit4("Color Tint", material->colorTint);
            }
        }
    }
    else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a GameObject to inspect.");
    }

    ImGui::End();
}