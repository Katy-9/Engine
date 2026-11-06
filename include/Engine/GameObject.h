#pragma once
#include <vector>
#include <string>
#include "Component.h"
#include <glm/glm.hpp>

class GameObject {
public:
    GameObject(std::string name, GameObject* parent = nullptr);
    ~GameObject();
    GameObject* Clone(GameObject* newParent = nullptr);
    void Update(float dt);
    void SetParent(GameObject* newParent);
    bool GetWorldAABB(glm::vec3& outMin, glm::vec3& outMax);
    bool RayAABBIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& outDist);

    template<typename T>
    T* CreateComponent(ComponentType type) {
        T* comp = new T(this);
        components.push_back(comp);
        return comp;
    }

    template<typename T>
    T* GetComponent(ComponentType type) {
        for (auto* comp : components) {
            if (comp->GetType() == type) {
                return static_cast<T*>(comp);
            }
        }
        return nullptr;
    }

public:
    std::string name;
    bool active = true;

    // Jerarquía
    GameObject* parent;
    std::vector<GameObject*> children;

    // Componentes
    std::vector<Component*> components;
};