#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "ResourceMesh.h"   // para Vertex

// ---------- Formato propio de Library ----------
// Todos los ficheros empiezan por: magic (4 bytes) + versión (uint32).

struct MeshData {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<glm::vec3> normals;
};

struct TextureData {
    std::string name;
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;   // RGBA, 8 bits por canal
};

// Un nodo del modelo = un GameObject
struct ModelNode {
    std::string name;
    int parent = -1;   // índice en ModelData::nodes (-1 = raíz). Siempre menor que el índice del nodo
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    uint64_t meshUUID = 0;      // 0 = sin malla
    uint64_t textureUUID = 0;   // 0 = sin textura
};

struct ModelData {
    std::string name;
    std::vector<ModelNode> nodes;
};

bool WriteMeshFile(const std::string& path, const MeshData& data);
bool ReadMeshFile(const std::string& path, MeshData& out);
bool WriteTextureFile(const std::string& path, const TextureData& data);
bool ReadTextureFile(const std::string& path, TextureData& out);
bool WriteModelFile(const std::string& path, const ModelData& data);
bool ReadModelFile(const std::string& path, ModelData& out);

// ---------- Ficheros .meta: texto plano "clave=valor" ----------
class MetaFile {
public:
    explicit MetaFile(const std::string& path);   // lee el fichero si existe

    std::string GetString(const std::string& key, const std::string& fallback = "") const;
    void SetString(const std::string& key, const std::string& value);

    uint64_t GetUUID(const std::string& key) const;   // 0 si no existe
    void SetUUID(const std::string& key, uint64_t value);

    int GetInt(const std::string& key, int fallback) const;
    void SetInt(const std::string& key, int value);

    bool Save();   // solo escribe si algo ha cambiado

private:
    std::string path;
    std::map<std::string, std::string> values;
    bool dirty = false;
};