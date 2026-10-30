#include "GameObject.h"

GameObject::GameObject(std::string name, GameObject* parent) : name(name), parent(parent) {
    if (parent != nullptr) {
        parent->children.push_back(this);
    }
}

GameObject::~GameObject() {
    for (auto* comp : components) {
        delete comp;
    }
    for (auto* child : children) {
        delete child;
    }
}

void GameObject::Update(float dt) {
    if (!active) return;

    for (auto* comp : components) {
        if (comp->IsActive()) {
            comp->Update(dt);
        }
    }

    for (auto* child : children) {
        child->Update(dt);
    }
}