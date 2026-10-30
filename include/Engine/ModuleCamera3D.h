#pragma once
#include "Module.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ModuleCamera3D : public Module {
public:
    ModuleCamera3D(Application* app);
    virtual ~ModuleCamera3D();

    bool Init() override;
    update_status Update(float dt) override;

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

public:
    glm::vec3 Position;
    glm::vec3 Reference;   // punto que miramos al orbitar o al hacer focus

    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;

    float yaw;
    float pitch;

    // Ajustables desde la ventana de configuración
    float baseSpeed = 20.0f;
    float turnSpeed = 0.2f;
    float fov = 60.0f;

private:
    void UpdateVectors();
    void Orbit(float dx, float dy, bool start);
    void Focus();

    float orbitDistance = 10.0f;
    bool wasOrbiting = false;
    bool fWasDown = false;
};