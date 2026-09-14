cmake_minimum_required(VERSION 3.10) # Establece la versión mínima
project(EngineProyect)

# Busca SDL3 a través de vcpkg
find_package(SDL3 CONFIG REQUIRED)

# Genera el archivo ejecutable (.exe)
add_executable(EngineProyect main.cpp)

# Enlaza la librería estática
target_link_libraries(EngineProyect PRIVATE SDL3::SDL3)