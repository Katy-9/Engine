#include "ModuleImporter.h"
#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "ComponentMaterial.h"
#include "Log.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <filesystem>

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
    if (base) candidates.push_back(fs::path(base) / "Assets" / file);   // en Assets, junto al .exe

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

    // Obligamos a DevIL a leer las texturas al derecho para OpenGL
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    LOG("Assimp %d.%d.%d | DevIL %d", aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionPatch(), ilGetInteger(IL_VERSION_NUM));
    return true;
}

bool ModuleImporter::CleanUp() {
    for (auto& entry : textureCache) {
        glDeleteTextures(1, &entry.second.id);
    }
    textureCache.clear();
    aiDetachAllLogStreams();
    return true;
}

GameObject* ModuleImporter::LoadFBX(const std::string& path) {
    LOG("Attempting to load FBX: %s", path.c_str());

    const aiScene* scene = aiImportFile(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

    if (scene == nullptr || !scene->HasMeshes()) {
        LOG("ERROR Loading FBX: Could not open %s", path.c_str());
        return nullptr;
    }

    LOG("FBX Loaded Successfully! Meshes found: %d", scene->mNumMeshes);

    GameObject* rootFBX = new GameObject(fs::path(path).stem().string(), App->scene->GetRoot());
    rootFBX->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* ai_mesh = scene->mMeshes[i];

        GameObject* childObj = new GameObject(ai_mesh->mName.C_Str(), rootFBX);
        childObj->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);
        ComponentMaterial* material = childObj->CreateComponent<ComponentMaterial>(ComponentType::MATERIAL);

        // --- TEXTURA DIFUSA DEL MATERIAL DE ESTA MALLA ---
        if (scene->HasMaterials() && ai_mesh->mMaterialIndex < scene->mNumMaterials) {
            aiString texFile;
            if (scene->mMaterials[ai_mesh->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &texFile) == AI_SUCCESS) {
                const std::string resolved = ResolveTexturePath(path, texFile.C_Str());
                if (!resolved.empty()) ApplyTexture(resolved, material);
                else LOG("Diffuse texture '%s' not found for mesh '%s'", texFile.C_Str(), ai_mesh->mName.C_Str());
            }
        }

        ComponentMesh* myMesh = childObj->CreateComponent<ComponentMesh>(ComponentType::MESH);
        myMesh->numVertices = ai_mesh->mNumVertices;
        myMesh->vertices.resize(myMesh->numVertices);

        if (ai_mesh->HasNormals()) myMesh->normals.resize(myMesh->numVertices);

        for (unsigned int v = 0; v < myMesh->numVertices; ++v) {
            myMesh->vertices[v].x = ai_mesh->mVertices[v].x;
            myMesh->vertices[v].y = ai_mesh->mVertices[v].y;
            myMesh->vertices[v].z = ai_mesh->mVertices[v].z;

            if (ai_mesh->HasTextureCoords(0)) {
                myMesh->vertices[v].u = ai_mesh->mTextureCoords[0][v].x;
                myMesh->vertices[v].v = ai_mesh->mTextureCoords[0][v].y;
            }
            else {
                myMesh->vertices[v].u = 0.0f;
                myMesh->vertices[v].v = 0.0f;
            }

            if (ai_mesh->HasNormals()) {
                myMesh->normals[v] = glm::vec3(ai_mesh->mNormals[v].x, ai_mesh->mNormals[v].y, ai_mesh->mNormals[v].z);
            }
        }

        if (ai_mesh->HasFaces()) {
            myMesh->numIndices = ai_mesh->mNumFaces * 3;
            myMesh->indices.resize(myMesh->numIndices);

            for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
                if (ai_mesh->mFaces[f].mNumIndices == 3) {
                    myMesh->indices[(f * 3)] = ai_mesh->mFaces[f].mIndices[0];
                    myMesh->indices[(f * 3) + 1] = ai_mesh->mFaces[f].mIndices[1];
                    myMesh->indices[(f * 3) + 2] = ai_mesh->mFaces[f].mIndices[2];
                }
            }
        }

        myMesh->GenerateBuffers();
        LOG("-> Mesh '%s' loaded. Vertices: %d, Indices: %d", ai_mesh->mName.C_Str(), myMesh->numVertices, myMesh->numIndices);
    }

    aiReleaseImport(scene);
    return rootFBX;
}

bool ModuleImporter::ApplyTexture(const std::string& path, ComponentMaterial* material) {
    auto it = textureCache.find(path);

    if (it == textureCache.end()) {   // primera vez que vemos esta textura: la cargamos
        unsigned int imageID;
        ilGenImages(1, &imageID);
        ilBindImage(imageID);

        if (!ilLoadImage(path.c_str())) {
            LOG("Failed to load texture: %s (DevIL error 0x%x)", path.c_str(), ilGetError());
            ilDeleteImages(1, &imageID);
            return false;
        }
        ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

        TextureInfo info{ 0, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT) };

        glGenTextures(1, &info.id);
        glBindTexture(GL_TEXTURE_2D, info.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        ilDeleteImages(1, &imageID);
        LOG("Texture loaded successfully: %s (%dx%d)", path.c_str(), info.width, info.height);
        it = textureCache.emplace(path, info).first;
    }

    material->textureID = it->second.id;      // el dueño de la textura es la caché, no el material
    material->textureWidth = it->second.width;
    material->textureHeight = it->second.height;
    material->texturePath = path;
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
    ApplyTextureRecursive(path, target);   // así también vale seleccionar el padre del FBX
}