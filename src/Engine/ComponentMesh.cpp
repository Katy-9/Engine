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
    const auto pos = [](const Vertex& v) { return glm::vec3(v.x, v.y, v.z); };

    // Si el importador no dio normales (primitivas), las calculamos sumando las normales de las caras vecinas
    if (normals.size() != vertices.size()) {
        normals.assign(vertices.size(), glm::vec3(0.0f));
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            const glm::vec3 p0 = pos(vertices[indices[i]]);
            const glm::vec3 n = glm::cross(pos(vertices[indices[i + 1]]) - p0, pos(vertices[indices[i + 2]]) - p0);
            normals[indices[i]] += n;
            normals[indices[i + 1]] += n;
            normals[indices[i + 2]] += n;
        }
        for (glm::vec3& n : normals) {
            n = glm::length(n) > 1e-6f ? glm::normalize(n) : glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    // Las líneas miden una fracción del tamaño del modelo, así que se ven bien en una casa y en un cubo
    const float modelSize = glm::length(aabbMax - aabbMin);

    // 2. GENERAR BUFFER PARA NORMALES DE VÉRTICE (Lineas Verdes)
    std::vector<float> v_normals;
    v_normals.reserve(vertices.size() * 6);
    const float vertexLen = 0.05f * modelSize;
    for (size_t i = 0; i < vertices.size(); i++) {
        const glm::vec3 p = pos(vertices[i]);
        const glm::vec3 end = p + normals[i] * vertexLen;
        v_normals.insert(v_normals.end(), { p.x, p.y, p.z, end.x, end.y, end.z });
    }
    numNormalVertices = (int)(v_normals.size() / 3);

    glGenVertexArrays(1, &vao_normals);
    glBindVertexArray(vao_normals);
    glGenBuffers(1, &vbo_normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, v_normals.size() * sizeof(float), v_normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // 3. GENERAR BUFFER PARA NORMALES DE CARA (Líneas Azules): del centro de cada triángulo hacia fuera
    std::vector<float> f_normals;
    const float faceLen = 0.08f * modelSize;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const glm::vec3 v0 = pos(vertices[indices[i]]);
        const glm::vec3 v1 = pos(vertices[indices[i + 1]]);
        const glm::vec3 v2 = pos(vertices[indices[i + 2]]);

        const glm::vec3 cross = glm::cross(v1 - v0, v2 - v0);
        if (glm::length(cross) < 1e-8f) continue;   // triángulo degenerado: normalize daría NaN

        const glm::vec3 center = (v0 + v1 + v2) / 3.0f;
        const glm::vec3 end = center + glm::normalize(cross) * faceLen;
        f_normals.insert(f_normals.end(), { center.x, center.y, center.z, end.x, end.y, end.z });
    }
    numFaceNormalVertices = (int)(f_normals.size() / 3);

    glGenVertexArrays(1, &vao_face_normals);
    glBindVertexArray(vao_face_normals);
    glGenBuffers(1, &vbo_face_normals);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_face_normals);
    glBufferData(GL_ARRAY_BUFFER, f_normals.size() * sizeof(float), f_normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void ComponentMesh::Draw(unsigned int shaderProgram, bool forceWireframe) {
    if (vao == 0) return;

    // --- DIBUJAR MALLA PRINCIPAL ---
    glBindVertexArray(vao);

    glPolygonMode(GL_FRONT_AND_BACK, (showWireframe || forceWireframe) ? GL_LINE : GL_FILL);

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
            glUniform4fv(tintLoc, 1, green);

            glBindVertexArray(vao_normals);
            glDrawArrays(GL_LINES, 0, numNormalVertices);
        }

        if (showFaceNormals && vao_face_normals != 0) {
            float blue[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
            glUniform4fv(tintLoc, 1, blue);

            glBindVertexArray(vao_face_normals);
            glDrawArrays(GL_LINES, 0, numFaceNormalVertices);
        }

        glBindVertexArray(0);
    }
}