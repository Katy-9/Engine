#pragma once
#include "Component.h"

class ResourceMesh;

class ComponentMesh : public Component {
public:
    ComponentMesh(GameObject* owner);
    virtual ~ComponentMesh();

    // Cambia la malla que usa este componente, ajustando los contadores de referencias
    void SetResource(ResourceMesh* newResource);
    ResourceMesh* GetResource() const { return resource; }

    void Draw(unsigned int shaderProgram, bool forceWireframe = false);

public:
    bool showWireframe = false;
    bool showVertexNormals = false;
    bool showFaceNormals = false;

private:
    ResourceMesh* resource = nullptr;
};