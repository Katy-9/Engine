#include "ModuleImporter.h"
#include "Application.h"
#include "ModuleScene.h"
#include "ModuleResources.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "ComponentMaterial.h"
#include "ResourceMesh.h"
#include "ResourceTexture.h"
#include "LibraryFormats.h"
#include "Log.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <assimp/version.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cctype>
#include <map>

namespace fs = std::filesystem;

// Assimp guarda una referencia a este objeto, así que tiene que vivir mientras dure el programa
static aiLogStream g_assimpLogStream;

// ---------------------------------------------------------------- utilidades

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

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

static bool IsTextureExtension(const std::string& ext) {
    return ext == ".png" || ext == ".tga" || ext == ".dds" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
}

// Copia la imagen que DevIL tiene activa a TextureData, convertida a RGBA de 8 bits
static bool CopyBoundImage(TextureData& out) {
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    out.width = ilGetInteger(IL_IMAGE_WIDTH);
    out.height = ilGetInteger(IL_IMAGE_HEIGHT);

    const unsigned char* pixels = ilGetData();
    if (pixels == nullptr || out.width <= 0 || out.height <= 0) return false;

    out.pixels.assign(pixels, pixels + (size_t)out.width * (size_t)out.height * 4);
    return true;
}

static bool DecodeImageFile(const std::string& path, TextureData& out) {
    unsigned int image = 0;
    ilGenImages(1, &image);
    ilBindImage(image);

    bool ok = ilLoadImage(path.c_str()) != IL_FALSE;
    if (!ok) LOG("Failed to decode image: %s (DevIL error 0x%x)", path.c_str(), ilGetError());
    else ok = CopyBoundImage(out);

    ilDeleteImages(1, &image);
    return ok;
}

static bool DecodeImageMemory(const void* buffer, size_t size, TextureData& out) {
    unsigned int image = 0;
    ilGenImages(1, &image);
    ilBindImage(image);

    bool ok = ilLoadL(IL_TYPE_UNKNOWN, buffer, (unsigned int)size) != IL_FALSE;
    if (!ok) LOG("Failed to decode embedded image (DevIL error 0x%x)", ilGetError());
    else ok = CopyBoundImage(out);

    ilDeleteImages(1, &image);
    return ok;
}

static MeshData BuildMeshData(const aiMesh* ai_mesh, const std::string& name) {
    MeshData data;
    data.name = name;
    data.vertices.resize(ai_mesh->mNumVertices);
    if (ai_mesh->HasNormals()) data.normals.resize(ai_mesh->mNumVertices);

    for (unsigned int v = 0; v < ai_mesh->mNumVertices; ++v) {
        data.vertices[v].x = ai_mesh->mVertices[v].x;
        data.vertices[v].y = ai_mesh->mVertices[v].y;
        data.vertices[v].z = ai_mesh->mVertices[v].z;

        if (ai_mesh->HasTextureCoords(0)) {
            data.vertices[v].u = ai_mesh->mTextureCoords[0][v].x;
            data.vertices[v].v = ai_mesh->mTextureCoords[0][v].y;
        }
        else {
            data.vertices[v].u = 0.0f;
            data.vertices[v].v = 0.0f;
        }

        if (ai_mesh->HasNormals()) {
            data.normals[v] = glm::vec3(ai_mesh->mNormals[v].x, ai_mesh->mNormals[v].y, ai_mesh->mNormals[v].z);
        }
    }

    if (ai_mesh->HasFaces()) {
        data.indices.reserve((size_t)ai_mesh->mNumFaces * 3);
        for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
            const aiFace& face = ai_mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;   // el post-proceso Triangulate deja solo triángulos
            data.indices.push_back(face.mIndices[0]);
            data.indices.push_back(face.mIndices[1]);
            data.indices.push_back(face.mIndices[2]);
        }
    }
    return data;
}

