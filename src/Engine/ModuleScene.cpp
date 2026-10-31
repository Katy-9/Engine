#include "ModuleScene.h"
#include "Application.h"
#include "ModuleImporter.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "Log.h"
#include <string>
#include <SDL3/SDL.h>

ModuleScene::ModuleScene(Application* app) : Module(app) {}

ModuleScene::~ModuleScene() {}

bool ModuleScene::Init() {
    LOG("Initializing Scene");
    root = new GameObject("Scene Root");

    const char* base = SDL_GetBasePath();
    const std::string basePath = base ? base : "";

    App->importer->LoadFBX(basePath + "Assets/baker_house.fbx");

    // La cámara
    GameObject* mainCam = new GameObject("Main Camera", root);
    mainCam->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);

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