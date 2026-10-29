#include "Log.h"
#include <iostream>

std::vector<std::string> g_EngineLog;

void Log_Engine(const char file[], int line, const char* format, ...)
{
    static char tmpString[4096];
    va_list ap;
    va_start(ap, format);
    vsnprintf(tmpString, 4096, format, ap);
    va_end(ap);

    // Formatear el texto
    std::string logMessage = std::string(tmpString) + "\n";

    // Mostrar en la consola normal de Visual Studio
    std::cout << logMessage;

    // Guardarlo en nuestro vector para que la ventana de ImGui lo lea
    g_EngineLog.push_back(logMessage);
}