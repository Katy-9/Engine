#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentTransform.h"
#include <cfloat>

GameObject::GameObject(std::string name, GameObject* parent) : name(name), parent(parent) {
    if (parent != nullptr) {
        parent->children.push_back(this);
    }
}

GameObject::~GameObject() {
    for (auto* comp : components) {
        delete comp;
    }
    for (auto* child : children) {
        delete child;
    }
}

void GameObject::Update(float dt) {
    if (!active) return;

    for (auto* comp : components) {
        if (comp->IsActive()) {
            comp->Update(dt);
        }
    }

    for (auto* child : children) {
        child->Update(dt);
    }
}

static void AccumulateAABB(GameObject* go, glm::vec3& mn, glm::vec3& mx, bool& found) {
    if (!go->active) return;

    ComponentMesh* mesh = go->GetComponent<ComponentMesh>(ComponentType::MESH);
    ComponentTransform* transform = go->GetComponent<ComponentTransform>(ComponentType::TRANSFORM);

    if (mesh && transform) {
        const glm::mat4 m = transform->GetGlobalMatrix();
        for (int i = 0; i < 8; ++i) {
            const glm::vec3 corner((i & 1) ? mesh->aabbMax.x : mesh->aabbMin.x,
                (i & 2) ? mesh->aabbMax.y : mesh->aabbMin.y,
                (i & 4) ? mesh->aabbMax.z : mesh->aabbMin.z);
            const glm::vec3 world = glm::vec3(m * glm::vec4(corner, 1.0f));
            mn = glm::min(mn, world);
            mx = glm::max(mx, world);
            found = true;
        }
    }
    for (GameObject* child : go->children) AccumulateAABB(child, mn, mx, found);
}

bool GameObject::GetWorldAABB(glm::vec3& outMin, glm::vec3& outMax) {
    glm::vec3 mn(FLT_MAX), mx(-FLT_MAX);
    bool found = false;
    AccumulateAABB(this, mn, mx, found);
    if (found) { outMin = mn; outMax = mx; }
    return found;
}
GameObject* GameObject::Clone(GameObject* newParent) {
    GameObject* parentToUse = (newParent != nullptr) ? newParent : this->parent;
    GameObject* clone = new GameObject(this->name + " (Copy)", parentToUse);
    clone->active = this->active;

    ComponentTransform* srcTrans = GetComponent<ComponentTransform>(ComponentType::TRANSFORM);
    if (srcTrans != nullptr) {
        ComponentTransform* dstTrans = clone->CreateComponent<ComponentTransform>(ComponentType::TRANSFORM);
        dstTrans->position = srcTrans->position + glm::vec3(1.0f, 0.0f, 1.0f);
        dstTrans->rotation = srcTrans->rotation;
        dstTrans->eulerRotation = srcTrans->eulerRotation;
        dstTrans->scale = srcTrans->scale;
    }
    ComponentMaterial* srcMat = GetComponent<ComponentMaterial>(ComponentType::MATERIAL);
    if (srcMat != nullptr) {
        ComponentMaterial* dstMat = clone->CreateComponent<ComponentMaterial>(ComponentType::MATERIAL);
        dstMat->texturePath = srcMat->texturePath;
        dstMat->textureWidth = srcMat->textureWidth;
        dstMat->textureHeight = srcMat->textureHeight;
        dstMat->textureID = srcMat->textureID;
        dstMat->useCheckerTexture = srcMat->useCheckerTexture;
        for (int i = 0; i < 4; ++i) {
            dstMat->colorTint[i] = srcMat->colorTint[i];
        }
    }
    ComponentMesh* srcMesh = GetComponent<ComponentMesh>(ComponentType::MESH);
    if (srcMesh != nullptr) {
        ComponentMesh* dstMesh = clone->CreateComponent<ComponentMesh>(ComponentType::MESH);
        dstMesh->vertices = srcMesh->vertices;
        dstMesh->indices = srcMesh->indices;
        dstMesh->normals = srcMesh->normals;
        dstMesh->numVertices = srcMesh->numVertices;
        dstMesh->numIndices = srcMesh->numIndices;
        dstMesh->showWireframe = srcMesh->showWireframe;
        dstMesh->showVertexNormals = srcMesh->showVertexNormals;
        dstMesh->showFaceNormals = srcMesh->showFaceNormals;
        dstMesh->GenerateBuffers();
    }

    for (GameObject* child : this->children) {
        child->Clone(clone);
    }

    return clone;
}