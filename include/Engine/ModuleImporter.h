#pragma once
#include "Module.h"
#include <cstdint>
#include <filesystem>
#include <string>

class GameObject;
class ResourceTexture;
class MetaFile;
struct aiTexture;

class ModuleImporter : public Module {
public:
    ModuleImporter(Application* app);
    virtual ~ModuleImporter();

    bool Init() override;
    bool CleanUp() override;

    // Importa el FBX a Library (si hace falta) y crea sus GameObjects leyendo el modelo de Library
    GameObject* LoadFBX(const std::string& path);
    // Importa la textura a Library (si hace falta) y se la pone a 'target' y a todos sus hijos
    void LoadTexture(const std::string& path, GameObject* target);

private:
    void ImportAssets();   // recorre Assets/ e importa lo que falte en Library/
    std::filesystem::path EnsureInAssets(const std::filesystem::path& file) const;
    std::string AssetKey(const std::filesystem::path& assetFile) const;

    // Devuelven el UUID del recurso (0 si falla)
    uint64_t ImportTextureFile(const std::filesystem::path& file);
    uint64_t ImportEmbeddedTexture(const aiTexture* texture, const std::string& textureName, const std::string& keyBase, MetaFile& meta);
    uint64_t ImportFBX(const std::filesystem::path& assetFile, const std::filesystem::path& sourceFile);

    GameObject* InstantiateModel(uint64_t modelUUID);
    void ApplyTextureRecursive(ResourceTexture* texture, GameObject* go);
};