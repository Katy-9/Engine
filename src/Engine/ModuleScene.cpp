#include "ModuleScene.h"
#include "Application.h"
#include "ModuleImporter.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "Log.h"
#include "ModuleCamera3D.h"
#include <string>
#include <SDL3/SDL.h>

ModuleScene::ModuleScene(Application* app) : Module(app) {}

ModuleScene::~ModuleScene() {}

bool ModuleScene::Init() {
    LOG("Initializing Scene");
    root = new GameObject("Scene Root");

    const char* base = SDL_GetBasePath();
    const std::string basePath = base ? base : "";

    GameObject* street = App->importer->LoadFBX(basePath + "Assets/Resources/Street environment_V01.fbx");

    // Encuadramos toda la escena al arrancar; sin selección para que no salga un contorno gigante
    if (street != nullptr) {
        selected = street;
        App->camera->Focus();
        selected = nullptr;
    }
    return true;
}

update_status ModuleScene::Update(float dt) {
    if (root) {
        root->Update(dt);
    }
    return UPDATE_CONTINUE;
}

bool ModuleScene::CleanUp() {
    LOG("Destroying Scene");
    if (root) {
        delete root;
    }
    return true;
}

void ModuleScene::DestroyGameObject(GameObject* go) {
    if (go == nullptr || go == root) return; 
    if (selected == go) {
        selected = nullptr;
    }

    if (go->parent != nullptr) {
        auto& children = go->parent->children;
        for (auto it = children.begin(); it != children.end(); ++it) {
            if (*it == go) {
                children.erase(it);
                break;
            }
        }
    }

    delete go;
}