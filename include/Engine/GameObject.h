#pragma once
#include <vector>
#include <string>
#include "Component.h"

class GameObject {
public:
    // Un GameObject siempre tiene un nombre y puede tener un padre (para la jerarquía)
    GameObject(std::string name, GameObject* parent = nullptr);
    ~GameObject();

    void Update(float dt);

    // Funciones clave para añadir componentes
    void AddComponent(Component* component);

public:
    std::string name;
    bool active = true;

    // Jerarquía (Árbol)
    GameObject* parent;
    std::vector<GameObject*> children;

    // Sus componentes (Transform, Mesh, Material...)
    std::vector<Component*> components;
};