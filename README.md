# LittleBigWorld

A custom C++ 2.5D/3D sandbox engine and create mode editor focused on physics-driven interaction, user-generated content systems, and modular world building.

The project is inspired by creation-focused platform/sandbox games and is being built from scratch using modern C++ libraries and OpenGL rendering.

---

# Features

* OpenGL rendering pipeline
* GLFW window/input handling
* GLAD OpenGL function loading
* GLM math and transformation systems
* Bullet Physics rigid body simulation
* Assimp model importing
* Object/entity management systems
* Runtime asset loading
* Create mode/editor foundation
* Physics-driven object interaction
* Camera movement and viewport controls

---

# Technologies

* C++
* OpenGL
* GLFW
* GLAD
* GLM
* Bullet Physics
* Assimp
* Visual Studio
* CMake

---

# Project Structure

```txt
LittleBigWorld/
├── assets/
├── shaders/
├── src/
├── libs/
├── build/
└── LittleBigWorld.sln
```

---

# Build Notes

* Built for x64
* Uses `/MDd` runtime in Debug
* Bullet Physics rebuilt to match runtime configuration
* Runtime DLLs (such as Assimp) must be placed beside the executable

---

# Status

Active engine/editor development in progress.
Focus is currently on rendering, physics integration, editor systems, and foundational architecture.
