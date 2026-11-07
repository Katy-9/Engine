#include "ModuleResources.h"
#include "ResourceMesh.h"
#include "ResourceTexture.h"
#include "Log.h"
#include <random>
#include <algorithm>

ModuleResources::ModuleResources(Application* app) : Module(app) {}
ModuleResources::~ModuleResources() {}

bool ModuleResources::CleanUp() {
    LOG("Freeing %d resources", (int)resources.size());
    resources.clear();
    keyToUUID.clear();
    return true;
}

uint64_t ModuleResources::GenerateUUID() const {
    static std::mt19937_64 rng{ std::random_device{}() };
    uint64_t id;
    do { id = rng(); } while (id == 0 || resources.count(id) != 0);
    return id;
}

template<typename T>
T* ModuleResources::Create(const std::string& key) {
    const uint64_t uuid = GenerateUUID();
    T* resource = new T(uuid, key);
    resources.emplace(uuid, std::unique_ptr<Resource>(resource));
    keyToUUID[key] = uuid;
    return resource;
}

ResourceMesh* ModuleResources::CreateMesh(const std::string& key) { return Create<ResourceMesh>(key); }
ResourceTexture* ModuleResources::CreateTexture(const std::string& key) { return Create<ResourceTexture>(key); }

Resource* ModuleResources::FindByKey(const std::string& key, ResourceType type) const {
    const auto it = keyToUUID.find(key);
    if (it == keyToUUID.end()) return nullptr;

    Resource* resource = resources.at(it->second).get();
    return resource->GetType() == type ? resource : nullptr;
}

ResourceMesh* ModuleResources::FindMesh(const std::string& key) const {
    return static_cast<ResourceMesh*>(FindByKey(key, ResourceType::MESH));
}

ResourceTexture* ModuleResources::FindTexture(const std::string& key) const {
    return static_cast<ResourceTexture*>(FindByKey(key, ResourceType::TEXTURE));
}

void ModuleResources::AddReference(Resource* resource) {
    if (resource) ++resource->referenceCount;
}

void ModuleResources::ReleaseReference(Resource* resource) {
    if (resource && resource->referenceCount > 0) --resource->referenceCount;
}

size_t ModuleResources::Count(ResourceType type) const {
    size_t n = 0;
    for (const auto& entry : resources) {
        if (entry.second->GetType() == type) ++n;
    }
    return n;
}

Resource* ModuleResources::Find(uint64_t uuid) const {
    const auto it = resources.find(uuid);
    return it != resources.end() ? it->second.get() : nullptr;
}

std::vector<Resource*> ModuleResources::GetResources(ResourceType type) const {
    std::vector<Resource*> result;
    for (const auto& entry : resources) {
        if (entry.second->GetType() == type) result.push_back(entry.second.get());
    }
    std::sort(result.begin(), result.end(), [](const Resource* a, const Resource* b) {
        return a->GetKey() < b->GetKey();
        });
    return result;
}