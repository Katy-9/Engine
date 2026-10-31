#pragma once
#include "Component.h"
#include <vector>
#include <glm/glm.hpp>

struct Vertex {
    float x, y, z;
    float u, v;
};

class ComponentMesh : public Component {
public:
    ComponentMesh(GameObject* owner);
    virtual ~ComponentMesh();

    // Buffers para la malla principal
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    unsigned int vao = 0;

    // --- Buffers para las líneas de las Normales ---
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

    void GenerateBuffers();
    void Draw(unsigned int shaderProgram, bool forceWireframe = false);

    bool showWireframe = false;
    bool showVertexNormals = false;
    bool showFaceNormals = false;

    glm::vec3 aabbMin{ 0.0f };
    glm::vec3 aabbMax{ 0.0f };

    std::vector<glm::vec3> normals;
};