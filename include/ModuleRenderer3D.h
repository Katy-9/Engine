#pragma once
#include "Module.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

class GameObject;

class ModuleRenderer3D : public Module {
public:
    ModuleRenderer3D(Application* app);
    virtual ~ModuleRenderer3D();

    bool Init() override;
    update_status PreUpdate(float dt) override;
    update_status PostUpdate(float dt) override;
    bool CleanUp() override;

public:
    SDL_GLContext context = nullptr;
    unsigned int defaultShaderProgram = 0;

    // Ayudas visuales (el editor las activa desde Configuration)
    bool showGrid = true;
    bool showAxis = true;
    bool showAABB = false;
    bool vsync = true;
    bool wireframe = false;    // wireframe global (el de cada objeto sigue en el Inspector)
    bool depthTest = true;
    bool cullFace = false;
    void SetVSync(bool enable);

private:
    void CreateDebugResources();
    void DrawAABB(const glm::vec3& mn, const glm::vec3& mx, const glm::vec3& color) const;
    void DrawAABBs(GameObject* go) const;

    unsigned int debugProgram = 0;
    unsigned int gridVAO = 0, gridVBO = 0;
    unsigned int aabbVAO = 0, aabbVBO = 0;
    int gridVertexCount = 0;   // los 6 vértices de los ejes van justo después
};