#pragma once
#include "Module.h"
#include <SDL3/SDL.h>
#include <functional>
#include <vector>

using SDLEventListener = std::function<void(const SDL_Event&)>;

class ModuleInput : public Module {
public:
    ModuleInput(Application* app);
    virtual ~ModuleInput();

    bool Init() override;
    update_status PreUpdate(float dt) override;
    bool CleanUp() override;

    void AddEventListener(SDLEventListener listener) { listeners.push_back(std::move(listener)); }

public:
    float mouse_wheel = 0.0f;
    bool uiCapturesMouse = false;
    bool uiCapturesKeyboard = false;

private:
    std::vector<SDLEventListener> listeners;
};