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
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

ModuleRenderer3D::ModuleRenderer3D(Application* app) : Module(app) {}
ModuleRenderer3D::~ModuleRenderer3D() {}

bool ModuleRenderer3D::Init() {
    LOG("Creating 3D Renderer context");
    context = SDL_GL_CreateContext(App->window->window);
    if (!context) return false;
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) return false;

    glEnable(GL_DEPTH_TEST);
    // IMPORTANTE: Quitamos el CULL FACE temporalmente para poder ver la casa por dentro y fuera
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    // --- SHADER MEJORADO ---
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
        "uniform int hasTexture;\n" // <--- NUEVO: Chivato para saber si hay textura
        "void main() {\n"
        "   if(hasTexture == 1) {\n"
        "       FragColor = texture(ourTexture, TexCoord) * colorTint;\n"
        "   } else {\n"
        "       FragColor = colorTint;\n" // Si no hay textura, usamos el color puro
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

    return true;
}

void DrawGameObject(GameObject* go, unsigned int shader) {
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

            // Si tenemos una textura cargada
            if (material->textureID != 0) {
                glUniform1i(hasTexLoc, 1); // Le decimos al shader: SÍ HAY TEXTURA

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, material->textureID);
                glUniform1i(glGetUniformLocation(shader, "ourTexture"), 0);
            }
            else {
                glUniform1i(hasTexLoc, 0); // Le decimos al shader: NO HAY TEXTURA
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }

        mesh->Draw(shader);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    for (GameObject* child : go->children) {
        DrawGameObject(child, shader);
    }
}

update_status ModuleRenderer3D::PreUpdate(float dt) {
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(defaultShaderProgram);

    glm::mat4 view = App->camera->GetViewMatrix();
    glm::mat4 projection = App->camera->GetProjectionMatrix();

    unsigned int viewLoc = glGetUniformLocation(defaultShaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(defaultShaderProgram, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    if (App->scene->GetRoot()) {
        DrawGameObject(App->scene->GetRoot(), defaultShaderProgram);
    }

    glUseProgram(0);

    return UPDATE_CONTINUE;
}

update_status ModuleRenderer3D::PostUpdate(float dt) {
    return UPDATE_CONTINUE;
}

bool ModuleRenderer3D::CleanUp() {
    glDeleteProgram(defaultShaderProgram);
    SDL_GL_DestroyContext(context);
    return true;
}