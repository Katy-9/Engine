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

#include "ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>

// --- DATOS DEL PROYECTO ---
static const std::string REPO_URL = "https://github.com/Katy-9/Engine";
static const char* ENGINE_VERSION = "0.1";

static ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
static ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::LOCAL;

// --- HISTORIAL DE UNDO (CTRL + Z) ---
struct UndoState {
    GameObject* go;
    glm::vec3 pos;
    glm::vec3 euler;
    glm::quat rot;
    glm::vec3 scale;
};
static std::vector<UndoState> g_UndoStack;
static bool g_WasUsingGizmo = false;
static bool g_MaximizeViewport = false;

static void PushUndo(GameObject* go) {
    if (!go) return;
    ComponentTransform* t = go->GetComponent<ComponentTransform>(ComponentType::TRANSFORM);
    if (!t) return;
    if (g_UndoStack.size() > 50) g_UndoStack.erase(g_UndoStack.begin());
    g_UndoStack.push_back({ go, t->position, t->eulerRotation, t->rotation, t->scale });
}

struct TeamMember { const char* name; const char* github; };
static const TeamMember TEAM[] = {
    { "Queralt Escutia", "Keral06" },
    { "Sofía Giner", "Katy-9" },
    { "Joel Martínez", "Jowey7"}
};

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

    io.ConfigWindowsMoveFromTitleBarOnly = true;

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

    ImGuizmo::BeginFrame();

    App->input->uiCapturesMouse = ImGui::GetIO().WantCaptureMouse;
    App->input->uiCapturesKeyboard = ImGui::GetIO().WantTextInput;
    return UPDATE_CONTINUE;
}

update_status ModuleEditor::Update(float dt) {
    update_status ret = DrawMainMenuBar();
    DrawSceneWindow();

    // Solo dibujamos los demás paneles si el Viewport NO está maximizado
    if (!g_MaximizeViewport) {
        if (showConsole) DrawConsole();
        if (showHierarchy) DrawHierarchy();
        if (showInspector) DrawInspector();
        if (showConfigurationWindow) DrawConfiguration();
    }
    if (showAboutWindow) DrawAboutWindow();

    if (!ImGui::GetIO().WantTextInput) {
        // ESPACIO: Maximizar / Restaurar el Scene Viewport
        if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
            g_MaximizeViewport = !g_MaximizeViewport;
        }

        // CTRL + Z (Deshacer última transformación y limpiar línea de debug)
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !g_UndoStack.empty()) {
            UndoState last = g_UndoStack.back();
            g_UndoStack.pop_back();
            ComponentTransform* t = last.go->GetComponent<ComponentTransform>(ComponentType::TRANSFORM);
            if (t) {
                t->position = last.pos;
                t->eulerRotation = last.euler;
                t->rotation = last.rot;
                t->scale = last.scale;
                App->scene->selected = last.go;
                App->renderer3D->hasLastRay = false;
                LOG("Undo applied to: %s", last.go->name.c_str());
            }
        }

        if (App->scene->selected != nullptr) {
            if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
                renamingObject = App->scene->selected;
                snprintf(renameBuffer, sizeof(renameBuffer), "%s", renamingObject->name.c_str());
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                if (g_UndoStack.size() > 0) g_UndoStack.clear();
                App->renderer3D->hasLastRay = false;
                App->scene->DestroyGameObject(App->scene->selected);
            }
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
                GameObject* clonedObj = App->scene->selected->Clone();
                App->scene->selected = clonedObj;
                LOG("GameObject duplicated: %s", clonedObj->name.c_str());
            }
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
            if (ImGui::MenuItem("Create Empty")) {
                GameObject* empty = new GameObject("Empty GameObject", App->scene->GetRoot());
                empty->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);
                App->scene->selected = empty;
            }

			ImGui::Separator();

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
            ImGui::MenuItem("Scene Viewport", nullptr, &showSceneWindow);
            ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &showInspector);
            ImGui::MenuItem("Console", nullptr, &showConsole);
            ImGui::MenuItem("Configuration", nullptr, &showConfigurationWindow);
            ImGui::MenuItem("About", nullptr, &showAboutWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("GitHub Documentation")) SDL_OpenURL((REPO_URL).c_str());
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

