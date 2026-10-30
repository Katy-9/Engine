#pragma once
#include "Component.h"
#include <vector>

// Formato de vértice profesional: Posición (X,Y,Z) y Textura (U,V)
struct Vertex {
    float x, y, z;
    float u, v;
};

class ComponentMesh : public Component {
public:
    ComponentMesh(GameObject* owner);
    virtual ~ComponentMesh();

    unsigned int vbo = 0;
    unsigned int ebo = 0;
    unsigned int vao = 0;

    int numVertices = 0;
    int numIndices = 0;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices; // Lista de índices para construir los triángulos

    void GenerateBuffers();
    void Draw(unsigned int shaderProgram);

    bool showWireframe = false;
    bool showVertexNormals = false;
    bool showFaceNormals = false;
};