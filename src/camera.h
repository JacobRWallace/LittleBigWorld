#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera
{
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Speed;
    float Sensitivity;

    Camera(glm::vec3 position = glm::vec3(0.0f, 4.0f, 8.0f),
           glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float speed = 5.0f,
           float sensitivity = 0.1f)
        : Position(position), Front(glm::normalize(front)), WorldUp(glm::normalize(up)), Speed(speed), Sensitivity(sensitivity)
    {
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::vec3 GetPosition() const { return Position; }
    glm::vec3 GetFront() const { return Front; }
    glm::vec3 GetUp() const { return Up; }
    glm::vec3 GetRight() const { return Right; }

    void MoveForward(float deltaTime)
    {
        Position += Front * Speed * deltaTime;
    }

    void MoveBackward(float deltaTime)
    {
        Position -= Front * Speed * deltaTime;
    }

    void MoveLeft(float deltaTime)
    {
        Position -= Right * Speed * deltaTime;
    }

    void MoveRight(float deltaTime)
    {
        Position += Right * Speed * deltaTime;
    }

    void MoveUp(float deltaTime)
    {
        Position += WorldUp * Speed * deltaTime;
    }

    void MoveDown(float deltaTime)
    {
        Position -= WorldUp * Speed * deltaTime;
    }

    void ProcessInput(GLFWwindow* window, float deltaTime)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            MoveUp(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            MoveDown(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            MoveLeft(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            MoveRight(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            MoveForward(deltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            MoveBackward(deltaTime);
    }
};

#endif
