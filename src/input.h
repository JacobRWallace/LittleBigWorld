#ifndef INPUT_H
#define INPUT_H

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "physics.h"

class Input
{
private:
    glm::vec2 mousePos;
    glm::vec2 lastMousePos;
    glm::vec3 currentMouseWorldPos;
    glm::vec3 lastMouseWorldPos;
    RigidBody* selectedBody;
    RigidBody* hoveredBody;
    bool isDragging;
    glm::vec3 dragOffset;

    static constexpr float MOUSE_DRAG_PLANE = 20.0f;
    unsigned int screenWidth;
    unsigned int screenHeight;
    PhysicsEngine* physics;

public:
    Input(unsigned int width, unsigned int height, PhysicsEngine* physicsEngine)
        : mousePos(0.0f),
          lastMousePos(0.0f),
          currentMouseWorldPos(0.0f),
          lastMouseWorldPos(0.0f),
          selectedBody(nullptr),
          hoveredBody(nullptr),
          isDragging(false),
          dragOffset(0.0f),
          screenWidth(width),
          screenHeight(height),
          physics(physicsEngine)
    {
    }

    ~Input() = default;

    void UpdateMousePos(double xpos, double ypos)
    {
        lastMousePos = mousePos;
        mousePos = glm::vec2(xpos, ypos);
    }

    glm::vec3 GetRayFromMouse(glm::vec3 cameraPos, glm::vec3 cameraDir, glm::vec3 cameraUp, float fov)
    {
        float x = (2.0f * mousePos.x) / screenWidth - 1.0f;
        float y = 1.0f - (2.0f * mousePos.y) / screenHeight;

        float aspect = (float)screenWidth / (float)screenHeight;
        float tanHalfFov = glm::tan(glm::radians(fov / 2.0f));
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraDir, cameraUp));
        glm::vec3 actualUp = glm::normalize(glm::cross(cameraRight, cameraDir));

        glm::vec3 rayDir = glm::normalize(cameraDir + x * aspect * tanHalfFov * cameraRight + y * tanHalfFov * actualUp);
        return rayDir;
    }

    void OnMouseButtonPress(glm::vec3 cameraPos, glm::vec3 cameraDir, glm::vec3 cameraUp, float fov)
    {
        glm::vec3 rayDir = GetRayFromMouse(cameraPos, cameraDir, cameraUp, fov);
        glm::vec3 rayEnd = cameraPos + rayDir * 1000.0f;

        selectedBody = physics->RaycastClosest(cameraPos, rayEnd);

        if (selectedBody)
        {
            isDragging = true;
            lastMousePos = mousePos;
            currentMouseWorldPos = cameraPos + rayDir * MOUSE_DRAG_PLANE;
            lastMouseWorldPos = currentMouseWorldPos;
            dragOffset = selectedBody->GetPosition() - currentMouseWorldPos;
        }
    }

    void OnMouseButtonRelease()
    {
        if (isDragging && selectedBody)
        {
            glm::vec3 velocity = (currentMouseWorldPos - lastMouseWorldPos) / 0.016f;
            selectedBody->body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
        }

        isDragging = false;
        selectedBody = nullptr;
    }

    void UpdateDragging(glm::vec3 cameraPos, glm::vec3 cameraDir, glm::vec3 cameraUp, float fov)
    {
        if (isDragging && selectedBody)
        {
            glm::vec3 rayDir = GetRayFromMouse(cameraPos, cameraDir, cameraUp, fov);
            lastMouseWorldPos = currentMouseWorldPos;
            currentMouseWorldPos = cameraPos + rayDir * MOUSE_DRAG_PLANE;

            glm::vec3 newObjectPos = currentMouseWorldPos + dragOffset;
            selectedBody->SetPosition(newObjectPos);
            selectedBody->body->setLinearVelocity(btVector3(0, 0, 0));
            selectedBody->body->setAngularVelocity(btVector3(0, 0, 0));
        }

        // Update hovered body every frame
        UpdateHoveredBody(cameraPos, cameraDir, cameraUp, fov);
    }

    void UpdateHoveredBody(glm::vec3 cameraPos, glm::vec3 cameraDir, glm::vec3 cameraUp, float fov)
    {
        glm::vec3 rayDir = GetRayFromMouse(cameraPos, cameraDir, cameraUp, fov);
        glm::vec3 rayEnd = cameraPos + rayDir * 1000.0f;

        hoveredBody = physics->RaycastClosest(cameraPos, rayEnd);
    }

    bool IsHoveringBody(RigidBody* body) const
    {
        return hoveredBody == body && !isDragging;
    }

    bool IsGrabbingBody(RigidBody* body) const
    {
        return selectedBody == body && isDragging;
    }
};

#endif
