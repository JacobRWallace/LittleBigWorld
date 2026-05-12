// main.cpp : LittleBigWorld - LBP 2.5D Create Mode

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

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;
const float FOV = 45.0f;

// Global instances
Camera* gCamera = nullptr;
Input* gInput = nullptr;
PhysicsEngine* gPhysics = nullptr;
Model* gCubeModel = nullptr;
Debug* gDebug = nullptr;

// GLFW callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

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

	// Load OpenGL function pointers with GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD\n";
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
	Model* cubeModel = new Model("assets/cube.obj", placeholderTexture);

	// Load pod model
	Model* podModel = new Model("assets/pod/pod.obj", placeholderTexture);

	// Initialize instances
	gPhysics = new PhysicsEngine();
	gCamera = new Camera();
	gInput = new Input(SCR_WIDTH, SCR_HEIGHT, gPhysics);
	gCubeModel = cubeModel;
	gDebug = new Debug();

	// Create test cube object with all properties
	// OBJ cube ranges from -1 to 1, so it's 2x2x2 in size
	Object cubeObject("TestCube", cubeModel, 1.0f, glm::vec3(2.0f, 2.0f, 2.0f));
	cubeObject.baseColor = glm::vec3(0.8f, 0.8f, 0.9f);
	cubeObject.highlightColor = glm::vec3(1.2f, 1.2f, 1.5f);
	cubeObject.friction = 0.8f;
	cubeObject.canBeSelected = true;
	cubeObject.layerIndex = 1;

	// Get pod model dimensions and scale them appropriately
	glm::vec3 podModelDimensions = podModel->GetDimensions();
	glm::vec3 podDimensions = podModelDimensions * 0.01f;  // Scale down to match rendering scale

	// Create pod object with scaled dimensions
	Object cubeObject2("Pod", podModel, 2.0f, podDimensions);
	cubeObject2.baseColor = glm::vec3(0.9f, 0.8f, 0.8f);
	cubeObject2.highlightColor = glm::vec3(1.5f, 1.2f, 1.2f);
	cubeObject2.friction = 0.8f;
	cubeObject2.canBeSelected = true;
	cubeObject2.layerIndex = 1;

	// Add physics bodies
	gPhysics->AddPlane(0.0f);  // Floor

	RigidBody* cubeBody = gPhysics->AddBox(glm::vec3(0.0f, 2.0f, 0.0f), cubeObject.dimensions, cubeObject.weight, 1);
	RigidBody* cubeBody2 = gPhysics->AddBox(glm::vec3(5.0f, 4.0f, 0.0f), cubeObject2.dimensions, cubeObject2.weight, 1);
	cubeBody->body->setFriction(cubeObject.friction);

	float lastFrame = 0.0f;

	// Main loop
	while (!glfwWindowShouldClose(window)) {
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

		// Render cube object
		glm::mat4 cubeModelMatrix = cubeBody->modelMatrix;
		shader.setMat4("model", cubeModelMatrix);
		cubeObject.model->Draw();

		// Render second cube object
		glm::mat4 cubeModelMatrix2 = cubeBody2->modelMatrix;
		cubeModelMatrix2 = glm::translate(cubeModelMatrix2, glm::vec3(0.0f, -podDimensions.y * 0.5f, 0.0f));
		cubeModelMatrix2 = glm::scale(cubeModelMatrix2, glm::vec3(0.01f, 0.01f, 0.01f));
		shader.setMat4("model", cubeModelMatrix2);
		cubeObject2.model->Draw();

		// Draw debug hitboxes for all rigid bodies
		gDebug->Clear();
		for (const auto& rigidBody : gPhysics->rigidBodies)
		{
			btCollisionShape* shape = rigidBody->body->getCollisionShape();
			if (shape && shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
			{
				btBoxShape* boxShape = static_cast<btBoxShape*>(shape);
				btVector3 halfExtents = boxShape->getHalfExtentsWithMargin();

				glm::vec3 center = rigidBody->GetPosition();
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

				gDebug->AddBox(center, extents, boxColor);
			}
		}
		gDebug->Render(shader, projection, view);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Clean up
	delete gCamera;
	delete gInput;
	delete gPhysics;
	delete gDebug;
	delete cubeModel;
	delete podModel;

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
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		gInput->OnMouseButtonPress(gCamera->GetPosition(), gCamera->GetFront(), gCamera->GetUp(), FOV);
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


