#pragma once
#include "Module.h"
#include <SDL3/SDL.h>

class ModuleWindow : public Module {
public:
    ModuleWindow(Application* app);
    virtual ~ModuleWindow();

    bool Init() override;
    bool CleanUp() override;

    update_status PreUpdate(float dt) override;

public:
    SDL_Window* window = nullptr;
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    void SetFullscreen(bool enable);
};