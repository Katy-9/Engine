#include "ModuleCamera3D.h"
#include "Application.h"
#include "ModuleInput.h"
#include "ModuleWindow.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include <SDL3/SDL.h>
#include <cmath>
#include "Log.h"

ModuleCamera3D::ModuleCamera3D(Application* app) : Module(app) {
    Position = glm::vec3(0.0f, 20.0f, 50.0f);
    Reference = glm::vec3(0.0f, 0.0f, 0.0f);

    yaw = -90.0f;
    pitch = -15.0f;
    UpdateVectors();
}

ModuleCamera3D::~ModuleCamera3D() {}

bool ModuleCamera3D::Init() { return true; }

update_status ModuleCamera3D::Update(float dt) {
    const bool* keys = SDL_GetKeyboardState(nullptr);

    float mouseX, mouseY;
    const SDL_MouseButtonFlags mouseFlags = SDL_GetRelativeMouseState(&mouseX, &mouseY);
    const bool leftClick = (mouseFlags & SDL_BUTTON_LMASK) != 0;
    const bool rightClick = (mouseFlags & SDL_BUTTON_RMASK) != 0;
    const bool middleClick = (mouseFlags & SDL_BUTTON_MMASK) != 0;
    const bool alt = keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT];

    const float boost = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) ? 2.0f : 1.0f;
    const float moveSpeed = baseSpeed * boost * dt;
    const bool mouseFree = isViewportHovered || rightClick;

    const bool orbiting = alt && leftClick && mouseFree;

    if (orbiting) {
        // Alt + clic izquierdo: orbitar alrededor del objeto seleccionado
        Orbit(mouseX * turnSpeed, mouseY * turnSpeed, !wasOrbiting);
    }
    else if (rightClick && mouseFree) {
        // Clic derecho: free look + WASD
        yaw += mouseX * turnSpeed;
        pitch = glm::clamp(pitch - mouseY * turnSpeed, -89.0f, 89.0f);
        UpdateVectors();

        if (keys[SDL_SCANCODE_W]) Position += Front * moveSpeed;
        if (keys[SDL_SCANCODE_S]) Position -= Front * moveSpeed;
        if (keys[SDL_SCANCODE_A]) Position -= Right * moveSpeed;
        if (keys[SDL_SCANCODE_D]) Position += Right * moveSpeed;
    }
    else if (middleClick && mouseFree) {
        // Clic central: paneo (trasladar sobre Right y Up)
        Position += (-Right * mouseX + Up * mouseY) * 0.05f * boost;
    }
    wasOrbiting = orbiting;

    // Rueda: zoom. Es un evento, no depende de dt; el paso escala con la distancia
    if (mouseFree && App->input->mouse_wheel != 0.0f) {
        const float step = glm::max(glm::length(Position - Reference) * 0.1f, 0.5f);
        Position += Front * App->input->mouse_wheel * step * boost;
    }

    // F: centrar en el objeto seleccionado (solo al pulsar, no mientras se mantiene)
    const bool fDown = keys[SDL_SCANCODE_F];
    if (fDown && !fWasDown && !App->input->uiCapturesKeyboard) Focus();
    fWasDown = fDown;

    UpdateVectors();
    return UPDATE_CONTINUE;
}

void ModuleCamera3D::UpdateVectors() {
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    Front = glm::normalize(direction);
    Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    Up = glm::normalize(glm::cross(Right, Front));
}

void ModuleCamera3D::Orbit(float dx, float dy, bool start) {
    if (start) {
        // Al empezar el gesto fijamos el pivote: el centro del seleccionado, o un punto delante de la cámara
        glm::vec3 mn, mx;
        GameObject* selected = App->scene->selected;
        if (selected && selected->GetWorldAABB(mn, mx)) Reference = (mn + mx) * 0.5f;
        else Reference = Position + Front * 10.0f;

        // Giramos hacia el pivote sin mover la cámara, para que no salte
        const glm::vec3 toPivot = Reference - Position;
        orbitDistance = glm::max(glm::length(toPivot), 0.5f);
        if (glm::length(toPivot) > 1e-4f) {
            const glm::vec3 d = glm::normalize(toPivot);
            yaw = glm::degrees(std::atan2(d.z, d.x));
            pitch = glm::degrees(std::asin(d.y));
        }
    }

    yaw += dx;
    pitch = glm::clamp(pitch - dy, -89.0f, 89.0f);
    UpdateVectors();
    Position = Reference - Front * orbitDistance;   // misma distancia, mirando siempre al pivote
}

void ModuleCamera3D::Focus() {
    glm::vec3 mn(-1.0f), mx(1.0f);   // sin selección: encuadramos el origen
    GameObject* selected = App->scene->selected;
    if (selected) selected->GetWorldAABB(mn, mx);

    Reference = (mn + mx) * 0.5f;
    const float radius = glm::max(glm::length(mx - mn) * 0.5f, 0.5f);
    orbitDistance = radius / std::sin(glm::radians(fov) * 0.5f) * 1.2f;   // 1.2 = un poco de margen
    Position = Reference - Front * orbitDistance;   // conserva la dirección actual de mirada
}

glm::mat4 ModuleCamera3D::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 ModuleCamera3D::GetProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 10000.0f);
}