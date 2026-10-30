#pragma once
#include "Module.h"

class GameObject;

class ModuleScene : public Module {
public:
    ModuleScene(Application* app);
    virtual ~ModuleScene();

    bool Init() override;
    update_status Update(float dt) override;
    bool CleanUp() override;

    GameObject* GetRoot() const { return root; }

    GameObject* selected = nullptr;   // GameObject seleccionado (lo escribe el editor, lo leen camara e input)

private:
    GameObject* root = nullptr;
};