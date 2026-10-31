#pragma once
#include "Module.h"
#include <string>
#include <unordered_map>

class GameObject;
class ComponentMaterial;

class ModuleImporter : public Module {
public:
    ModuleImporter(Application* app);
    virtual ~ModuleImporter();

    bool Init() override;
    bool CleanUp() override;

    GameObject* LoadFBX(const std::string& path);
    void LoadTexture(const std::string& path, GameObject* target);

    size_t GetLoadedTextureCount() const { return textureCache.size(); }

private:
    struct TextureInfo { unsigned int id; int width; int height; };

    bool ApplyTexture(const std::string& path, ComponentMaterial* material);
    void ApplyTextureRecursive(const std::string& path, GameObject* go);

    std::unordered_map<std::string, TextureInfo> textureCache;
};