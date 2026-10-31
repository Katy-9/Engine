#pragma once

class GameObject;

namespace PrimitiveGenerator {
    // Funciones para generar las formas básicas y añadirlas a la escena
    GameObject* CreateCube(GameObject* parent);
    GameObject* CreatePyramid(GameObject* parent);
    GameObject* CreateSphere(int segments, int rings, GameObject* parent);
    GameObject* CreateCylinder(int segments, GameObject* parent);
    GameObject* CreatePlane(GameObject* parent);
    GameObject* CreateCone(int segments, GameObject* parent);
}