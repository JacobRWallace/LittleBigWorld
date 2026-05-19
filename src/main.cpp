// main.cpp : LittleBigWorld - LBP 2.5D Create Mode

// TODO: Make background border have walls and ceiling so objects don't fall out of the world.
// TODO: Make objects directory that contains .json files for each object type that define their properties (dimensions, friction, colors, etc) and load them at runtime instead of hardcoding in the code. This will allow for easier editing and adding new objects without recompiling.
// TODO: Make background border a separate object with its own collision shape so it can be edited and textured separately from the flat plane background.
// TODO: Make background a 3D model with proper textures instead of just a flat plane. This will allow for more interesting level design and visual variety.
// TODO: Make background a .json object file that is loaded in every level.
// TODO: Make Cube object a .json as well as pod.json that can be edited and reloaded without recompiling.

// TODO: how can we get these textures to load on the models for cube.obj, pod.obj
// TODO: Need to find an optimal file structure for a game wiht a ton of objects with models textures and metadata.



#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"
#include "object.h"
#include "camera.h"
#include "input.h"
#include "physics.h"
#include "model.h"
#include "debug.h"
#include "cursor.h"
#include "object_manager.h"
#include "spawn_manager.h"
#include "ui.h"

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;
const float FOV = 45.0f;

// Global instances
Camera* gCamera = nullptr;
Input* gInput = nullptr;
PhysicsEngine* gPhysics = nullptr;
Model* gCubeModel = nullptr;
Debug* gDebug = nullptr;
CursorManager* gCursor = nullptr;
ObjectManager* gObjectManager = nullptr;
SpawnManager* gSpawnManager = nullptr;
SpawnUI* gSpawnUI = nullptr;

// Model cache
std::map<std::string, Model*> gModelCache;

