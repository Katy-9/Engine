#include "ModuleInput.h"
#include "Application.h"
#include "ModuleImporter.h"
#include "Log.h"
#include <SDL3/SDL.h>
#include <string>
#include "ModuleScene.h"

ModuleInput::ModuleInput(Application* app) : Module(app) {}
ModuleInput::~ModuleInput() {}

bool ModuleInput::Init() {
    LOG("Initializing SDL Input");
    return true;
}

update_status ModuleInput::PreUpdate(float dt) {
    mouse_wheel = 0.0f; // Reseteamos la rueda del ratón

    SDL_Event event;
    while (SDL_PollEvent(&event)) {

        for (const SDLEventListener& l : listeners) l(event);

        if (event.type == SDL_EVENT_QUIT) {
            return UPDATE_STOP;
        }

        if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            mouse_wheel += event.wheel.y;   // acumula si llegan varios eventos en un frame
        }

        // --- DRAG AND DROP ---
        if (event.type == SDL_EVENT_DROP_FILE) {
            std::string filePath = event.drop.data;
            std::string extension = filePath.substr(filePath.find_last_of(".") + 1);
            for (auto& c : extension) c = tolower(c);

            if (extension == "fbx") {
                LOG("FBX file dropped! Loading...");
                App->importer->LoadFBX(filePath);
            }
            else if (extension == "png" || extension == "dds" || extension == "tga") {
                LOG("Texture dropped! File: %s", filePath.c_str());

                App->importer->LoadTexture(filePath, App->scene->selected);
            }
        }
    }
    return UPDATE_CONTINUE;
}

bool ModuleInput::CleanUp() {
    LOG("Destroying SDL Input");
    return true;
}