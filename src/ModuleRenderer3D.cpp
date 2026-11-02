#include "ModuleRenderer3D.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleCamera3D.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "ComponentMaterial.h"
#include "Log.h"

#include <vector>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

ModuleRenderer3D::ModuleRenderer3D(Application* app) : Module(app) {}
ModuleRenderer3D::~ModuleRenderer3D() {}

unsigned int checkerTextureID = 0;

bool ModuleRenderer3D::Init() {
    LOG("Creating 3D Renderer context");
    context = SDL_GL_CreateContext(App->window->window);
    if (!context) return false;
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) return false;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    // --- CREAR TEXTURA CHECKERBOARD (Ajedrez) ---
    const int CHECKERS_HEIGHT = 64;
    const int CHECKERS_WIDTH = 64;
    GLubyte checkerImage[CHECKERS_HEIGHT][CHECKERS_WIDTH][4];
    for (int i = 0; i < CHECKERS_HEIGHT; i++) {
        for (int j = 0; j < CHECKERS_WIDTH; j++) {
            int c = ((((i & 0x8) == 0) ^ (((j & 0x8)) == 0))) * 255;
            checkerImage[i][j][0] = (GLubyte)c; // R
            checkerImage[i][j][1] = (GLubyte)c; // G
            checkerImage[i][j][2] = (GLubyte)c; // B
            checkerImage[i][j][3] = (GLubyte)255; // Alpha
        }
    }

    glGenTextures(1, &checkerTextureID);
    glBindTexture(GL_TEXTURE_2D, checkerTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // Nearest para que no se vea borroso
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHECKERS_WIDTH, CHECKERS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);
    glBindTexture(GL_TEXTURE_2D, 0);

    // --- COMPILAR SHADER ---
    const char* vertexShaderSource = "#version 460 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "}\0";

    const char* fragmentShaderSource = "#version 460 core\n"
        "in vec2 TexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D ourTexture;\n"
        "uniform vec4 colorTint;\n"
        "uniform int hasTexture;\n"
        "void main() {\n"
        "   if(hasTexture == 1) {\n"
        "       FragColor = texture(ourTexture, TexCoord) * colorTint;\n"
        "   } else {\n"
        "       FragColor = colorTint;\n"
        "   }\n"
        "}\n\0";

    int success;
    char infoLog[512];

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(vertexShader, 512, NULL, infoLog); LOG("VERTEX SHADER ERROR: %s", infoLog); }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog); LOG("FRAGMENT SHADER ERROR: %s", infoLog); }

    defaultShaderProgram = glCreateProgram();
    glAttachShader(defaultShaderProgram, vertexShader);
    glAttachShader(defaultShaderProgram, fragmentShader);
    glLinkProgram(defaultShaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    SetVSync(vsync);

    CreateDebugResources();
    CreateFramebuffer(App->window->width, App->window->height);
    return true;
}
void ModuleRenderer3D::CreateFramebuffer(int width, int height) {
    if (width <= 0 || height <= 0) return;
    renderWidth = width;
    renderHeight = height;

    if (sceneFBO != 0) glDeleteFramebuffers(1, &sceneFBO);
    if (sceneTexture != 0) glDeleteTextures(1, &sceneTexture);
    if (sceneRBO != 0) glDeleteRenderbuffers(1, &sceneRBO);

    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderWidth, renderHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);

    glGenRenderbuffers(1, &sceneRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, renderWidth, renderHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH24_STENCIL8, GL_RENDERBUFFER, sceneRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DrawGameObject(GameObject* go, unsigned int shader, bool wireframe) {
    if (!go->active) return;

    ComponentMesh* mesh = go->GetComponent<ComponentMesh>(ComponentType::MESH);
    ComponentTransform* transform = go->GetComponent<ComponentTransform>(ComponentType::TRANSFORM);

    if (mesh && transform) {
        unsigned int modelLoc = glGetUniformLocation(shader, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform->GetGlobalMatrix()));

        ComponentMaterial* material = go->GetComponent<ComponentMaterial>(ComponentType::MATERIAL);
        if (material) {
            unsigned int tintLoc = glGetUniformLocation(shader, "colorTint");
            glUniform4fv(tintLoc, 1, material->colorTint);
            unsigned int hasTexLoc = glGetUniformLocation(shader, "hasTexture");

            // Lógica para elegir qué textura usar
            unsigned int textureToUse = material->textureID;
            if (material->useCheckerTexture) {
                textureToUse = checkerTextureID; // Usamos el ajedrez si el botón está pulsado
            }

            if (textureToUse != 0) {
                glUniform1i(hasTexLoc, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureToUse);
                glUniform1i(glGetUniformLocation(shader, "ourTexture"), 0);
            }
            else {
                glUniform1i(hasTexLoc, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }

        else {
            glUniform4f(glGetUniformLocation(shader, "colorTint"), 1.0f, 1.0f, 1.0f, 1.0f);
            glUniform1i(glGetUniformLocation(shader, "hasTexture"), 0);
        }

        mesh->Draw(shader, wireframe);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    for (GameObject* child : go->children) {
        DrawGameObject(child, shader, wireframe);
    }
}

static unsigned int CompileProgram(const char* vertexSrc, const char* fragmentSrc) {
    auto compile = [](GLenum type, const char* src) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        int ok;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[512]; glGetShaderInfoLog(shader, 512, nullptr, log); LOG("SHADER ERROR: %s", log); }
        return shader;
        };

    unsigned int vs = compile(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(program, 512, nullptr, log); LOG("SHADER LINK ERROR: %s", log); }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void ModuleRenderer3D::CreateDebugResources() {
    // Shader de líneas: posición + color por vértice
    debugProgram = CompileProgram(
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "uniform mat4 viewProj;\n"
        "out vec3 vColor;\n"
        "void main() { vColor = aColor; gl_Position = viewProj * vec4(aPos, 1.0); }\n",
        "#version 330 core\n"
        "in vec3 vColor;\n"
        "out vec4 FragColor;\n"
        "void main() { FragColor = vec4(vColor, 1.0); }\n");

    // Grid en el plano XZ + 3 ejes (x y z r g b por vértice)
    std::vector<float> v;
    auto line = [&v](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        v.insert(v.end(), { a.x, a.y, a.z, c.x, c.y, c.z, b.x, b.y, b.z, c.x, c.y, c.z });
        };

    const float half = 50.0f;
    for (int i = -(int)half; i <= (int)half; ++i) {
        if (i == 0) continue;   // las líneas centrales las ocupan los ejes X y Z
        line({ (float)i, 0.0f, -half }, { (float)i, 0.0f, half }, glm::vec3(0.35f));
        line({ -half, 0.0f, (float)i }, { half, 0.0f, (float)i }, glm::vec3(0.35f));
    }
    gridVertexCount = (int)(v.size() / 6);

    line({ -half, 0.0f, 0.0f }, { half, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });   // X rojo
    line({ 0.0f, 0.0f, 0.0f }, { 0.0f, 5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });    // Y verde
    line({ 0.0f, 0.0f, -half }, { 0.0f, 0.0f, half }, { 0.0f, 0.0f, 1.0f });   // Z azul

    auto setup = [](unsigned int& vao, unsigned int& vbo, const float* data, size_t bytes, GLenum usage) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, bytes, data, usage);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        };
    setup(gridVAO, gridVBO, v.data(), v.size() * sizeof(float), GL_STATIC_DRAW);
    setup(aabbVAO, aabbVBO, nullptr, 24 * 6 * sizeof(float), GL_DYNAMIC_DRAW);   // buffer reutilizable para las cajas
    glBindVertexArray(0);
}

