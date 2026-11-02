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

    // AABB en coordenadas de mundo de este objeto y todos sus hijos. Devuelve false si no hay ninguna malla.
    bool GetWorldAABB(glm::vec3& outMin, glm::vec3& outMax);

    // --- TEMPLATES DE COMPONENTES ---

    // Crea un componente del tipo que le pidamos, le pasa este GameObject como dueño, y lo guarda
    template<typename T>
    T* CreateComponent(ComponentType type) {
        T* comp = new T(this);
        components.push_back(comp);
        return comp;
    }

    // Busca un componente por su tipo y nos lo devuelve. Si no lo tiene, devuelve nullptr
    template<typename T>
    T* GetComponent(ComponentType type) {
        for (auto* comp : components) {
            if (comp->GetType() == type) {
                return static_cast<T*>(comp); // Convierte el puntero genérico al tipo correcto
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