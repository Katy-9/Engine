#include "Application.h"
#include "Log.h"

int main(int argc, char* argv[]) {
    LOG("Engine starting...");
    Application* App = new Application();

    if (!App->Init()) {
        LOG("Engine Initialization Failed!");
        delete App;
        return -1;
    }

    // BUCLE PRINCIPAL (Game Loop)
    update_status state = UPDATE_CONTINUE;
    while (state == UPDATE_CONTINUE) {
        state = App->Update();
    }

    LOG("Engine CleanUp...");
    App->CleanUp();
    delete App;

    LOG("Exiting successfully.");
    return 0;
}