void ModuleEditor::DrawSceneWindow() {
    if (g_MaximizeViewport) PlaceWindow(0.00f, 0.00f, 1.00f, 1.00f);
    else                    PlaceWindow(0.20f, 0.00f, 0.55f, 0.70f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Scene Viewport", &showSceneWindow);

    App->camera->isViewportHovered = ImGui::IsWindowHovered();
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    if (viewportSize.x > 0 && viewportSize.y > 0) {
        if (viewportSize.x != App->renderer3D->renderWidth || viewportSize.y != App->renderer3D->renderHeight) {
            App->renderer3D->CreateFramebuffer((int)viewportSize.x, (int)viewportSize.y);
            App->camera->SetAspectRatio(viewportSize.x / viewportSize.y);
        }
    }

    ImVec2 windowPos = ImGui::GetCursorScreenPos();
    if (App->renderer3D->sceneTexture != 0) {
        ImGui::Image((ImTextureID)(intptr_t)App->renderer3D->sceneTexture, viewportSize, ImVec2(0, 1), ImVec2(1, 0));
    }

    // 1. DIBUJAR Y ACTUALIZAR EL GIZMO + CUBO DE VISTAS
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(windowPos.x, windowPos.y, viewportSize.x, viewportSize.y);

    glm::mat4 viewMat = App->camera->GetViewMatrix();
    glm::mat4 projMat = App->camera->GetProjectionMatrix();

    if (App->scene->selected != nullptr) {
        ComponentTransform* transform = App->scene->selected->GetComponent<ComponentTransform>(ComponentType::TRANSFORM);
        if (transform != nullptr) {

            if (!ImGui::GetIO().WantTextInput) {
                if (ImGui::IsKeyPressed(ImGuiKey_W)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
                if (ImGui::IsKeyPressed(ImGuiKey_E)) mCurrentGizmoOperation = ImGuizmo::ROTATE;
                if (ImGui::IsKeyPressed(ImGuiKey_R)) mCurrentGizmoOperation = ImGuizmo::SCALE;
            }

            glm::mat4 globalMat = transform->GetGlobalMatrix();

            // Si mantienes pulsado CTRL, hace "Snap" (va a saltos exactos de 1 unidad o 15 grados)
            bool useSnap = ImGui::GetIO().KeyCtrl;
            float snapValues[3] = { 1.0f, 1.0f, 1.0f };
            if (mCurrentGizmoOperation == ImGuizmo::ROTATE) snapValues[0] = snapValues[1] = snapValues[2] = 15.0f;
            else if (mCurrentGizmoOperation == ImGuizmo::SCALE) snapValues[0] = snapValues[1] = snapValues[2] = 0.5f;

            ImGuizmo::Manipulate(glm::value_ptr(viewMat), glm::value_ptr(projMat),
                mCurrentGizmoOperation, mCurrentGizmoMode, glm::value_ptr(globalMat),
                nullptr, useSnap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing()) {
                // Justo en el primer frame en que agarramos la flecha del Gizmo, guardamos foto para Ctrl+Z
                if (!g_WasUsingGizmo) {
                    PushUndo(App->scene->selected);
                }
                transform->SetGlobalMatrix(globalMat);
            }
            g_WasUsingGizmo = ImGuizmo::IsUsing();
        }
    }

    // --- CUBO INTERACTIVO 3D ARRIBA A LA DERECHA (ViewManipulate) ---
    ImVec2 viewManipulatePos = ImVec2(windowPos.x + viewportSize.x - 110.0f, windowPos.y + 10.0f);
    glm::mat4 oldView = viewMat;
    ImGuizmo::ViewManipulate(glm::value_ptr(viewMat), 10.0f, viewManipulatePos, ImVec2(100.0f, 100.0f), 0x10101010);
    if (viewMat != oldView) {
        // Si el usuario ha girado el cubo de la esquina, actualizamos la cámara
        glm::mat4 invView = glm::inverse(viewMat);
        App->camera->Position = glm::vec3(invView[3]);
        glm::vec3 front = -glm::normalize(glm::vec3(invView[2]));
        App->camera->pitch = glm::degrees(std::asin(glm::clamp(front.y, -1.0f, 1.0f)));
        App->camera->yaw = glm::degrees(std::atan2(front.z, front.x));
    }

    // --- BARRA DE HERRAMIENTAS FLOTANTE ARRIBA A LA IZQUIERDA ---
    ImGui::SetCursorScreenPos(ImVec2(windowPos.x + 10.0f, windowPos.y + 10.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 0.85f));
    if (ImGui::Button("Move (W)"))   mCurrentGizmoOperation = ImGuizmo::TRANSLATE; ImGui::SameLine();
    if (ImGui::Button("Rotate (E)")) mCurrentGizmoOperation = ImGuizmo::ROTATE;    ImGui::SameLine();
    if (ImGui::Button("Scale (R)"))  mCurrentGizmoOperation = ImGuizmo::SCALE;     ImGui::SameLine();
    if (ImGui::Button(mCurrentGizmoMode == ImGuizmo::LOCAL ? "Mode: Local" : "Mode: World")) {
        mCurrentGizmoMode = (mCurrentGizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    }
    ImGui::SameLine();
    if (ImGui::Button(g_MaximizeViewport ? "Restore (Space)" : "Maximize (Space)")) {
        g_MaximizeViewport = !g_MaximizeViewport;
    }
    ImGui::PopStyleColor();
    bool isToolbarHovered = ImGui::IsItemHovered() || ImGui::IsAnyItemActive();

    App->camera->isUsingGizmo = ImGuizmo::IsUsing();

    // 2. MOUSE PICKING (Solo si no tocamos el Gizmo ni la barra de botones)
    if (ImGui::IsWindowHovered() && !isToolbarHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) {

        ImVec2 mousePos = ImGui::GetMousePos();
        float mouseX = mousePos.x - windowPos.x;
        float mouseY = mousePos.y - windowPos.y;

        if (mouseX >= 0 && mouseY >= 0 && mouseX <= viewportSize.x && mouseY <= viewportSize.y) {

            glm::vec3 rayDir = App->camera->ScreenToWorldRay(mouseX, mouseY, viewportSize.x, viewportSize.y);
            glm::vec3 rayOrigin = App->camera->Position;

            // Guardamos el rayo para dibujarlo en 3D (Debug Raycast)
            App->renderer3D->lastRayOrigin = rayOrigin;
            App->renderer3D->lastRayEnd = rayOrigin + rayDir * 500.0f;
            App->renderer3D->hasLastRay = true;

            GameObject* closestObj = nullptr;
            float closestDist = FLT_MAX;

            auto checkIntersections = [&](auto& self, GameObject* go) -> void {
                if (!go->active) return;

                if (go->GetComponent<ComponentMesh>(ComponentType::MESH) != nullptr) {
                    float dist;
                    if (go->RayAABBIntersection(rayOrigin, rayDir, dist)) {
                        if (dist < closestDist) {
                            closestDist = dist;
                            closestObj = go;
                        }
                    }
                }
                for (GameObject* child : go->children) self(self, child);
                };

            checkIntersections(checkIntersections, App->scene->GetRoot());

            App->scene->selected = closestObj;
            if (closestObj) {
                App->renderer3D->showAABB = true;
                LOG("Picked GameObject: %s", closestObj->name.c_str());
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
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

static GameObject* objectToDuplicate = nullptr;
static GameObject* objectToDelete = nullptr;
static GameObject* objectToAddChild = nullptr;

void ModuleEditor::DrawHierarchy() {
    PlaceWindow(0.00f, 0.00f, 0.20f, 0.70f);
    ImGui::Begin("Hierarchy", &showHierarchy);

    if (App->scene != nullptr && App->scene->GetRoot() != nullptr) {
        DrawHierarchyNode(App->scene->GetRoot());
    }

    // Un espacio invisible debajo de los nodos para poder soltar cosas "fuera" de todo
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_NODE")) {
            GameObject* draggedObj = *(GameObject**)payload->Data;
            if (draggedObj != nullptr && App->scene != nullptr) {
                draggedObj->SetParent(App->scene->GetRoot());
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();

    if (objectToAddChild != nullptr) {
        GameObject* child = new GameObject("Empty Child", objectToAddChild);
        child->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);
        App->scene->selected = child;
        LOG("Created Empty Child inside: %s", objectToAddChild->name.c_str());
        objectToAddChild = nullptr;
    }

    if (objectToDuplicate != nullptr) {
        GameObject* clonedObj = objectToDuplicate->Clone();
        App->scene->selected = clonedObj;
        LOG("GameObject duplicated: %s", clonedObj->name.c_str());
        objectToDuplicate = nullptr;
    }

    if (objectToDelete != nullptr) {
        App->scene->DestroyGameObject(objectToDelete);
        objectToDelete = nullptr;
    }
}

void ModuleEditor::DrawHierarchyNode(GameObject* go) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

    if (App->scene->selected == go) flags |= ImGuiTreeNodeFlags_Selected;

    const bool isLeaf = go->children.empty();
    if (isLeaf) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (!isLeaf && App->scene->selected != nullptr && App->scene->selected != go) {
        GameObject* parentWalker = App->scene->selected->parent;
        while (parentWalker != nullptr) {
            if (parentWalker == go) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                break;
            }
            parentWalker = parentWalker->parent;
        }
    }

    bool nodeOpen = ImGui::TreeNodeEx(go, flags, "%s", go->name.c_str());

    // Doble clic para renombrar
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        renamingObject = go;
        snprintf(renameBuffer, sizeof(renameBuffer), "%s", go->name.c_str());
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        App->scene->selected = go;
    }

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("GAMEOBJECT_NODE", &go, sizeof(GameObject*));
        ImGui::Text("Mover %s", go->name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT_NODE")) {
            GameObject* draggedObj = *(GameObject**)payload->Data;
            if (draggedObj != nullptr && draggedObj != go) {
                draggedObj->SetParent(go);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Click Derecho -> PopUp Menú Completo
    if (ImGui::BeginPopupContextItem()) {
        App->scene->selected = go;
        if (ImGui::MenuItem("Create Empty Child")) {
            objectToAddChild = go;
        }
        if (ImGui::MenuItem("Rename", "F2")) {
            renamingObject = go;
            snprintf(renameBuffer, sizeof(renameBuffer), "%s", go->name.c_str());
        }
        if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
            objectToDuplicate = go;
        }
        if (ImGui::MenuItem("Delete", "Del")) {
            objectToDelete = go;
        }

        // Mover Arriba / Abajo en la lista de hijos (Pág. 132 de teoría)
        if (go->parent != nullptr) {
            ImGui::Separator();
            auto& siblings = go->parent->children;
            auto it = std::find(siblings.begin(), siblings.end(), go);
            if (it != siblings.end()) {
                size_t idx = std::distance(siblings.begin(), it);
                if (ImGui::MenuItem("Move Up", nullptr, false, idx > 0)) {
                    std::swap(siblings[idx], siblings[idx - 1]);
                }
                if (ImGui::MenuItem("Move Down", nullptr, false, idx + 1 < siblings.size())) {
                    std::swap(siblings[idx], siblings[idx + 1]);
                }
            }
        }
        ImGui::EndPopup();
    }

    // Cajetín de Input de Texto JUSTO DEBAJO del nodo (antes de los hijos)
    if (renamingObject == go) {
        ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 10.0f);
        if (ImGui::InputText("##RenameInput", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            if (strlen(renameBuffer) > 0) {
                go->name = renameBuffer;
            }
            renamingObject = nullptr;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) || (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered())) {
            renamingObject = nullptr;
        }
    }

    if (nodeOpen && !isLeaf) {
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
                // Guardamos la foto de cómo estaba ANTES de que los sliders lo modifiquen
                UndoState beforeEdit = { selectedGameObject, transform->position, transform->eulerRotation, transform->rotation, transform->scale };
                bool startedEditing = false;

                ImGui::DragFloat3("Position", &transform->position[0], 0.1f);
                startedEditing |= ImGui::IsItemActivated();

                if (ImGui::DragFloat3("Rotation", &transform->eulerRotation[0], 0.5f)) {
                    transform->UpdateRotationFromEuler();
                }
                startedEditing |= ImGui::IsItemActivated();

                ImGui::DragFloat3("Scale", &transform->scale[0], 0.1f);
                startedEditing |= ImGui::IsItemActivated();

                // Si hemos empezado a arrastrar o escribir en CUALQUIERA de los 3, guardamos para Ctrl+Z
                if (startedEditing) {
                    if (g_UndoStack.size() > 50) g_UndoStack.erase(g_UndoStack.begin());
                    g_UndoStack.push_back(beforeEdit);
                }
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
        ImGui::Checkbox("Mouse Picking Ray", &App->renderer3D->showRay);
        ImGui::Checkbox("Selection Outline (Stencil)", &App->renderer3D->showOutline);

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