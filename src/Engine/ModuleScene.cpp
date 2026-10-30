#include "ModuleScene.h"
#include "Application.h"
#include "ModuleImporter.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "Log.h"

ModuleScene::ModuleScene(Application* app) : Module(app) {}

ModuleScene::~ModuleScene() {}

bool ModuleScene::Init() {
    LOG("Initializing Scene");
    root = new GameObject("Scene Root");

    App->importer->LoadFBX("Assets/baker_house.fbx");

    // Buscamos la casa que acabamos de importar y la ponemos de pie
    for (GameObject* child : root->children) {
        if (child->name == "FBX_Model") {
            ComponentTransform* transform = child->GetComponent<ComponentTransform>(ComponentType::TRANSFORM);
            if (transform) {
                transform->eulerRotation = glm::vec3(-90.0f, 0.0f, 0.0f);
                transform->UpdateRotationFromEuler();
            }
        }
    }

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
        delete root; // Esto borrará automáticamente a todos sus hijos
    }
    return true;
}