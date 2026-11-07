#include "ComponentMaterial.h"
#include "ResourceTexture.h"
#include "ModuleResources.h"
#include "Application.h"

extern Application* App;

ComponentMaterial::~ComponentMaterial() {
    SetTexture(nullptr);
}

void ComponentMaterial::SetTexture(ResourceTexture* newTexture) {
    if (newTexture == texture) return;
    if (newTexture) App->resources->AddReference(newTexture);
    if (texture) App->resources->ReleaseReference(texture);
    texture = newTexture;
}

unsigned int ComponentMaterial::GetTextureID() const {
    return texture ? texture->id : 0;
}