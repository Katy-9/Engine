#pragma once
#include "Component.h"
#include <string>

class ComponentMaterial : public Component {
public:
    ComponentMaterial(GameObject* owner) : Component(owner, ComponentType::MATERIAL) {}
    virtual ~ComponentMaterial() {}

public:
    std::string texturePath = "None";
    int textureWidth = 0;
    int textureHeight = 0;

    unsigned int textureID = 0; // ID de la textura en VRAM que genera OpenGL

    // Color Picker (R, G, B, A)
    float colorTint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    bool useCheckerTexture = false;
};