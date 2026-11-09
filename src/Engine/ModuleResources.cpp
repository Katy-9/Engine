#include "ModuleResources.h"
#include "ResourceMesh.h"
#include "ResourceTexture.h"
#include "LibraryFormats.h"
#include "Log.h"
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <filesystem>
#include <random>

ModuleResources::ModuleResources(Application* app) : Module(app) {}
ModuleResources::~ModuleResources() {}

bool ModuleResources::Init() {
    const char* base = SDL_GetBasePath();
    basePath = base ? base : "";
    assetsPath = basePath + "Assets/";
    libraryPath = basePath + "Library/";

    std::error_code ec;
    for (const char* folder : { "Meshes", "Textures", "Models" }) {
        std::filesystem::create_directories(libraryPath + folder, ec);
    }
    std::filesystem::create_directories(assetsPath + "Resources", ec);

    LOG("Library folder: %s", libraryPath.c_str());
    return true;
}

bool ModuleResources::CleanUp() {
    LOG("Freeing %d resources", (int)resources.size());
    resources.clear();
    keyToUUID.clear();
    return true;
}

std::string ModuleResources::GetLibraryFile(const char* folder, uint64_t uuid, const char* extension) const {
    return libraryPath + folder + "/" + std::to_string(uuid) + extension;
}

uint64_t ModuleResources::GenerateUUID() const {
    static std::mt19937_64 rng{ std::random_device{}() };
    uint64_t id;
    do { id = rng(); } while (id == 0 || resources.count(id) != 0);
    return id;
}

template<typename T>
T* ModuleResources::Create(const std::string& key, uint64_t uuid) {
    if (uuid == 0) uuid = GenerateUUID();
    T* resource = new T(uuid, key);
    resources.emplace(uuid, std::unique_ptr<Resource>(resource));
    keyToUUID[key] = uuid;
    return resource;
}

ResourceMesh* ModuleResources::CreateMesh(const std::string& key, uint64_t uuid) { return Create<ResourceMesh>(key, uuid); }
ResourceTexture* ModuleResources::CreateTexture(const std::string& key, uint64_t uuid) { return Create<ResourceTexture>(key, uuid); }

Resource* ModuleResources::FindByKey(const std::string& key, ResourceType type) const {
    const auto it = keyToUUID.find(key);
    if (it == keyToUUID.end()) return nullptr;

    const auto res = resources.find(it->second);
    if (res == resources.end()) return nullptr;
    return res->second->GetType() == type ? res->second.get() : nullptr;
}

ResourceMesh* ModuleResources::FindMesh(const std::string& key) const {
    return static_cast<ResourceMesh*>(FindByKey(key, ResourceType::MESH));
}

ResourceTexture* ModuleResources::FindTexture(const std::string& key) const {
    return static_cast<ResourceTexture*>(FindByKey(key, ResourceType::TEXTURE));
}

ResourceMesh* ModuleResources::LoadMesh(uint64_t uuid) {
    if (Resource* existing = Find(uuid)) {
        return existing->GetType() == ResourceType::MESH ? static_cast<ResourceMesh*>(existing) : nullptr;
    }

    MeshData data;
    const std::string file = GetLibraryFile("Meshes", uuid, ".kmesh");
    if (!ReadMeshFile(file, data)) {
        LOG("Could not read mesh from Library: %s", file.c_str());
        return nullptr;
    }

    ResourceMesh* mesh = CreateMesh(data.name, uuid);
    mesh->vertices = std::move(data.vertices);
    mesh->indices = std::move(data.indices);
    mesh->normals = std::move(data.normals);
    mesh->numVertices = (int)mesh->vertices.size();
    mesh->numIndices = (int)mesh->indices.size();
    mesh->GenerateBuffers();
    return mesh;
}

ResourceTexture* ModuleResources::LoadTexture(uint64_t uuid) {
    if (Resource* existing = Find(uuid)) {
        return existing->GetType() == ResourceType::TEXTURE ? static_cast<ResourceTexture*>(existing) : nullptr;
    }

    TextureData data;
    const std::string file = GetLibraryFile("Textures", uuid, ".ktex");
    if (!ReadTextureFile(file, data)) {
        LOG("Could not read texture from Library: %s", file.c_str());
        return nullptr;
    }

    ResourceTexture* texture = CreateTexture(data.name, uuid);
    texture->width = data.width;
    texture->height = data.height;

    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void ModuleResources::AddReference(Resource* resource) {
    if (resource) ++resource->referenceCount;
}

void ModuleResources::ReleaseReference(Resource* resource) {
    if (resource && resource->referenceCount > 0) --resource->referenceCount;
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

size_t ModuleResources::Count(ResourceType type) const {
    size_t n = 0;
    for (const auto& entry : resources) {
        if (entry.second->GetType() == type) ++n;
    }
    return n;
}