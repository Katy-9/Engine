#pragma once
#include "Component.h"

class ResourceTexture;

class ComponentMaterial : public Component {
public:
    ComponentMaterial(GameObject* owner) : Component(owner, ComponentType::MATERIAL) {}
    virtual ~ComponentMaterial();

    // Cambia la textura que usa este componente, ajustando los contadores de referencias
    void SetTexture(ResourceTexture* newTexture);
    ResourceTexture* GetTexture() const { return texture; }
    unsigned int GetTextureID() const;   // 0 si no hay textura

public:
    // Color Picker (R, G, B, A)
    float colorTint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool useCheckerTexture = false;

private:
    ResourceTexture* texture = nullptr;
};