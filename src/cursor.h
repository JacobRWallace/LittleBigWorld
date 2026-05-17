#ifndef CURSOR_H
#define CURSOR_H

#include <GLFW/glfw3.h>
#include <string>
#include <iostream>

class CursorManager
{
private:
    GLFWcursor* normalCursor;
    GLFWcursor* grabCursor;
    GLFWcursor* grabbingCursor;
    GLFWwindow* window;
    GLFWcursor* currentCursor;

public:
    CursorManager(GLFWwindow* win) : window(win), normalCursor(nullptr), grabCursor(nullptr), grabbingCursor(nullptr), currentCursor(nullptr)
    {
    }

    ~CursorManager()
    {
        if (normalCursor) glfwDestroyCursor(normalCursor);
        if (grabCursor) glfwDestroyCursor(grabCursor);
        if (grabbingCursor) glfwDestroyCursor(grabbingCursor);
    }

    bool LoadCursors(const std::string& normalPath, const std::string& grabPath)
    {
        // Load normal cursor (pointer)
        normalCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

        // Load grab cursor - try from file, fall back to hand cursor
        grabbingCursor = CreateCursorFromFile(grabPath);
        if (!grabbingCursor)
        {
            grabbingCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        }

        grabCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

        // Set initial cursor
        if (normalCursor)
        {
            glfwSetCursor(window, normalCursor);
            currentCursor = normalCursor;
        }

        return true;
    }

    void SetNormalCursor()
    {
        if (normalCursor && currentCursor != normalCursor)
        {
            glfwSetCursor(window, normalCursor);
            currentCursor = normalCursor;
        }
    }

    void SetGrabCursor()
    {
        if (grabCursor && currentCursor != grabCursor)
        {
            glfwSetCursor(window, grabCursor);
            currentCursor = grabCursor;
        }
    }

    void SetGrabbingCursor()
    {
        if (grabbingCursor && currentCursor != grabbingCursor)
        {
            glfwSetCursor(window, grabbingCursor);
            currentCursor = grabbingCursor;
        }
    }

private:
    GLFWcursor* CreateCursorFromFile(const std::string& path)
    {
        // Attempt to load cursor from PNG file
        // For now, this returns nullptr to fall back to standard cursor
        // Full PNG loading would require additional image processing libraries

        std::cout << "Note: Custom cursor loading not yet implemented. Using standard cursor." << std::endl;
        return nullptr;
    }
};

#endif