void ModuleRenderer3D::DrawAABB(const glm::vec3& mn, const glm::vec3& mx, const glm::vec3& color) const {
    auto corner = [&](int i) {
        return glm::vec3((i & 1) ? mx.x : mn.x, (i & 2) ? mx.y : mn.y, (i & 4) ? mx.z : mn.z);
        };

    float v[24 * 6];
    int n = 0;
    for (int i = 0; i < 8; ++i) {
        for (int bit : { 1, 2, 4 }) {
            if (i & bit) continue;   // cada arista = dos esquinas que difieren en un solo bit (12 aristas)
            for (int k : { i, i | bit }) {
                const glm::vec3 p = corner(k);
                v[n++] = p.x; v[n++] = p.y; v[n++] = p.z;
                v[n++] = color.x; v[n++] = color.y; v[n++] = color.z;
            }
        }
    }

    glBindVertexArray(aabbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, aabbVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_LINES, 0, 24);
}

void ModuleRenderer3D::DrawAABBs(GameObject* go) const {
    if (!go->active) return;

    glm::vec3 mn, mx;
    if (go->GetComponent<ComponentMesh>(ComponentType::MESH) && go->GetWorldAABB(mn, mx)) {
        const glm::vec3 color = (go == App->scene->selected) ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(0.6f);
        DrawAABB(mn, mx, color);   // amarillo si es el seleccionado, gris el resto
    }
    for (GameObject* child : go->children) DrawAABBs(child);
}

update_status ModuleRenderer3D::PreUpdate(float dt) {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, renderWidth, renderHeight);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return UPDATE_CONTINUE;
}

void ModuleRenderer3D::SetVSync(bool enable) {
    vsync = enable;
    if (!SDL_GL_SetSwapInterval(enable ? 1 : 0)) LOG("VSync not supported: %s", SDL_GetError());
}

update_status ModuleRenderer3D::PostUpdate(float dt) {
    const glm::mat4 view = App->camera->GetViewMatrix();
    const glm::mat4 projection = App->camera->GetProjectionMatrix();

    if (depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

    // --- ESCENA ---
    glUseProgram(defaultShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(defaultShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(defaultShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    if (App->scene->GetRoot()) {
        DrawGameObject(App->scene->GetRoot(), defaultShaderProgram, wireframe);
    }

    // --- AYUDAS VISUALES: grid, ejes y cajas ---
    glUseProgram(debugProgram);
    glUniformMatrix4fv(glGetUniformLocation(debugProgram, "viewProj"), 1, GL_FALSE, glm::value_ptr(projection * view));

    glBindVertexArray(gridVAO);
    if (showGrid) glDrawArrays(GL_LINES, 0, gridVertexCount);
    if (showAxis) glDrawArrays(GL_LINES, gridVertexCount, 6);

    if (showAABB && App->scene->GetRoot()) DrawAABBs(App->scene->GetRoot());

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, App->window->width, App->window->height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return UPDATE_CONTINUE;
}

bool ModuleRenderer3D::CleanUp() {
    if (sceneFBO != 0) glDeleteFramebuffers(1, &sceneFBO);
    if (sceneTexture != 0) glDeleteTextures(1, &sceneTexture);
    if (sceneRBO != 0) glDeleteRenderbuffers(1, &sceneRBO);
    glDeleteProgram(defaultShaderProgram);
    glDeleteProgram(debugProgram);
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteVertexArrays(1, &aabbVAO);
    glDeleteBuffers(1, &aabbVBO);
    SDL_GL_DestroyContext(context);
    return true;
}