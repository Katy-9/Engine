#pragma once
#include "Module.h"
#include "Resource.h"
#include <cstdint>
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

    bool Init() override;
    bool CleanUp() override;

    // Rutas junto al .exe (terminan en '/')
    const std::string& GetBasePath() const { return basePath; }
    const std::string& GetAssetsPath() const { return assetsPath; }
    const std::string& GetLibraryPath() const { return libraryPath; }
    // Ruta de un fichero de Library: <Library>/<folder>/<uuid><extension>
    std::string GetLibraryFile(const char* folder, uint64_t uuid, const char* extension) const;

    // nullptr si ese recurso aún no está cargado
    ResourceMesh* FindMesh(const std::string& key) const;
    ResourceTexture* FindTexture(const std::string& key) const;

    // Registra un recurso nuevo (uuid 0 = generar uno) para que el llamador lo rellene
    ResourceMesh* CreateMesh(const std::string& key, uint64_t uuid = 0);
    ResourceTexture* CreateTexture(const std::string& key, uint64_t uuid = 0);

    // Carga el recurso desde Library. Si ya está en memoria devuelve el mismo, sin duplicarlo
    ResourceMesh* LoadMesh(uint64_t uuid);
    ResourceTexture* LoadTexture(uint64_t uuid);

    void AddReference(Resource* resource);
    void ReleaseReference(Resource* resource);

    Resource* Find(uint64_t uuid) const;
    // Todos los recursos de ese tipo, ordenados por nombre (para listas del editor)
    std::vector<Resource*> GetResources(ResourceType type) const;

    size_t Count(ResourceType type) const;
    const std::unordered_map<uint64_t, std::unique_ptr<Resource>>& GetAll() const { return resources; }

    uint64_t GenerateUUID() const;

private:
    template<typename T> T* Create(const std::string& key, uint64_t uuid);
    Resource* FindByKey(const std::string& key, ResourceType type) const;

    std::string basePath;
    std::string assetsPath;
    std::string libraryPath;

    std::unordered_map<uint64_t, std::unique_ptr<Resource>> resources;
    std::unordered_map<std::string, uint64_t> keyToUUID;
};