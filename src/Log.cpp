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

    // Quitamos saltos de línea finales: los mensajes de Assimp ya traen uno y dejaban líneas en blanco
    std::string message = tmpString;
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) message.pop_back();

    std::cout << message << "\n";

    // Tope de líneas: si no, el vector crece sin límite
    if (g_EngineLog.size() > 1000) {
        g_EngineLog.erase(g_EngineLog.begin(), g_EngineLog.begin() + 200);
    }
    g_EngineLog.push_back(message);
}