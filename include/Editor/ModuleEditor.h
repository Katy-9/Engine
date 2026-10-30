#pragma once
#include "Module.h"

class GameObject;

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
    update_status DrawMainMenuBar();
    void DrawConsole();
    void DrawHierarchy();
    void DrawHierarchyNode(GameObject* go);
    void DrawAboutWindow();
    void DrawConfiguration();
    void DrawInspector();

private:
    bool showAboutWindow = false;
    bool showConfigurationWindow = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showConsole = true;
};