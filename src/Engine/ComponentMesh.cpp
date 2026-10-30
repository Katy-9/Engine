#include "ComponentMesh.h"
#include "Log.h"
#include <glad/glad.h>
#include <stddef.h> 
#include <glm/glm.hpp>

ComponentMesh::ComponentMesh(GameObject* owner) : Component(owner, ComponentType::MESH) {}

ComponentMesh::~ComponentMesh() {
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (ebo != 0) glDeleteBuffers(1, &ebo);
    if (vao != 0) glDeleteVertexArrays(1, &vao);

    // Limpiar las normales
    if (vbo_normals != 0) glDeleteBuffers(1, &vbo_normals);
    if (vao_normals != 0) glDeleteVertexArrays(1, &vao_normals);
    if (vbo_face_normals != 0) glDeleteBuffers(1, &vbo_face_normals);
    if (vao_face_normals != 0) glDeleteVertexArrays(1, &vao_face_normals);
}

void ComponentMesh::GenerateBuffers() {
    if (vertices.empty()) return;

    // AABB local: el vértice más lejano en cada eje
    aabbMin = aabbMax = glm::vec3(vertices[0].x, vertices[0].y, vertices[0].z);
    for (const Vertex& v : vertices) {
        aabbMin = glm::min(aabbMin, glm::vec3(v.x, v.y, v.z));
        aabbMax = glm::max(aabbMax, glm::vec3(v.x, v.y, v.z));
    }

    // 1. GENERAR BUFFER DE LA MALLA PRINCIPAL
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
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
    glBindVertexArray(0);

    // 2. GENERAR BUFFER PARA NORMALES DE VÉRTICE (Lineas Verdes)
    std::vector<float> v_normals;
    for (size_t i = 0; i < vertices.size(); i++) {
        glm::vec3 p(vertices[i].x, vertices[i].y, vertices[i].z);
        glm::vec3 dir = glm::normalize(p);
        if (glm::length(dir) < 0.1f) dir = glm::vec3(0, 1, 0); // Evitar errores en el origen

        // Punto inicial y punto final de la raya
        v_normals.push_back(p.x); v_normals.push_back(p.y); v_normals.push_back(p.z);
        v_normals.push_back(p.x + dir.x * 1.5f); v_normals.push_back(p.y + dir.y * 1.5f); v_normals.push_back(p.z + dir.z * 1.5f);
    }
    numNormalVertices = v_normals.size() / 3;

    glGenVertexArrays(1, &vao_normals);
    glBindVertexArray(vao_normals);
    glGenBuffers(1, &vbo_normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, v_normals.size() * sizeof(float), v_normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // 3. GENERAR BUFFER PARA NORMALES DE CARA (Líneas Azules)
    std::vector<float> f_normals;
    if (!indices.empty()) {
        for (size_t i = 0; i < indices.size(); i += 3) {
            glm::vec3 v0(vertices[indices[i]].x, vertices[indices[i]].y, vertices[indices[i]].z);
            glm::vec3 v1(vertices[indices[i + 1]].x, vertices[indices[i + 1]].y, vertices[indices[i + 1]].z);
            glm::vec3 v2(vertices[indices[i + 2]].x, vertices[indices[i + 2]].y, vertices[indices[i + 2]].z);

            glm::vec3 center = (v0 + v1 + v2) / 3.0f;
            glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0)); // Producto vectorial real

            f_normals.push_back(center.x); f_normals.push_back(center.y); f_normals.push_back(center.z);
            f_normals.push_back(center.x + faceNormal.x * 2.0f);
            f_normals.push_back(center.y + faceNormal.y * 2.0f);
            f_normals.push_back(center.z + faceNormal.z * 2.0f);
        }
    }
    numFaceNormalVertices = f_normals.size() / 3;

    glGenVertexArrays(1, &vao_face_normals);
    glBindVertexArray(vao_face_normals);
    glGenBuffers(1, &vbo_face_normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_face_normals);
    glBufferData(GL_ARRAY_BUFFER, f_normals.size() * sizeof(float), f_normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void ComponentMesh::Draw(unsigned int shaderProgram) {
    if (vao == 0) return;

    // --- DIBUJAR MALLA PRINCIPAL ---
    glBindVertexArray(vao);
    if (showWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (!indices.empty()) {
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
    }
    else {
        glDrawArrays(GL_TRIANGLES, 0, numVertices);
    }
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


    // --- DIBUJAR NORMALES CON EL MISMO SHADER MODERNO ---
    if (showVertexNormals || showFaceNormals) {

        // Le decimos al shader que apague las texturas para pintar las líneas de color puro
        unsigned int hasTexLoc = glGetUniformLocation(shaderProgram, "hasTexture");
        glUniform1i(hasTexLoc, 0);
        unsigned int tintLoc = glGetUniformLocation(shaderProgram, "colorTint");

        if (showVertexNormals && vao_normals != 0) {
            float green[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
            glUniform4fv(tintLoc, 1, green); // Cambiamos el color a Verde

            glBindVertexArray(vao_normals);
            glDrawArrays(GL_LINES, 0, numNormalVertices); // GL_LINES en vez de GL_TRIANGLES
        }

        if (showFaceNormals && vao_face_normals != 0) {
            float blue[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
            glUniform4fv(tintLoc, 1, blue); // Cambiamos el color a Azul

            glBindVertexArray(vao_face_normals);
            glDrawArrays(GL_LINES, 0, numFaceNormalVertices);
        }

        glBindVertexArray(0);
    }
}