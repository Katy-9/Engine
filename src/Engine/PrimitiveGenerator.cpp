#include "PrimitiveGenerator.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace PrimitiveGenerator {

    // Función auxiliar interna para preparar un GameObject nuevo con todos sus componentes
    GameObject* CreateBasePrimitive(const char* name, GameObject* parent) {
        GameObject* go = new GameObject(name, parent);
        go->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);
        go->CreateComponent<ComponentMaterial>(ComponentType::MATERIAL);
        return go;
    }

    GameObject* CreateCube(GameObject* parent) {
        GameObject* go = CreateBasePrimitive("Cube", parent);
        ComponentMesh* mesh = go->CreateComponent<ComponentMesh>(ComponentType::MESH);

        // Los 8 vértices de un cubo. Usamos 24 vértices para mapear bien las texturas (UVs) por cada cara.
        float v[] = {
            // Front face
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            // Back face
             0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
             // Left face
             -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
             -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
             -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
             -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
             // Right face
              0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
              0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
              0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
              0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
              // Top face
              -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
               0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
               0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
              -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
              // Bottom face
              -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
               0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
               0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
              -0.5f, -0.5f,  0.5f,  0.0f, 1.0f
        };

        unsigned int ind[] = {
            0,  1,  2,      0,  2,  3,    // Front
            4,  5,  6,      4,  6,  7,    // Back
            8,  9,  10,     8,  10, 11,   // Left
            12, 13, 14,     12, 14, 15,   // Right
            16, 17, 18,     16, 18, 19,   // Top
            20, 21, 22,     20, 22, 23    // Bottom
        };

        for (int i = 0; i < 24; i++) {
            mesh->vertices.push_back({ v[i * 5], v[i * 5 + 1], v[i * 5 + 2], v[i * 5 + 3], v[i * 5 + 4] });
        }
        for (int i = 0; i < 36; i++) {
            mesh->indices.push_back(ind[i]);
        }

        mesh->numVertices = 24;
        mesh->numIndices = 36;
        mesh->GenerateBuffers();
        return go;
    }

    GameObject* CreatePyramid(GameObject* parent) {
        GameObject* go = CreateBasePrimitive("Pyramid", parent);
        ComponentMesh* mesh = go->CreateComponent<ComponentMesh>(ComponentType::MESH);

        float v[] = {
            // Base
            -0.5f, 0.0f, -0.5f,  0.0f, 0.0f,
             0.5f, 0.0f, -0.5f,  1.0f, 0.0f,
             0.5f, 0.0f,  0.5f,  1.0f, 1.0f,
            -0.5f, 0.0f,  0.5f,  0.0f, 1.0f,
            // Top point (para todas las caras)
             0.0f, 1.0f,  0.0f,  0.5f, 1.0f
        };
        unsigned int ind[] = {
            0, 2, 1,   0, 3, 2, // Base
            0, 1, 4,            // Front
            1, 2, 4,            // Right
            2, 3, 4,            // Back
            3, 0, 4             // Left
        };

        for (int i = 0; i < 5; i++) mesh->vertices.push_back({ v[i * 5], v[i * 5 + 1], v[i * 5 + 2], v[i * 5 + 3], v[i * 5 + 4] });
        for (int i = 0; i < 18; i++) mesh->indices.push_back(ind[i]);

        mesh->numVertices = 5;
        mesh->numIndices = 18;
        mesh->GenerateBuffers();
        return go;
    }

    GameObject* CreateSphere(int segments, int rings, GameObject* parent) {
        GameObject* go = CreateBasePrimitive("Sphere", parent);
        ComponentMesh* mesh = go->CreateComponent<ComponentMesh>(ComponentType::MESH);

        float radius = 0.5f;
        for (int y = 0; y <= rings; ++y) {
            for (int x = 0; x <= segments; ++x) {
                float xSegment = (float)x / (float)segments;
                float ySegment = (float)y / (float)rings;
                float xPos = radius * std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
                float yPos = radius * std::cos(ySegment * M_PI);
                float zPos = radius * std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

                mesh->vertices.push_back({ xPos, yPos, zPos, xSegment, ySegment });
            }
        }

        for (int y = 0; y < rings; ++y) {
            for (int x = 0; x < segments; ++x) {
                mesh->indices.push_back((y + 1) * (segments + 1) + x);
                mesh->indices.push_back(y * (segments + 1) + x);
                mesh->indices.push_back(y * (segments + 1) + x + 1);

                mesh->indices.push_back((y + 1) * (segments + 1) + x);
                mesh->indices.push_back(y * (segments + 1) + x + 1);
                mesh->indices.push_back((y + 1) * (segments + 1) + x + 1);
            }
        }
        mesh->numVertices = mesh->vertices.size();
        mesh->numIndices = mesh->indices.size();
        mesh->GenerateBuffers();
        return go;
    }

    GameObject* CreateCylinder(int segments, GameObject* parent) {
        GameObject* go = CreateBasePrimitive("Cylinder", parent);
        ComponentMesh* mesh = go->CreateComponent<ComponentMesh>(ComponentType::MESH);

        float radius = 0.5f;
        float height = 1.0f;

        // Creamos los vértices a lo largo del círculo
        for (int i = 0; i <= segments; ++i) {
            float theta = (float)i / (float)segments * 2.0f * M_PI;
            float x = radius * std::cos(theta);
            float z = radius * std::sin(theta);
            float u = (float)i / (float)segments;

            // Vértice de arriba y vértice de abajo
            mesh->vertices.push_back({ x,  height / 2.0f, z, u, 1.0f });
            mesh->vertices.push_back({ x, -height / 2.0f, z, u, 0.0f });
        }

        // Conectar los vértices para hacer las caras laterales
        for (int i = 0; i < segments; ++i) {
            int top1 = i * 2;
            int bot1 = i * 2 + 1;
            int top2 = (i + 1) * 2;
            int bot2 = (i + 1) * 2 + 1;

            mesh->indices.push_back(top1); mesh->indices.push_back(bot1); mesh->indices.push_back(top2);
            mesh->indices.push_back(top2); mesh->indices.push_back(bot1); mesh->indices.push_back(bot2);
        }

        mesh->numVertices = mesh->vertices.size();
        mesh->numIndices = mesh->indices.size();
        mesh->GenerateBuffers();
        return go;
    }
}