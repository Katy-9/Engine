#include "ComponentMesh.h"
#include "ResourceMesh.h"
#include "ModuleResources.h"
#include "Application.h"

extern Application* App;

ComponentMesh::ComponentMesh(GameObject* owner) : Component(owner, ComponentType::MESH) {}

ComponentMesh::~ComponentMesh() {
    SetResource(nullptr);
}

void ComponentMesh::SetResource(ResourceMesh* newResource) {
    if (newResource == resource) return;
    if (newResource) App->resources->AddReference(newResource);
    if (resource) App->resources->ReleaseReference(resource);
    resource = newResource;
}

void ComponentMesh::Draw(unsigned int shaderProgram, bool forceWireframe) {
    if (resource == nullptr || resource->vao == 0) return;

    resource->DrawMesh(showWireframe || forceWireframe);

    if (showVertexNormals || showFaceNormals) {
        resource->DrawNormals(shaderProgram, showVertexNormals, showFaceNormals);
    }
}