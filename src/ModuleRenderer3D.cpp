#include "ModuleRenderer3D.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "Log.h"
#include <glad/glad.h>

ModuleRenderer3D::ModuleRenderer3D(Application* app) : Module(app) {}
ModuleRenderer3D::~ModuleRenderer3D() {}

bool ModuleRenderer3D::Init() {
    LOG("Creating 3D Renderer context");

    context = SDL_GL_CreateContext(App->window->window);
    if (!context) {
        LOG("OpenGL context could not be created! SDL Error: %s", SDL_GetError());
        return false;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        LOG("Failed to initialize GLAD");
        return false;
    }

    LOG("Vendor: %s", glGetString(GL_VENDOR));
    LOG("Renderer: %s", glGetString(GL_RENDERER));
    LOG("OpenGL version supported: %s", glGetString(GL_VERSION));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    return true;
}

update_status ModuleRenderer3D::PreUpdate(float dt) {
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return UPDATE_CONTINUE;
}

update_status ModuleRenderer3D::PostUpdate(float dt) {
    return UPDATE_CONTINUE;
}

bool ModuleRenderer3D::CleanUp() {
    LOG("Destroying 3D Renderer");
    SDL_GL_DestroyContext(context); // SDL3 usa DestroyContext
    return true;
}