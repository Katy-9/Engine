#pragma once
#include "Resource.h"
#include <vector>
#include <glm/glm.hpp>

struct Vertex {
    float x, y, z;
    float u, v;
};

class ResourceMesh : public Resource {
public:
    ResourceMesh(uint64_t uuid, const std::string& key) : Resource(uuid, ResourceType::MESH, key) {}
    ~ResourceMesh() override;

    void GenerateBuffers();
    void DrawMesh(bool wireframe) const;
    void DrawNormals(unsigned int shaderProgram, bool vertexNormals, bool faceNormals) const;

public:
    // Buffers de la malla principal
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    unsigned int vao = 0;

    // Buffers de las líneas de las normales
    unsigned int vao_normals = 0;
    unsigned int vbo_normals = 0;
    int numNormalVertices = 0;

    unsigned int vao_face_normals = 0;
    unsigned int vbo_face_normals = 0;
    int numFaceNormalVertices = 0;

    int numVertices = 0;
    int numIndices = 0;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<glm::vec3> normals;

    glm::vec3 aabbMin{ 0.0f };
    glm::vec3 aabbMax{ 0.0f };
};