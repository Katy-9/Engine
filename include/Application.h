#pragma once
#include <vector>
#include <cstdint> // Necesario para uint64_t
#include "Globals.h"

class Module;
class ModuleWindow;
class ModuleRenderer3D;
class ModuleInput;
class ModuleEditor;

class Application {
public:
    Application();
    ~Application();

    bool Init();
    update_status Update();
    bool CleanUp();

public:
    ModuleWindow* window = nullptr;
    ModuleRenderer3D* renderer3D = nullptr;
    ModuleInput* input = nullptr;
    ModuleEditor* editor = nullptr;

private:
    std::vector<Module*> modules;
    uint64_t lastTime = 0;
    float dt = 0.0f;
};