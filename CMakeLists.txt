cmake_minimum_required(VERSION 3.15)
project(KurokoEngine) # Name your engine

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Packages
find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(DevIL REQUIRED)
find_package(imgui CONFIG REQUIRED)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB_RECURSE INCLUDE_FILES "include/*.h")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${INCLUDE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE src include src/Engine include/Engine src/Editor include/Editor)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    glad::glad
    glm::glm
    assimp::assimp
    DevIL::IL
    DevIL::ILU
    imgui::imgui
)

# Copia la carpeta Assets junto al .exe después de compilar
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Assets
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets)

# Que Visual Studio arranque el motor y no ALL_BUILD
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})