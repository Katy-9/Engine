#pragma once
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <string>

#define LOG(format, ...) Log_Engine(__FILE__, __LINE__, format, ##__VA_ARGS__)

void Log_Engine(const char file[], int line, const char* format, ...);

extern std::vector<std::string> g_EngineLog;