#pragma once

// Tipos de componentes que nos pide el profesor
enum class ComponentType {
    TRANSFORM,
    MESH,
    MATERIAL,
    CAMERA
};

class GameObject; // Declaración anticipada

class Component {
public:
    Component(GameObject* owner, ComponentType type) : owner(owner), type(type), active(true) {}
    virtual ~Component() {}

    virtual void Enable() { active = true; }
    virtual void Disable() { active = false; }
    virtual void Update(float dt) {}

    ComponentType GetType() const { return type; }
    bool IsActive() const { return active; }

protected:
    GameObject* owner;
    ComponentType type;
    bool active;
};