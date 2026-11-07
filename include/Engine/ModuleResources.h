#pragma once
#include "Module.h"
#include "Resource.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ResourceMesh;
class ResourceTexture;

class ModuleResources : public Module {
public:
    ModuleResources(Application* app);
    virtual ~ModuleResources();

    bool CleanUp() override;

    // nullptr si ese recurso aún no está cargado
    ResourceMesh* FindMesh(const std::string& key) const;
    ResourceTexture* FindTexture(const std::string& key) const;

    Resource* Find(uint64_t uuid) const;
    // Todos los recursos de ese tipo, ordenados por nombre (para listas del editor)
    std::vector<Resource*> GetResources(ResourceType type) const;

    // Registra un recurso nuevo (UUID nuevo, 0 referencias) para que el llamador lo rellene
    ResourceMesh* CreateMesh(const std::string& key);
    ResourceTexture* CreateTexture(const std::string& key);

    void AddReference(Resource* resource);
    void ReleaseReference(Resource* resource);

    size_t Count(ResourceType type) const;
    const std::unordered_map<uint64_t, std::unique_ptr<Resource>>& GetAll() const { return resources; }

private:
    template<typename T> T* Create(const std::string& key);
    Resource* FindByKey(const std::string& key, ResourceType type) const;
    uint64_t GenerateUUID() const;

    std::unordered_map<uint64_t, std::unique_ptr<Resource>> resources;
    std::unordered_map<std::string, uint64_t> keyToUUID;
};