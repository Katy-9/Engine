#include "ModuleInput.h"
#include "Log.h"
#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>

ModuleInput::ModuleInput(Application* app) : Module(app) {}
ModuleInput::~ModuleInput() {}

bool ModuleInput::Init() {
    LOG("Initializing SDL Input");
    return true;
}

update_status ModuleInput::PreUpdate(float dt) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
            return UPDATE_STOP;
        }
    }
    return UPDATE_CONTINUE;
}

bool ModuleInput::CleanUp() {
    LOG("Destroying SDL Input");
    return true;
}