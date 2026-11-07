#include "ModuleImporter.h"
#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "ComponentMaterial.h"
#include "ModuleResources.h"
#include "ResourceMesh.h"
#include "ResourceTexture.h"
#include "Log.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <assimp/version.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace fs = std::filesystem;

// Assimp guarda una referencia a este objeto, así que tiene que vivir mientras dure el programa
static aiLogStream g_assimpLogStream;


// El FBX guarda la ruta de la textura de la máquina de su autor. La buscamos por nombre de fichero.
static std::string ResolveTexturePath(const std::string& fbxPath, const std::string& texName) {
    const char* base = SDL_GetBasePath();
    const fs::path file = fs::path(texName).filename();

    std::vector<fs::path> candidates = {
        fs::path(texName),                          // tal cual viene en el FBX
        fs::path(fbxPath).parent_path() / file,     // junto al FBX
    };
    if (base) candidates.push_back(fs::path(base) / "Assets" / "Resources" / file);   // en Assets, junto al .exe

    for (const fs::path& c : candidates) {
        std::error_code ec;
        if (fs::exists(c, ec)) return c.string();
    }
    return {};
}

ModuleImporter::ModuleImporter(Application* app) : Module(app) {}
ModuleImporter::~ModuleImporter() {}

bool ModuleImporter::Init() {
    LOG("Initializing Importer Module (Assimp & DevIL)");

    // Los mensajes de Assimp van a nuestra consola
    g_assimpLogStream = {};
    g_assimpLogStream.callback = [](const char* msg, char*) { LOG("[Assimp] %s", msg); };
    aiAttachLogStream(&g_assimpLogStream);

    ilInit();
    iluInit();

    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    LOG("Assimp %d.%d.%d | DevIL %d", aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionPatch(), ilGetInteger(IL_VERSION_NUM));
    return true;
}

bool ModuleImporter::CleanUp() {
    aiDetachAllLogStreams();
    return true;
}

// Pasa la matriz local de un nodo de Assimp al ComponentTransform del GameObject
static void ApplyNodeTransform(const aiNode* node, ComponentTransform* t) {
    aiVector3D scaling, position;
    aiQuaternion rotation;
    node->mTransformation.Decompose(scaling, rotation, position);

    t->position = glm::vec3(position.x, position.y, position.z);
    t->scale = glm::vec3(scaling.x, scaling.y, scaling.z);
    t->rotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
    t->eulerRotation = glm::degrees(glm::eulerAngles(t->rotation));
}

GameObject* ModuleImporter::LoadFBX(const std::string& path) {
    LOG("Attempting to load FBX: %s", path.c_str());

    // Por defecto Assimp parte cada nodo FBX en varios nodos auxiliares ($AssimpFbx$_Translation...)
    // que llenarían la jerarquía de ruido. Con PRESERVE_PIVOTS = 0 los fusiona en un solo nodo.
    aiPropertyStore* props = aiCreatePropertyStore();
    aiSetImportPropertyInteger(props, AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);
    const aiScene* scene = aiImportFileExWithProperties(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality, nullptr, props);
    aiReleasePropertyStore(props);

    if (scene == nullptr || !scene->HasMeshes() || scene->mRootNode == nullptr) {
        LOG("ERROR Loading FBX: Could not open %s", path.c_str());
        if (scene != nullptr) aiReleaseImport(scene);
        return nullptr;
    }

    LOG("FBX Loaded Successfully! Meshes found: %d", scene->mNumMeshes);

    // El GameObject raíz lleva el nombre del fichero y la transformación del nodo raíz del FBX
    GameObject* rootFBX = new GameObject(fs::path(path).stem().string(), App->scene->GetRoot());
    ImportNode(scene, scene->mRootNode, rootFBX, path);

    aiReleaseImport(scene);
    return rootFBX;
}