// Pasa la matriz local de un nodo de Assimp a posición / rotación / escala
static void DecomposeNode(const aiNode* node, ModelNode& out) {
    aiVector3D scaling, position;
    aiQuaternion rotation;
    node->mTransformation.Decompose(scaling, rotation, position);

    out.position = glm::vec3(position.x, position.y, position.z);
    out.scale = glm::vec3(scaling.x, scaling.y, scaling.z);
    out.rotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
}

// Aplana el árbol de nodos de Assimp en ModelData (padres siempre antes que sus hijos).
// Un nodo con una sola malla la lleva dentro; con varias, cada malla va en un hijo.
static void BuildNodes(const aiScene* scene, const aiNode* node, int parent, const std::string& nameOverride,
    const std::vector<uint64_t>& meshUUIDs, const std::vector<uint64_t>& meshTextures, ModelData& model) {

    ModelNode entry;
    entry.name = !nameOverride.empty() ? nameOverride
        : (node->mName.length > 0 ? std::string(node->mName.C_Str()) : std::string("Node"));
    entry.parent = parent;
    DecomposeNode(node, entry);

    if (node->mNumMeshes == 1) {
        entry.meshUUID = meshUUIDs[node->mMeshes[0]];
        entry.textureUUID = meshTextures[node->mMeshes[0]];
    }

    const int index = (int)model.nodes.size();
    model.nodes.push_back(entry);

    if (node->mNumMeshes > 1) {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            const unsigned int meshIndex = node->mMeshes[i];
            const aiMesh* aiM = scene->mMeshes[meshIndex];

            ModelNode meshEntry;
            meshEntry.name = aiM->mName.length > 0 ? std::string(aiM->mName.C_Str()) : entry.name + "_mesh" + std::to_string(i);
            meshEntry.parent = index;
            meshEntry.meshUUID = meshUUIDs[meshIndex];
            meshEntry.textureUUID = meshTextures[meshIndex];
            model.nodes.push_back(meshEntry);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        BuildNodes(scene, node->mChildren[i], index, "", meshUUIDs, meshTextures, model);
    }
}

// ---------------------------------------------------------------- módulo

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

    ImportAssets();
    return true;
}

bool ModuleImporter::CleanUp() {
    aiDetachAllLogStreams();
    return true;
}

// Recorre Assets/ y crea en Library/ todo lo que aún no esté importado.
// Las texturas van primero porque los modelos las referencian.
void ModuleImporter::ImportAssets() {
    LOG("Scanning Assets for resources to import...");

    std::vector<fs::path> textures, models;
    std::error_code ec;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(App->resources->GetAssetsPath(), ec)) {
        if (!entry.is_regular_file(ec)) continue;

        const std::string ext = ToLower(entry.path().extension().string());
        if (IsTextureExtension(ext)) textures.push_back(entry.path());
        else if (ext == ".fbx") models.push_back(entry.path());
    }

    for (const fs::path& t : textures) ImportTextureFile(t);
    for (const fs::path& m : models) ImportFBX(m, m);

    LOG("Assets scan finished: %d textures, %d models", (int)textures.size(), (int)models.size());
}

// Devuelve la ruta del fichero dentro de Assets/. Si estaba fuera, lo copia a Assets/Resources
fs::path ModuleImporter::EnsureInAssets(const fs::path& file) const {
    std::error_code ec;
    const fs::path assets = fs::weakly_canonical(App->resources->GetAssetsPath(), ec);
    const fs::path canon = fs::weakly_canonical(file, ec);

    const fs::path rel = canon.lexically_relative(assets);
    if (!rel.empty() && rel.begin()->string() != "..") return canon;   // ya está dentro de Assets

    const fs::path dest = assets / "Resources" / canon.filename();
    if (!fs::exists(dest, ec)) {
        fs::copy_file(canon, dest, ec);
        if (ec) {
            LOG("Could not copy %s into Assets: %s", canon.string().c_str(), ec.message().c_str());
            return canon;
        }
        LOG("Copied %s to Assets/Resources", canon.filename().string().c_str());
    }
    return dest;
}

