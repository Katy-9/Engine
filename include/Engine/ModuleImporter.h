#pragma once
#include "Module.h"
#include <string>

class GameObject;

class ModuleImporter : public Module {
public:
    ModuleImporter(Application* app);
    virtual ~ModuleImporter();

    bool Init() override;
    bool CleanUp() override;

    void LoadFBX(const std::string& path);
    void LoadTexture(const std::string& path, GameObject* target);
};