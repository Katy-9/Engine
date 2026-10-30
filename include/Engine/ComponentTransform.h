#pragma once
#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class ComponentTransform : public Component {
public:
    ComponentTransform(GameObject* owner);
    virtual ~ComponentTransform();

    // Actualiza el Quaternion interno basándose en los ángulos Euler de la UI
    void UpdateRotationFromEuler();

    // Calcula y devuelve la matriz 4x4 final que se enviará al Shader de OpenGL
    glm::mat4 GetGlobalMatrix() const;

public:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 eulerRotation; // ImGui mostrará esto (grados en lugar de Quaternion)
    glm::vec3 scale;
};