// Rellena 'go' a partir de 'node': su transform, sus mallas y, recursivamente, sus hijos.
void ModuleImporter::ImportNode(const aiScene* scene, const aiNode* node, GameObject* go, const std::string& fbxPath) {
    ApplyNodeTransform(node, go->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM));

    if (node->mNumMeshes == 1) {
        // Caso habitual (un edificio = un nodo = una malla): la malla vive en el propio nodo
        AttachMesh(scene, node->mMeshes[0], go, fbxPath);
    }
    else {
        // Varias mallas en un nodo (p. ej. una por material): un hijo por malla
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            const aiMesh* aiM = scene->mMeshes[node->mMeshes[i]];
            const std::string meshName = aiM->mName.length > 0
                ? std::string(aiM->mName.C_Str())
                : std::string(node->mName.C_Str()) + "_mesh" + std::to_string(i);
            GameObject* meshGO = new GameObject(meshName, go);
            meshGO->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);
            AttachMesh(scene, node->mMeshes[i], meshGO, fbxPath);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        const aiNode* childNode = node->mChildren[i];
        const std::string childName = childNode->mName.length > 0 ? std::string(childNode->mName.C_Str()) : std::string("Node");
        ImportNode(scene, childNode, new GameObject(childName, go), fbxPath);
    }
}

// Rellena 'go' con el ComponentMaterial (textura difusa) y el ComponentMesh de una malla de Assimp
void ModuleImporter::AttachMesh(const aiScene* scene, unsigned int meshIndex, GameObject* go, const std::string& fbxPath) {
    const aiMesh* ai_mesh = scene->mMeshes[meshIndex];
    ComponentMaterial* material = go->CreateComponent<ComponentMaterial>(ComponentType::MATERIAL);

    // --- TEXTURA DIFUSA DEL MATERIAL DE ESTA MALLA ---
    if (scene->HasMaterials() && ai_mesh->mMaterialIndex < scene->mNumMaterials) {
        aiString texFile;
        if (scene->mMaterials[ai_mesh->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &texFile) == AI_SUCCESS) {
            const aiTexture* embeddedTex = scene->GetEmbeddedTexture(texFile.C_Str());

            if (embeddedTex != nullptr) {
                LOG("Embedded texture found: %s", texFile.C_Str());
                const std::string cacheName = fbxPath + "_embedded_" + texFile.C_Str();

                if (embeddedTex->mHeight == 0) {
                    ApplyTextureFromMemory(embeddedTex->pcData, embeddedTex->mWidth, cacheName, material);
                }
                else {
                    const size_t size = embeddedTex->mWidth * embeddedTex->mHeight * 4;
                    ApplyTextureFromMemory(embeddedTex->pcData, size, cacheName, material);
                }
            }
            else {
                const std::string resolved = ResolveTexturePath(fbxPath, texFile.C_Str());
                if (!resolved.empty()) ApplyTexture(resolved, material);
                else LOG("Diffuse texture '%s' not found for mesh '%s'", texFile.C_Str(), ai_mesh->mName.C_Str());
            }
        }
    }

    // --- MALLA: la clave es fichero + índice, así dos nodos que usan la misma malla comparten el recurso ---
    const std::string meshKey = fbxPath + "#" + std::to_string(meshIndex);
    ResourceMesh* resource = App->resources->FindMesh(meshKey);

    if (resource == nullptr) {
        resource = App->resources->CreateMesh(meshKey);
        resource->numVertices = ai_mesh->mNumVertices;
        resource->vertices.resize(resource->numVertices);

        if (ai_mesh->HasNormals()) resource->normals.resize(resource->numVertices);

        for (unsigned int v = 0; v < ai_mesh->mNumVertices; ++v) {
            resource->vertices[v].x = ai_mesh->mVertices[v].x;
            resource->vertices[v].y = ai_mesh->mVertices[v].y;
            resource->vertices[v].z = ai_mesh->mVertices[v].z;

            if (ai_mesh->HasTextureCoords(0)) {
                resource->vertices[v].u = ai_mesh->mTextureCoords[0][v].x;
                resource->vertices[v].v = ai_mesh->mTextureCoords[0][v].y;
            }
            else {
                resource->vertices[v].u = 0.0f;
                resource->vertices[v].v = 0.0f;
            }

            if (ai_mesh->HasNormals()) {
                resource->normals[v] = glm::vec3(ai_mesh->mNormals[v].x, ai_mesh->mNormals[v].y, ai_mesh->mNormals[v].z);
            }
        }

        if (ai_mesh->HasFaces()) {
            resource->numIndices = ai_mesh->mNumFaces * 3;
            resource->indices.resize(resource->numIndices);

            for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
                if (ai_mesh->mFaces[f].mNumIndices == 3) {
                    resource->indices[(f * 3)] = ai_mesh->mFaces[f].mIndices[0];
                    resource->indices[(f * 3) + 1] = ai_mesh->mFaces[f].mIndices[1];
                    resource->indices[(f * 3) + 2] = ai_mesh->mFaces[f].mIndices[2];
                }
            }
        }

        resource->GenerateBuffers();
        LOG("-> Mesh '%s' loaded. Vertices: %d, Indices: %d", ai_mesh->mName.C_Str(), resource->numVertices, resource->numIndices);
    }

    go->CreateComponent<ComponentMesh>(ComponentType::MESH)->SetResource(resource);
}

