#include "GameObject.h"

GameObject::GameObject(std::string name, GameObject* parent) : name(name), parent(parent) {
    // Si me asignan un padre al nacer, me añado a su lista de hijos automáticamente
    if (parent != nullptr) {
        parent->children.push_back(this);
    }
}

GameObject::~GameObject() {
    // Limpiar memoria
    for (auto* comp : components) {
        delete comp;
    }
    for (auto* child : children) {
        delete child;
    }
}

void GameObject::Update(float dt) {
    if (!active) return; // Si el objeto está desactivado en el editor, no hace nada

    // Actualizar todos los componentes
    for (auto* comp : components) {
        if (comp->IsActive()) {
            comp->Update(dt);
        }
    }

    // Actualizar todos los hijos en cascada
    for (auto* child : children) {
        child->Update(dt);
    }
}

void GameObject::AddComponent(Component* component) {
    components.push_back(component);
}