// Nombre único del asset: su ruta relativa al .exe, p. ej. "Assets/Resources/foo.fbx"
std::string ModuleImporter::AssetKey(const fs::path& assetFile) const {
    std::error_code ec;
    const fs::path rel = fs::relative(assetFile, App->resources->GetBasePath(), ec);
    return (ec || rel.empty()) ? assetFile.filename().generic_string() : rel.generic_string();
}

uint64_t ModuleImporter::ImportTextureFile(const fs::path& file) {
    const fs::path assetFile = EnsureInAssets(file);
    MetaFile meta(assetFile.string() + ".meta");

    uint64_t uuid = meta.GetUUID("guid");
    if (uuid == 0) uuid = App->resources->GenerateUUID();
    meta.SetUUID("guid", uuid);

    const std::string libFile = App->resources->GetLibraryFile("Textures", uuid, ".ktex");
    std::error_code ec;
    if (!fs::exists(libFile, ec)) {
        TextureData data;
        data.name = AssetKey(assetFile);
        if (!DecodeImageFile(assetFile.string(), data)) return 0;

        if (!WriteTextureFile(libFile, data)) {
            LOG("Could not write texture to Library: %s", libFile.c_str());
            return 0;
        }
        LOG("Texture imported: %s (%dx%d)", data.name.c_str(), data.width, data.height);
    }

    meta.Save();
    return uuid;
}

// Las texturas embebidas no son un fichero en Assets: su UUID se guarda en el .meta del FBX
uint64_t ModuleImporter::ImportEmbeddedTexture(const aiTexture* texture, const std::string& textureName, const std::string& keyBase, MetaFile& meta) {
    const std::string metaKey = "tex." + textureName;
    uint64_t uuid = meta.GetUUID(metaKey);
    if (uuid == 0) {
        uuid = App->resources->GenerateUUID();
        meta.SetUUID(metaKey, uuid);
    }

    const std::string libFile = App->resources->GetLibraryFile("Textures", uuid, ".ktex");
    std::error_code ec;
    if (fs::exists(libFile, ec)) return uuid;

    TextureData data;
    data.name = keyBase + "/" + textureName;

    if (texture->mHeight == 0) {
        // Imagen comprimida (png, jpg...): mWidth es el tamaño en bytes
        if (!DecodeImageMemory(texture->pcData, texture->mWidth, data)) return 0;
    }
    else {
        // Píxeles sin comprimir (aiTexel = BGRA). Invertimos las filas para que coincida con el origen de DevIL
        data.width = (int)texture->mWidth;
        data.height = (int)texture->mHeight;
        data.pixels.resize((size_t)data.width * data.height * 4);
        for (int y = 0; y < data.height; ++y) {
            for (int x = 0; x < data.width; ++x) {
                const aiTexel& t = texture->pcData[(size_t)(data.height - 1 - y) * data.width + x];
                unsigned char* dst = &data.pixels[((size_t)y * data.width + x) * 4];
                dst[0] = t.r; dst[1] = t.g; dst[2] = t.b; dst[3] = t.a;
            }
        }
    }

    if (!WriteTextureFile(libFile, data)) return 0;
    LOG("Embedded texture imported: %s (%dx%d)", data.name.c_str(), data.width, data.height);
    return uuid;
}

