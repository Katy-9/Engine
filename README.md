# Kuroko Engine 

Motor gráfico 3D desarrollado desde cero como proyecto para la asignatura de Motores de videojuegos.

* **Repositorio de GitHub:** [https://github.com/Katy-9/Engine](https://github.com/Katy-9/Engine)

### Integrantes del Grupo
* Queralt Escutia — [Keral06](https://github.com/Keral06)
* Sofía Giner — [Katy-9](https://github.com/Katy-9)
* Joel Martínez — [Jowey7](https://github.com/Jowey7)

---

## 🚀 Cómo Usar el Motor

### Controles de la Cámara (Viewport)
* **Orbital (ALT + Clic Izquierdo + Mover ratón):** Rota la cámara alrededor del objeto seleccionado o del pivote central.
* **Free Look (Clic Derecho + Mover ratón):** Permite mirar libremente alrededor del escenario.
* **Movimiento (W, A, S, D):** Desplaza la cámara hacia adelante, atrás y a los lados mientras mantienes pulsado el Clic Derecho.
* **Panorámica (Clic Central / Rueda pulsada + Mover ratón):** Desplaza la cámara vertical y horizontalmente.
* **Zoom (Rueda del ratón):** Acerca o aleja la vista del punto de interés.
* **Centrar (Tecla F):** Encuadra y centra la cámara automáticamente en el objeto que tengas seleccionado en la jerarquía.
* **Velocidad Turbo (Shift Izquierdo/Derecho):** Duplica la velocidad de movimiento de la cámara al combinarlo con las teclas WASD.

### Interfaz y Editor
* **Menú Superior (`GameObject > 3D Object`):** Permite añadir primitivas geométricas básicas a la escena (Cubo, Esfera, Plano, Cono, Pirámide y Cilindro).
* **Drag and Drop:** Puedes arrastrar archivos `.fbx` (con soporte para mallas, normales y texturas físicas o incrustadas) o archivos de textura compatibles (`.png`, `.dds`) directamente a la ventana del motor para cargarlos o aplicarlos al objeto seleccionado.
* **Ventana Inspector:** Permite modificar en tiempo real la posición, rotación (con ángulos de Euler) y escala, activar/desactivar componentes, alternar mallas en modo *Wireframe*, y visualizar las normales de vértices y caras.
* **Ventana Configuration:** Permite controlar parámetros de rendimiento
* **Consola de Logs:** Muestra en tiempo real los mensajes de inicialización, advertencias y errores devueltos por el motor y librerías externas.

---

## ✨ Funcionalidades Extra

* **Soporte Híbrido de Texturas (Incuidas y Externas):** El importador basado en Assimp es capaz de detectar y descomprimir texturas incrustadas en el modelo FBX
* **Generación de Figuras Primitivas:** El motor cuenta con un generador de figuras para crear geometrías tridimensionales (esferas, cubos, planos, cilindros, conos y pirámides).
* **Función de duplicado y delete:** El motor cuenta con os atajos `Ctrl + D` para duplicar y `Supr` para eliminar game objects
* **Wireframe:** En las opciones del mesh aparte de las normales se puede ver también el wireframe del objeto 3D
---

## 💡 Comentarios Adicionales para el Profesor

* **Arquitectura:** El motor está estructurado bajo un patrón de módulos independientes (`ModuleWindow`, `ModuleInput`, `ModuleRenderer3D`, `ModuleCamera3D`, `ModuleScene`, `ModuleImporter` y `ModuleEditor`) controlados por una clase central (`Application`), lo que facilita la escalabilidad y el mantenimiento del bucle principal.
* **Gestión de Memoria:** Se implementa un sistema estricto de destrucción en cascada (`CleanUp` y destructores recursivos) tanto en la jerarquía de `GameObjects` y sus componentes como en el caché de texturas para evitar memory leaks.
