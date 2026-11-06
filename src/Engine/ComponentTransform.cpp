#include "ComponentTransform.h"
#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

ComponentTransform::ComponentTransform(GameObject* owner)
    : Component(owner, ComponentType::TRANSFORM)
{
    // Valores iniciales por defecto cuando se crea el componente
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    eulerRotation = glm::vec3(0.0f, 0.0f, 0.0f);
    rotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
    scale = glm::vec3(1.0f, 1.0f, 1.0f);
}

ComponentTransform::~ComponentTransform() {}

void ComponentTransform::UpdateRotationFromEuler() {
    // Convierte los grados (Euler) que ponemos en el ImGui a Radianes, 
    // y de radianes crea el Quaternion interno que usa OpenGL para rotar.
    glm::vec3 radians = glm::radians(eulerRotation);
    rotation = glm::quat(radians);
}

glm::mat4 ComponentTransform::GetGlobalMatrix() const {
    // 1. Calculamos la matriz LOCAL de este GameObject en concreto
    glm::mat4 localMatrix = glm::mat4(1.0f);
    localMatrix = glm::translate(localMatrix, position);
    localMatrix = localMatrix * glm::mat4_cast(rotation);
    localMatrix = glm::scale(localMatrix, scale);

    // 2. Si este objeto tiene un Padre (jerarquía), multiplicamos la matriz
    // del padre por la nuestra para obtener la posición/rotación GLOBAL en el mundo.
    if (owner->parent != nullptr) {
        ComponentTransform* parentTransform = owner->parent->GetComponent<ComponentTransform>(ComponentType::TRANSFORM);
        if (parentTransform != nullptr) {

            // LA MATRIZ GLOBAL = MATRIZ PADRE * MATRIZ LOCAL
            return parentTransform->GetGlobalMatrix() * localMatrix;
        }
    }

    // 3. Si no tiene padre (es un nodo raíz), la matriz local es su matriz global.
    return localMatrix;
}

void ComponentTransform::SetGlobalMatrix(const glm::mat4& globalMatrix) {
    glm::mat4 localMatrix = globalMatrix;

    // Si tenemos padre, la matriz del Gizmo (Global) hay que pasarla a Local multiplicando por la inversa del padre
    if (owner->parent != nullptr) {
        ComponentTransform* pTransform = owner->parent->GetComponent<ComponentTransform>(ComponentType::TRANSFORM);
        if (pTransform) {
            localMatrix = glm::inverse(pTransform->GetGlobalMatrix()) * globalMatrix;
        }
    }

    // Descomponemos la matriz resultante con GLM y lo guardamos
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(localMatrix, scale, rotation, position, skew, perspective);
    eulerRotation = glm::degrees(glm::eulerAngles(rotation));
}