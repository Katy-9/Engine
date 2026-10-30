#include "ComponentMesh.h"
#include "Log.h"
#include <glad/glad.h>
#include <stddef.h> // Para la macro offsetof

ComponentMesh::ComponentMesh(GameObject* owner) : Component(owner, ComponentType::MESH) {}

ComponentMesh::~ComponentMesh() {
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (ebo != 0) glDeleteBuffers(1, &ebo);
    if (vao != 0) glDeleteVertexArrays(1, &vao);
}

void ComponentMesh::GenerateBuffers() {
    if (vertices.empty()) return;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    if (!indices.empty()) {
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }

    // Atributo 0: Posición (X, Y, Z)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Atributo 1: Coordenadas de textura (U, V)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glBindVertexArray(0);
}

void ComponentMesh::Draw(unsigned int shaderProgram) {
    if (vao == 0) return;

    glBindVertexArray(vao);

    if (showWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Dibujamos SIEMPRE usando glDrawElements, ya que cargamos los índices en el importador
    if (!indices.empty()) {
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}