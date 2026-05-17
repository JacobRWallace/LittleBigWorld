#ifndef DEBUG_H
#define DEBUG_H

#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include "shader.h"

struct DebugLine
{
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
};

class Debug
{
private:
    std::vector<DebugLine> lines;
    GLuint VAO, VBO;
    bool initialized;
    Shader* debugShader;

public:
    Debug() : VAO(0), VBO(0), initialized(false), debugShader(nullptr) {}

    ~Debug()
    {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (debugShader) delete debugShader;
    }

    void InitializeShader()
    {
        if (!debugShader)
        {
            debugShader = new Shader("assets/shaders/debug_hitboxOutline.glsl", "assets/shaders/debug_hitboxColor.glsl");
        }
    }

    void AddLine(glm::vec3 start, glm::vec3 end, glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f))
    {
        lines.push_back({start, end, color});
    }

    void AddBox(glm::vec3 center, glm::vec3 halfExtents, glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f))
    {
        // Get all 8 corners of the box in local space
        glm::vec3 corners[8] = {
            glm::vec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
            glm::vec3(halfExtents.x, -halfExtents.y, -halfExtents.z),
            glm::vec3(halfExtents.x, halfExtents.y, -halfExtents.z),
            glm::vec3(-halfExtents.x, halfExtents.y, -halfExtents.z),
            glm::vec3(-halfExtents.x, -halfExtents.y, halfExtents.z),
            glm::vec3(halfExtents.x, -halfExtents.y, halfExtents.z),
            glm::vec3(halfExtents.x, halfExtents.y, halfExtents.z),
            glm::vec3(-halfExtents.x, halfExtents.y, halfExtents.z),
        };

        // Transform corners to world space
        for (int i = 0; i < 8; i++)
        {
            corners[i] = center + corners[i];
        }

        // Bottom face
        AddLine(corners[0], corners[1], color);
        AddLine(corners[1], corners[2], color);
        AddLine(corners[2], corners[3], color);
        AddLine(corners[3], corners[0], color);

        // Top face
        AddLine(corners[4], corners[5], color);
        AddLine(corners[5], corners[6], color);
        AddLine(corners[6], corners[7], color);
        AddLine(corners[7], corners[4], color);

        // Vertical edges
        AddLine(corners[0], corners[4], color);
        AddLine(corners[1], corners[5], color);
        AddLine(corners[2], corners[6], color);
        AddLine(corners[3], corners[7], color);
    }

    void AddBoxWithTransform(glm::mat4 transform, glm::vec3 halfExtents, glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f))
    {
        // Get all 8 corners of the box in local space
        glm::vec3 corners[8] = {
            glm::vec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
            glm::vec3(halfExtents.x, -halfExtents.y, -halfExtents.z),
            glm::vec3(halfExtents.x, halfExtents.y, -halfExtents.z),
            glm::vec3(-halfExtents.x, halfExtents.y, -halfExtents.z),
            glm::vec3(-halfExtents.x, -halfExtents.y, halfExtents.z),
            glm::vec3(halfExtents.x, -halfExtents.y, halfExtents.z),
            glm::vec3(halfExtents.x, halfExtents.y, halfExtents.z),
            glm::vec3(-halfExtents.x, halfExtents.y, halfExtents.z),
        };

        // Transform corners to world space using the matrix
        for (int i = 0; i < 8; i++)
        {
            corners[i] = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
        }

        // Bottom face
        AddLine(corners[0], corners[1], color);
        AddLine(corners[1], corners[2], color);
        AddLine(corners[2], corners[3], color);
        AddLine(corners[3], corners[0], color);

        // Top face
        AddLine(corners[4], corners[5], color);
        AddLine(corners[5], corners[6], color);
        AddLine(corners[6], corners[7], color);
        AddLine(corners[7], corners[4], color);

        // Vertical edges
        AddLine(corners[0], corners[4], color);
        AddLine(corners[1], corners[5], color);
        AddLine(corners[2], corners[6], color);
        AddLine(corners[3], corners[7], color);
    }

    void Render(Shader& shader, glm::mat4 projection, glm::mat4 view)
    {
        if (lines.empty()) return;

        if (!initialized)
        {
            InitializeBuffers();
            InitializeShader();
            initialized = true;
        }

        UpdateBuffers();

        debugShader->use();
        debugShader->setMat4("projection", projection);
        debugShader->setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        debugShader->setMat4("model", model);

        glLineWidth(2.0f);
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, (GLsizei)(lines.size() * 2));
        glBindVertexArray(0);
        glLineWidth(1.0f);
    }

    void Clear()
    {
        lines.clear();
    }

private:
    void InitializeBuffers()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // Vertex position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Vertex color
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void UpdateBuffers()
    {
        std::vector<float> vertexData;
        for (const auto& line : lines)
        {
            // Start vertex
            vertexData.push_back(line.start.x);
            vertexData.push_back(line.start.y);
            vertexData.push_back(line.start.z);
            vertexData.push_back(line.color.x);
            vertexData.push_back(line.color.y);
            vertexData.push_back(line.color.z);

            // End vertex
            vertexData.push_back(line.end.x);
            vertexData.push_back(line.end.y);
            vertexData.push_back(line.end.z);
            vertexData.push_back(line.color.x);
            vertexData.push_back(line.color.y);
            vertexData.push_back(line.color.z);
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

#endif
