#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRenderer3D.h"
#include "ModuleInput.h"
#include "ModuleEditor.h"
#include "ModuleScene.h" // <--- Este include faltaba y causaba el primer error
#include "ModuleCamera3D.h"
#include "ModuleImporter.h"
#include "Log.h"
#include <SDL3/SDL.h>

Application::Application() {
    input = new ModuleInput(this);
    window = new ModuleWindow(this);
    renderer3D = new ModuleRenderer3D(this);
    importer = new ModuleImporter(this);
	camera = new ModuleCamera3D(this); // <--- Inicializamos la cámara
    scene = new ModuleScene(this);
    editor = new ModuleEditor(this);

    modules.push_back(input);
    modules.push_back(window);
    modules.push_back(renderer3D);
    modules.push_back(importer);
	modules.push_back(camera); // <--- Agregamos la cámara a la lista de módulos
    modules.push_back(scene);
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