// Importa el FBX a Library: una malla (.kmesh) por cada malla y un modelo (.kmodel) con la jerarquía.
// 'sourceFile' es donde estaba el FBX originalmente (para buscar sus texturas junto a él).
uint64_t ModuleImporter::ImportFBX(const fs::path& assetFile, const fs::path& sourceFile) {
    MetaFile meta(assetFile.string() + ".meta");
    const std::string key = AssetKey(assetFile);
    std::error_code ec;

    // ¿Ya importado? El .meta tiene su UUID y están todos los ficheros de Library
    uint64_t modelUUID = meta.GetUUID("guid");
    if (modelUUID != 0 && fs::exists(App->resources->GetLibraryFile("Models", modelUUID, ".kmodel"), ec)) {
        const int meshCount = meta.GetInt("meshes", -1);
        bool complete = meshCount >= 0;
        for (int i = 0; complete && i < meshCount; ++i) {
            const uint64_t meshUUID = meta.GetUUID("mesh." + std::to_string(i));
            complete = meshUUID != 0 && fs::exists(App->resources->GetLibraryFile("Meshes", meshUUID, ".kmesh"), ec);
        }
        if (complete) return modelUUID;
    }

    LOG("Importing FBX: %s", key.c_str());

    // Por defecto Assimp parte cada nodo FBX en varios nodos auxiliares ($AssimpFbx$_Translation...)
    // que llenarían la jerarquía de ruido. Con PRESERVE_PIVOTS = 0 los fusiona en un solo nodo.
    aiPropertyStore* props = aiCreatePropertyStore();
    aiSetImportPropertyInteger(props, AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);
    const aiScene* scene = aiImportFileExWithProperties(assetFile.string().c_str(), aiProcessPreset_TargetRealtime_MaxQuality, nullptr, props);
    aiReleasePropertyStore(props);

    if (scene == nullptr || !scene->HasMeshes() || scene->mRootNode == nullptr) {
        LOG("ERROR Importing FBX: Could not open %s", assetFile.string().c_str());
        if (scene != nullptr) aiReleaseImport(scene);
        return 0;
    }

    if (modelUUID == 0) modelUUID = App->resources->GenerateUUID();
    meta.SetUUID("guid", modelUUID);
    meta.SetInt("meshes", (int)scene->mNumMeshes);

    std::vector<uint64_t> meshUUIDs(scene->mNumMeshes, 0);
    std::vector<uint64_t> meshTextures(scene->mNumMeshes, 0);
    std::map<unsigned int, uint64_t> materialTextures;   // una textura por material, aunque lo usen varias mallas

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* ai_mesh = scene->mMeshes[i];

        // UUID estable de la malla: se guarda en el .meta para que Library se regenere igual
        const std::string metaKey = "mesh." + std::to_string(i);
        uint64_t meshUUID = meta.GetUUID(metaKey);
        if (meshUUID == 0) {
            meshUUID = App->resources->GenerateUUID();
            meta.SetUUID(metaKey, meshUUID);
        }
        meshUUIDs[i] = meshUUID;

        const MeshData data = BuildMeshData(ai_mesh, key + "#" + std::to_string(i));
        const std::string meshFile = App->resources->GetLibraryFile("Meshes", meshUUID, ".kmesh");
        if (!WriteMeshFile(meshFile, data)) {
            LOG("Could not write mesh to Library: %s", meshFile.c_str());
            meshUUIDs[i] = 0;
        }

        // Textura difusa del material de esta malla
        const unsigned int materialIndex = ai_mesh->mMaterialIndex;
        const auto cached = materialTextures.find(materialIndex);
        if (cached != materialTextures.end()) {
            meshTextures[i] = cached->second;
            continue;
        }

        uint64_t textureUUID = 0;
        aiString texFile;
        if (scene->HasMaterials() && materialIndex < scene->mNumMaterials
            && scene->mMaterials[materialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &texFile) == AI_SUCCESS) {

            if (const aiTexture* embedded = scene->GetEmbeddedTexture(texFile.C_Str())) {
                textureUUID = ImportEmbeddedTexture(embedded, texFile.C_Str(), key, meta);
            }
            else {
                const std::string resolved = ResolveTexturePath(sourceFile.string(), texFile.C_Str());
                if (!resolved.empty()) textureUUID = ImportTextureFile(resolved);
                else LOG("Diffuse texture '%s' not found for mesh '%s'", texFile.C_Str(), ai_mesh->mName.C_Str());
            }
        }
        materialTextures[materialIndex] = textureUUID;
        meshTextures[i] = textureUUID;
    }

    ModelData model;
    model.name = assetFile.stem().string();
    BuildNodes(scene, scene->mRootNode, -1, model.name, meshUUIDs, meshTextures, model);
    aiReleaseImport(scene);

    const std::string modelFile = App->resources->GetLibraryFile("Models", modelUUID, ".kmodel");
    if (!WriteModelFile(modelFile, model)) {
        LOG("Could not write model to Library: %s", modelFile.c_str());
        return 0;
    }

    meta.Save();
    LOG("FBX imported: %s (%d meshes, %d nodes)", key.c_str(), (int)meshUUIDs.size(), (int)model.nodes.size());
    return modelUUID;
}

