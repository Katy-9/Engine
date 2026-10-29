#pragma once
#include "Module.h"

class ModuleEditor : public Module {
public:
    ModuleEditor(Application* app);
    virtual ~ModuleEditor();

    bool Init() override;
    update_status PreUpdate(float dt) override;
    update_status Update(float dt) override;
    update_status PostUpdate(float dt) override;
    bool CleanUp() override;

private:
    void DrawMainMenuBar();
    void DrawConsole();
};