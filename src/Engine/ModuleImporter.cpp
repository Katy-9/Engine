#include "ModuleImporter.h"
#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "ComponentMaterial.h"
#include "Log.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <glad/glad.h>

ModuleImporter::ModuleImporter(Application* app) : Module(app) {}
ModuleImporter::~ModuleImporter() {}

bool ModuleImporter::Init() {
    LOG("Initializing Importer Module (Assimp & DevIL)");
    ilInit();
    iluInit();

    // Obligamos a DevIL a leer las texturas al derecho para OpenGL
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    return true;
}

bool ModuleImporter::CleanUp() {
    return true;
}

void ModuleImporter::LoadFBX(const std::string& path) {
    LOG("Attempting to load FBX: %s", path.c_str());

    const aiScene* scene = aiImportFile(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

    if (scene != nullptr && scene->HasMeshes()) {
        LOG("FBX Loaded Successfully! Meshes found: %d", scene->mNumMeshes);

        GameObject* rootFBX = new GameObject("FBX_Model", App->scene->GetRoot());
        rootFBX->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh* ai_mesh = scene->mMeshes[i];

            GameObject* childObj = new GameObject(ai_mesh->mName.C_Str(), rootFBX);
            childObj->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);
            childObj->CreateComponent<ComponentMaterial>(ComponentType::MATERIAL);

            ComponentMesh* myMesh = childObj->CreateComponent<ComponentMesh>(ComponentType::MESH);
            myMesh->numVertices = ai_mesh->mNumVertices;
            myMesh->vertices.resize(myMesh->numVertices);

            for (unsigned int v = 0; v < myMesh->numVertices; ++v) {
                myMesh->vertices[v].x = ai_mesh->mVertices[v].x;
                myMesh->vertices[v].y = ai_mesh->mVertices[v].y;
                myMesh->vertices[v].z = ai_mesh->mVertices[v].z;

                if (ai_mesh->HasTextureCoords(0)) {
                    myMesh->vertices[v].u = ai_mesh->mTextureCoords[0][v].x;
                    myMesh->vertices[v].v = ai_mesh->mTextureCoords[0][v].y;
                }
                else {
                    myMesh->vertices[v].u = 0.0f;
                    myMesh->vertices[v].v = 0.0f;
                }
            }

            if (ai_mesh->HasFaces()) {
                myMesh->numIndices = ai_mesh->mNumFaces * 3;
                myMesh->indices.resize(myMesh->numIndices);

                for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
                    if (ai_mesh->mFaces[f].mNumIndices == 3) {
                        myMesh->indices[(f * 3)] = ai_mesh->mFaces[f].mIndices[0];
                        myMesh->indices[(f * 3) + 1] = ai_mesh->mFaces[f].mIndices[1];
                        myMesh->indices[(f * 3) + 2] = ai_mesh->mFaces[f].mIndices[2];
                    }
                }
            }

            myMesh->GenerateBuffers();
            LOG("-> Mesh '%s' loaded. Vertices: %d, Indices: %d", ai_mesh->mName.C_Str(), myMesh->numVertices, myMesh->numIndices);
        }

        aiReleaseImport(scene);
    }
    else {
        LOG("ERROR Loading FBX: Could not open %s", path.c_str());
    }
}

void ModuleImporter::LoadTexture(const std::string& path, GameObject* target) {
    if (target == nullptr) {
        LOG("No GameObject selected to apply the texture!");
        return;
    }

    ComponentMaterial* material = target->GetComponent<ComponentMaterial>(ComponentType::MATERIAL);
    if (material == nullptr) {
        LOG("Selected GameObject does not have a Material Component!");
        return;
    }

    unsigned int imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (ilLoadImage(path.c_str())) {
        ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

        material->textureWidth = ilGetInteger(IL_IMAGE_WIDTH);
        material->textureHeight = ilGetInteger(IL_IMAGE_HEIGHT);
        material->texturePath = path;

        if (material->textureID != 0) glDeleteTextures(1, &material->textureID);

        glGenTextures(1, &material->textureID);
        glBindTexture(GL_TEXTURE_2D, material->textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, material->textureWidth, material->textureHeight,
            0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());
        glGenerateMipmap(GL_TEXTURE_2D);

        LOG("Texture loaded successfully: %s (%dx%d)", path.c_str(), material->textureWidth, material->textureHeight);
    }
    else {
        LOG("Failed to load texture: %s", path.c_str());
    }

    ilDeleteImages(1, &imageID);
}