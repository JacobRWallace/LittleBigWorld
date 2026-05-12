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

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;
const float FOV = 45.0f;

// Global instances
Camera* gCamera = nullptr;
Input* gInput = nullptr;
PhysicsEngine* gPhysics = nullptr;

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

	// Initialize instances
	gPhysics = new PhysicsEngine();
	gCamera = new Camera();
	gInput = new Input(SCR_WIDTH, SCR_HEIGHT, gPhysics);

	// Create test cube object with all properties
	// OBJ cube ranges from -1 to 1, so it's 2x2x2 in size
	Object cubeObject("TestCube", cubeModel, 1.0f, glm::vec3(2.0f, 2.0f, 2.0f));
	cubeObject.baseColor = glm::vec3(0.8f, 0.8f, 0.9f);
	cubeObject.highlightColor = glm::vec3(1.2f, 1.2f, 1.5f);
	cubeObject.friction = 0.8f;
	cubeObject.canBeSelected = true;
	cubeObject.layerIndex = 1;

	// Add physics bodies
	gPhysics->AddPlane(0.0f);  // Floor

	RigidBody* cubeBody = gPhysics->AddBox(glm::vec3(0.0f, 2.0f, 0.0f), cubeObject.dimensions, cubeObject.weight, 1);
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
		floorModel = glm::scale(floorModel, glm::vec3(10.0f, 0.1f, 10.0f));
		shader.setMat4("model", floorModel);
		cubeModel->Draw();

		// Render cube object
		glm::mat4 cubeModelMatrix = cubeBody->modelMatrix;
		shader.setMat4("model", cubeModelMatrix);
		cubeObject.model->Draw();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Clean up
	delete gCamera;
	delete gInput;
	delete gPhysics;
	delete cubeModel;

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