// Sube a la GPU la imagen que DevIL tiene activa (ya convertida a RGBA)
static void UploadBoundImage(ResourceTexture* texture) {
    texture->width = ilGetInteger(IL_IMAGE_WIDTH);
    texture->height = ilGetInteger(IL_IMAGE_HEIGHT);

    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool ModuleImporter::ApplyTexture(const std::string& path, ComponentMaterial* material) {
    ResourceTexture* texture = App->resources->FindTexture(path);

    if (texture == nullptr) {
        unsigned int imageID;
        ilGenImages(1, &imageID);
        ilBindImage(imageID);

        if (!ilLoadImage(path.c_str())) {
            LOG("Failed to load texture: %s (DevIL error 0x%x)", path.c_str(), ilGetError());
            ilDeleteImages(1, &imageID);
            return false;
        }
        ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

        texture = App->resources->CreateTexture(path);
        UploadBoundImage(texture);

        ilDeleteImages(1, &imageID);
        LOG("Texture loaded successfully: %s (%dx%d)", path.c_str(), texture->width, texture->height);
    }

    material->SetTexture(texture);
    return true;
}

void ModuleImporter::ApplyTextureRecursive(const std::string& path, GameObject* go) {
    ComponentMaterial* material = go->GetComponent<ComponentMaterial>(ComponentType::MATERIAL);
    if (material) ApplyTexture(path, material);
    for (GameObject* child : go->children) ApplyTextureRecursive(path, child);
}

void ModuleImporter::LoadTexture(const std::string& path, GameObject* target) {
    if (target == nullptr) {
        LOG("No GameObject selected to apply the texture!");
        return;
    }
    ApplyTextureRecursive(path, target);
}

bool ModuleImporter::ApplyTextureFromMemory(const void* buffer, size_t size, const std::string& cacheName, ComponentMaterial* material) {
    ResourceTexture* texture = App->resources->FindTexture(cacheName);

    if (texture == nullptr) {
        unsigned int imageID;
        ilGenImages(1, &imageID);
        ilBindImage(imageID);

        if (!ilLoadL(IL_TYPE_UNKNOWN, buffer, size)) {
            LOG("Failed to load embedded texture (DevIL error 0x%x)", ilGetError());
            ilDeleteImages(1, &imageID);
            return false;
        }
        ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

        texture = App->resources->CreateTexture(cacheName);
        UploadBoundImage(texture);

        ilDeleteImages(1, &imageID);
        LOG("Embedded texture loaded successfully: %s (%dx%d)", cacheName.c_str(), texture->width, texture->height);
    }

    material->SetTexture(texture);
    return true;
}