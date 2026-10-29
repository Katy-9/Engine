#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRenderer3D.h"
#include "ModuleInput.h"
#include "ModuleEditor.h"
#include "Log.h"
#include <SDL3/SDL.h>

Application::Application() {
    input = new ModuleInput(this);
    window = new ModuleWindow(this);
    renderer3D = new ModuleRenderer3D(this);
    editor = new ModuleEditor(this);

    // El orden de ejecución es crítico
    modules.push_back(input);
    modules.push_back(window);
    modules.push_back(renderer3D);
    modules.push_back(editor);
}

Application::~Application() {
    for (Module* mod : modules) {
        delete mod;
    }
}

bool Application::Init() {
    bool ret = true;
    for (Module* mod : modules) {
        ret = mod->Init();
        if (!ret) return false;
    }
    return ret;
}

update_status Application::Update() {
    update_status ret = UPDATE_CONTINUE;

    // Calcular Delta Time (dt)
    uint64_t currentTime = SDL_GetPerformanceCounter();
    dt = (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
    lastTime = currentTime;

    for (auto* mod : modules) {
        if (ret == UPDATE_CONTINUE) ret = mod->PreUpdate(dt);
    }
    for (auto* mod : modules) {
        if (ret == UPDATE_CONTINUE) ret = mod->Update(dt);
    }
    for (auto* mod : modules) {
        if (ret == UPDATE_CONTINUE) ret = mod->PostUpdate(dt);
    }

    SDL_GL_SwapWindow(window->window);

    return ret;
}

bool Application::CleanUp() {
    bool ret = true;
    for (auto it = modules.rbegin(); it != modules.rend(); ++it) {
        if (!(*it)->CleanUp()) ret = false;
    }
    return ret;
}