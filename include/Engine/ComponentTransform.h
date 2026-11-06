#pragma once
#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class ComponentTransform : public Component {
public:
    ComponentTransform(GameObject* owner);
    virtual ~ComponentTransform();

    void UpdateRotationFromEuler();

    glm::mat4 GetGlobalMatrix() const;

    void SetGlobalMatrix(const glm::mat4& globalMatrix);

public:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 eulerRotation; // ImGui mostrará esto (grados en lugar de Quaternion)
    glm::vec3 scale;
};