#pragma once
#include "Module.h"

class ModuleInput : public Module {
public:
    ModuleInput(Application* app);
    virtual ~ModuleInput();

    bool Init() override;
    update_status PreUpdate(float dt) override;
    bool CleanUp() override;

public:
    float mouse_wheel = 0.0f; // Guardará el movimiento de la rueda del ratón
};