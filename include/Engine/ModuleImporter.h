#pragma once
#include "Module.h"
#include <string>

class GameObject;
class ComponentMaterial;
struct aiScene;
struct aiNode;

class ModuleImporter : public Module {
public:
    ModuleImporter(Application* app);
    virtual ~ModuleImporter();

    bool Init() override;
    bool CleanUp() override;

    GameObject* LoadFBX(const std::string& path);
    void LoadTexture(const std::string& path, GameObject* target);

private:
    void ImportNode(const aiScene* scene, const aiNode* node, GameObject* go, const std::string& fbxPath);
    void AttachMesh(const aiScene* scene, unsigned int meshIndex, GameObject* go, const std::string& fbxPath);

    bool ApplyTextureFromMemory(const void* buffer, size_t size, const std::string& cacheName, ComponentMaterial* material);
    bool ApplyTexture(const std::string& path, ComponentMaterial* material);
    void ApplyTextureRecursive(const std::string& path, GameObject* go);
};