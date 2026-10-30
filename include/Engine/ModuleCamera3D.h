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

    // Estas matrices son las que enviaremos a la tarjeta gráfica luego
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

public:
    glm::vec3 Position;
    glm::vec3 Reference; // Punto al que miramos cuando orbitamos

    // Vectores direccionales de la cámara
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;

    float yaw;
    float pitch;
};