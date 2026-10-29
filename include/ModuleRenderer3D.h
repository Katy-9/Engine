#pragma once
#include "Module.h"
#include <SDL3/SDL.h>

class ModuleRenderer3D : public Module {
public:
    ModuleRenderer3D(Application* app);
    virtual ~ModuleRenderer3D();

    bool Init() override;
    update_status PreUpdate(float dt) override;
    update_status PostUpdate(float dt) override;
    bool CleanUp() override;

public:
    SDL_GLContext context;
};