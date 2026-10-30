#include "ModuleWindow.h"
#include "Log.h"

ModuleWindow::ModuleWindow(Application* app) : Module(app) {}
ModuleWindow::~ModuleWindow() {}

bool ModuleWindow::Init() {
    LOG("Initializing SDL window");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG("SDL_VIDEO could not initialize! SDL_Error: %s", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Banderas para SDL3
    SDL_WindowFlags flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // SDL3 CreateWindow ya no pide posición X/Y
    window = SDL_CreateWindow("Kuroko Engine", width, height, flags);

    if (window == nullptr) {
        LOG("Window could not be created! SDL_Error: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool ModuleWindow::CleanUp() {
    LOG("Destroying SDL window");
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}

update_status ModuleWindow::PreUpdate(float dt) {
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    if (w > 0 && h > 0) {      // minimizada = 0x0: ignoramos para no dividir entre 0
        width = w;
        height = h;
    }
    return UPDATE_CONTINUE;
}