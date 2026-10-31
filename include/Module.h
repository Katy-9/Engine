#pragma once
#include "Globals.h"

class Application;

class Module {
public:
    Module(Application* parent) : App(parent) {}
    virtual ~Module() {}

    virtual bool Init() { return true; }
    virtual update_status PreUpdate(float dt) { return UPDATE_CONTINUE; }
    virtual update_status Update(float dt) { return UPDATE_CONTINUE; }
    virtual update_status PostUpdate(float dt) { return UPDATE_CONTINUE; }
    virtual bool CleanUp() { return true; }

protected:
    Application* App;
};