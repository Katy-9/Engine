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

    void CreateFramebuffer(int width, int height);
    unsigned int sceneFBO = 0;
    unsigned int sceneTexture = 0;
    unsigned int sceneRBO = 0;
    int renderWidth = 1280;
    int renderHeight = 720;

public:
    SDL_GLContext context = nullptr;
    unsigned int defaultShaderProgram = 0;

    bool showGrid = true;
    bool showAxis = true;
    bool showAABB = false;
    bool vsync = true;
    bool wireframe = false;
    bool depthTest = true;
    bool cullFace = false;
    void SetVSync(bool enable);
    bool showRay = true;
    bool hasLastRay = false;
    glm::vec3 lastRayOrigin{ 0.0f };
    glm::vec3 lastRayEnd{ 0.0f };

private:
    void CreateDebugResources();
    void DrawAABB(const glm::vec3& mn, const glm::vec3& mx, const glm::vec3& color) const;
    void DrawAABBs(GameObject* go) const;

    unsigned int debugProgram = 0;
    unsigned int gridVAO = 0, gridVBO = 0;
    unsigned int aabbVAO = 0, aabbVBO = 0;
    int gridVertexCount = 0;
};