// GLFW callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main()
{
	std::cout << "LittleBigWorld - LBP 2.5D Create Mode\n";

	// Initialize GLFW
	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW\n";
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LittleBigWorld - Create Mode", NULL, NULL);

	if (!window) {
		glfwTerminate();
		std::cout << "Failed to create GLFW window\n";
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetKeyCallback(window, key_callback);

	// Load OpenGL function pointers with GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD\n";
		return -1;
	}

	// Initialize ImGui
	if (!ImGuiManager::Initialize(window, "#version 330"))
	{
		std::cout << "Failed to initialize ImGui\n";
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	// Build and compile shader
	Shader shader("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");

	// Create a white placeholder texture
	unsigned char whitePixel[3] = { 255, 255, 255 };
	GLuint placeholderTexture;
	glGenTextures(1, &placeholderTexture);
	glBindTexture(GL_TEXTURE_2D, placeholderTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Load model with placeholder texture
	Model* cubeModel = new Model("assets/objects/cube/cube.obj", placeholderTexture);

	// Initialize instances
	gPhysics = new PhysicsEngine();
	gCamera = new Camera();
	gInput = new Input(SCR_WIDTH, SCR_HEIGHT, gPhysics);
	gCubeModel = cubeModel;
	gDebug = new Debug();
	gCursor = new CursorManager(window);
	gCursor->LoadCursors("assets/cursor_pointer.png", "assets/cursor_grab.png");

	// Initialize ObjectManager and load objects
	gObjectManager = new ObjectManager();
	if (!gObjectManager->Load("assets/objects"))
	{
		std::cerr << "Failed to load objects!" << std::endl;
	}

	// Build palette and print info
	gObjectManager->BuildPalette();
	gObjectManager->PrintPalette();
	gObjectManager->PrintStats();

	// Initialize SpawnManager
	gSpawnManager = new SpawnManager(gObjectManager, gPhysics);

	// Initialize Spawn UI
	gSpawnUI = new SpawnUI(gObjectManager, gSpawnManager);
	gSpawnUI->LoadIconTextures();

	// Load models for all objects
	for (const auto& name : gObjectManager->GetPaletteOrder())
	{
		const ObjectDef* def = gObjectManager->Get(name);
		if (def && gModelCache.find(def->model.path) == gModelCache.end())
		{
			gModelCache[def->model.path] = new Model(def->model.path.c_str(), placeholderTexture);
		}
	}

	// Spawn floor only
	gPhysics->AddPlane(0.0f);

	// TODO: Load objects from JSON via ObjectManager
	// For now, physics bodies will be created when objects are spawned from JSON

	float lastFrame = 0.0f;	while (!glfwWindowShouldClose(window)) {
		float currentFrame = (float)glfwGetTime();
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Process input
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		gCamera->ProcessInput(window, deltaTime);

		// Update physics
		gPhysics->StepSimulation(deltaTime);

		// Render
		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();

		// Set texture uniform
		shader.setInt("textureSampler", 0);

		// Projection matrix (perspective)
		glm::mat4 projection = glm::perspective(glm::radians(FOV), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		shader.setMat4("projection", projection);

		// View matrix (from camera manager)
		glm::mat4 view = gCamera->GetViewMatrix();
		shader.setMat4("view", view);

		// Render floor
		glm::mat4 floorModel = glm::mat4(1.0f);
		floorModel = glm::scale(floorModel, glm::vec3(10.0f, 0.1f, 3.0f));
		shader.setMat4("model", floorModel);
		cubeModel->Draw();

		// Render spawned objects
		for (const auto& spawnedObj : gSpawnManager->GetSpawnedObjects())
		{
			if (spawnedObj.rigidBody && spawnedObj.rigidBody->objectDef && spawnedObj.model)
			{
				glm::mat4 modelMatrix = spawnedObj.rigidBody->modelMatrix;

				// Apply model offset and scale
				modelMatrix = glm::translate(modelMatrix, spawnedObj.rigidBody->objectDef->model.offset);
				modelMatrix = glm::scale(modelMatrix, spawnedObj.rigidBody->objectDef->model.scale);

				shader.setMat4("model", modelMatrix);
				spawnedObj.model->Draw();
			}
		}

		// Draw debug hitboxes for all rigid bodies
		gDebug->Clear();
		for (const auto& rigidBody : gPhysics->rigidBodies)
		{
			btCollisionShape* shape = rigidBody->body->getCollisionShape();
			if (shape && shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
			{
				btBoxShape* boxShape = static_cast<btBoxShape*>(shape);
				btVector3 halfExtents = boxShape->getHalfExtentsWithMargin();

				glm::vec3 extents = glm::vec3(halfExtents.getX(), halfExtents.getY(), halfExtents.getZ());

				// Default green color
				glm::vec3 boxColor = glm::vec3(0.0f, 1.0f, 0.0f);

				// Highlight if hovering over it
				if (gInput->IsHoveringBody(rigidBody))
				{
					boxColor = glm::vec3(1.0f, 1.0f, 0.0f);  // Yellow when hovering
				}

				// Highlight if currently grabbed
				if (gInput->IsGrabbingBody(rigidBody))
				{
					boxColor = glm::vec3(1.0f, 0.0f, 0.0f);  // Red when grabbed
				}

				// Use the rigid body's model matrix which includes rotation
				gDebug->AddBoxWithTransform(rigidBody->modelMatrix, extents, boxColor);
			}
		}
		gDebug->Render(shader, projection, view);

		// Render ImGui
		ImGuiManager::NewFrame();
		gSpawnUI->Render();
		ImGuiManager::Render();

		// Update cursor based on interaction state
		if (gInput->IsGrabbingAny())
		{
			gCursor->SetGrabbingCursor();
		}
		else if (gInput->IsHoveringAny())
		{
			gCursor->SetGrabCursor();  // Show grab cursor when hovering over objects
		}
		else
		{
			gCursor->SetNormalCursor();
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Clean up
	delete gCamera;
	delete gInput;
	delete gPhysics;
	delete gDebug;
	delete gCursor;
	delete gObjectManager;
	delete gSpawnManager;
	delete gSpawnUI;

	// Shutdown ImGui
	ImGuiManager::Shutdown();

	// Clean up models
	delete cubeModel;
	for (auto& pair : gModelCache)
	{
		delete pair.second;
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Don't process game input if ImGui wants the mouse
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse && button == GLFW_MOUSE_BUTTON_LEFT)
		return;

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		// Check if in spawn mode
		if (gSpawnManager->IsSpawning())
		{
			// Raycast to find where to spawn
			glm::vec3 rayDir = gInput->GetRayFromMouse(gCamera->GetPosition(), gCamera->GetFront(), gCamera->GetUp(), FOV);
			glm::vec3 spawnPos = gCamera->GetPosition() + rayDir * 20.0f;  // Spawn 20 units forward

			// Get model for spawning
			const ObjectDef* def = gObjectManager->GetSelectedPaletteObject();
			if (def && gModelCache.count(def->model.path))
			{
				gSpawnManager->SpawnAtPosition(spawnPos, gModelCache[def->model.path]);
			}
		}
		else
		{
			// Normal grab interaction
			gInput->OnMouseButtonPress(gCamera->GetPosition(), gCamera->GetFront(), gCamera->GetUp(), FOV);
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		gInput->OnMouseButtonRelease();
	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	gInput->UpdateMousePos(xpos, ypos);
	gInput->UpdateDragging(gCamera->GetPosition(), gCamera->GetFront(), gCamera->GetUp(), FOV);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
		return;

	// Don't process game input if ImGui wants keyboard
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard && key != GLFW_KEY_ESCAPE && key != GLFW_KEY_U)
		return;

	// ESC to cancel spawn mode
	if (key == GLFW_KEY_ESCAPE)
	{
		if (gSpawnManager->IsSpawning())
		{
			gSpawnManager->CancelSpawning();
		}
		return;
	}

	// Number keys 0-9 to select object for spawning
	if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
	{
		int index = key - GLFW_KEY_0;
		if (gObjectManager->SelectPaletteItem(index))
		{
			const ObjectDef* def = gObjectManager->GetSelectedPaletteObject();
			if (def)
			{
				gSpawnManager->StartSpawning(def->name);
			}
		}
		return;
	}

	// P to print palette
	if (key == GLFW_KEY_P)
	{
		gObjectManager->PrintPalette();
		return;
	}

	// U to toggle UI
	if (key == GLFW_KEY_U)
	{
		gSpawnUI->Toggle();
		return;
	}

	// H for help
	if (key == GLFW_KEY_H)
	{
		std::cout << "\n=== CONTROLS ===" << std::endl;
		std::cout << "0-9: Select object to spawn" << std::endl;
		std::cout << "Click in world: Spawn selected object" << std::endl;
		std::cout << "ESC: Cancel spawn mode" << std::endl;
		std::cout << "P: Print palette" << std::endl;
		std::cout << "U: Toggle UI" << std::endl;
		std::cout << "H: Show this help" << std::endl;
		std::cout << "================\n" << std::endl;
		return;
	}
}


