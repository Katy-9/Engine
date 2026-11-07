#pragma once
#include "Resource.h"

class ResourceTexture : public Resource {
public:
    ResourceTexture(uint64_t uuid, const std::string& key) : Resource(uuid, ResourceType::TEXTURE, key) {}
    ~ResourceTexture() override;

public:
    unsigned int id = 0;   // ID de la textura en VRAM que genera OpenGL
    int width = 0;
    int height = 0;
};