// Crea los GameObjects de un modelo de Library. Las mallas y texturas se cargan una sola vez aunque se repitan
GameObject* ModuleImporter::InstantiateModel(uint64_t modelUUID) {
    ModelData model;
    const std::string file = App->resources->GetLibraryFile("Models", modelUUID, ".kmodel");
    if (!ReadModelFile(file, model) || model.nodes.empty()) {
        LOG("Could not read model from Library: %s", file.c_str());
        return nullptr;
    }

    std::vector<GameObject*> created(model.nodes.size(), nullptr);
    for (size_t i = 0; i < model.nodes.size(); ++i) {
        const ModelNode& node = model.nodes[i];

        GameObject* parent = App->scene->GetRoot();
        if (node.parent >= 0 && (size_t)node.parent < i) parent = created[node.parent];

        GameObject* go = new GameObject(node.name, parent);
        created[i] = go;

        ComponentTransform* transform = go->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);
        transform->position = node.position;
        transform->rotation = node.rotation;
        transform->scale = node.scale;
        transform->eulerRotation = glm::degrees(glm::eulerAngles(node.rotation));

        if (node.meshUUID != 0) {
            ComponentMaterial* material = go->CreateComponent<ComponentMaterial>(ComponentType::MATERIAL);
            if (node.textureUUID != 0) material->SetTexture(App->resources->LoadTexture(node.textureUUID));

            ResourceMesh* mesh = App->resources->LoadMesh(node.meshUUID);
            if (mesh != nullptr) go->CreateComponent<ComponentMesh>(ComponentType::MESH)->SetResource(mesh);
        }
    }

    LOG("Model '%s' loaded from Library (%d GameObjects)", model.name.c_str(), (int)model.nodes.size());
    return created[0];
}

GameObject* ModuleImporter::LoadFBX(const std::string& path) {
    LOG("Attempting to load FBX: %s", path.c_str());

    const fs::path assetFile = EnsureInAssets(path);
    const uint64_t modelUUID = ImportFBX(assetFile, path);
    if (modelUUID == 0) {
        LOG("ERROR Loading FBX: Could not open %s", path.c_str());
        return nullptr;
    }
    return InstantiateModel(modelUUID);
}

void ModuleImporter::LoadTexture(const std::string& path, GameObject* target) {
    const uint64_t uuid = ImportTextureFile(path);
    if (uuid == 0) return;

    if (target == nullptr) {
        LOG("Texture imported. Select a GameObject to apply it.");
        return;
    }

    ResourceTexture* texture = App->resources->LoadTexture(uuid);
    if (texture != nullptr) ApplyTextureRecursive(texture, target);
}

void ModuleImporter::ApplyTextureRecursive(ResourceTexture* texture, GameObject* go) {
    ComponentMaterial* material = go->GetComponent<ComponentMaterial>(ComponentType::MATERIAL);
    if (material) material->SetTexture(texture);
    for (GameObject* child : go->children) ApplyTextureRecursive(texture, child);
}