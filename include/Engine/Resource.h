#pragma once
#include <cstdint>
#include <string>

enum class ResourceType { MESH, TEXTURE };

class Resource {
public:
    Resource(uint64_t uuid, ResourceType type, const std::string& key) : uuid(uuid), type(type), key(key) {}
    virtual ~Resource() {}

    uint64_t GetUUID() const { return uuid; }
    ResourceType GetType() const { return type; }
    // Identifica el origen del recurso (ruta del fichero, "Primitive:Cube"...)
    const std::string& GetKey() const { return key; }
    unsigned int GetReferenceCount() const { return referenceCount; }

private:
    friend class ModuleResources;   // solo el gestor modifica el contador
    uint64_t uuid;
    ResourceType type;
    std::string key;
    unsigned int referenceCount = 0;
};