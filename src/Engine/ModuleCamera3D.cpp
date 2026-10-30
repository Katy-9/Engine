#include "ModuleCamera3D.h"
#include "Application.h"
#include "ModuleInput.h"
#include "ModuleWindow.h"
#include <SDL3/SDL.h>
#include "Log.h"

ModuleCamera3D::ModuleCamera3D(Application* app) : Module(app) {
    Position = glm::vec3(0.0f, 20.0f, 50.0f); // Empezamos más lejos
    Reference = glm::vec3(0.0f, 0.0f, 0.0f);

    yaw = -90.0f;
    pitch = -15.0f;
}

ModuleCamera3D::~ModuleCamera3D() {}

bool ModuleCamera3D::Init() { return true; }

update_status ModuleCamera3D::Update(float dt) {
    float moveSpeed = 20.0f * dt; // Un poco más rápido por defecto
    float turnSpeed = 0.2f;

    const bool* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_LSHIFT]) moveSpeed *= 3.0f;

    float mouseX, mouseY;
    SDL_MouseButtonFlags mouseFlags = SDL_GetRelativeMouseState(&mouseX, &mouseY);
    bool rightClick = (mouseFlags & SDL_BUTTON_RMASK) != 0;
    bool altPressed = keys[SDL_SCANCODE_LALT];

    if (rightClick && !altPressed) {
        yaw += mouseX * turnSpeed;
        pitch -= mouseY * turnSpeed;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        if (keys[SDL_SCANCODE_W]) Position += Front * moveSpeed;
        if (keys[SDL_SCANCODE_S]) Position -= Front * moveSpeed;
        if (keys[SDL_SCANCODE_A]) Position -= Right * moveSpeed;
        if (keys[SDL_SCANCODE_D]) Position += Right * moveSpeed;
    }

    // --- CENTRAR CÁMARA (TECLA F) ---
    // Nos teletransportamos muy lejos para asegurarnos de ver modelos grandes
    if (keys[SDL_SCANCODE_F]) {
        Position = glm::vec3(0.0f, 100.0f, 200.0f);
        yaw = -90.0f;
        pitch = -25.0f;
    }

    if (App->input->mouse_wheel != 0) {
        Position += Front * App->input->mouse_wheel * moveSpeed * 5.0f;
    }

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    Front = glm::normalize(direction);
    Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    Up = glm::normalize(glm::cross(Right, Front));

    return UPDATE_CONTINUE;
}

glm::mat4 ModuleCamera3D::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 ModuleCamera3D::GetProjectionMatrix() const {
    float aspect = (float)App->window->width / (float)App->window->height;
    // --- LÍMITE DE VISIÓN (Far Plane) AUMENTADO A 10000 ---
    return glm::perspective(glm::radians(60.0f), aspect, 0.1f, 10000.0f);
}