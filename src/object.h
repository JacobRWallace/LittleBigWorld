#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>
#include <string>
#include "model.h"

struct Object
{
    // Model and rendering
    Model* model;              // Pointer to the 3D model asset

    // Physics properties
    float weight;              // Mass in kg
    glm::vec3 dimensions;      // X (width), Y (height), Z (depth)
    float friction;            // Friction coefficient

    // Visual properties
    glm::vec3 baseColor;       // Base color tint (RGB)
    glm::vec3 highlightColor;  // Color when highlighted (RGB)
    bool isHighlighted;        // Current highlight state

    // Interaction properties
    bool canBeSelected;        // Can this object be selected by the player
    bool canBeGrabbed;         // Can this object be grabbed and moved
    bool allowRotation;        // Can this object be rotated

    // Object metadata
    std::string name;          // Object identifier/name
    int layerIndex;            // Which 2.5D layer (0, 1, or 2)

    // Constructor with defaults
    Object()
        : model(nullptr),
          weight(1.0f),
          dimensions(1.0f, 1.0f, 1.0f),
          friction(0.5f),
          baseColor(1.0f, 1.0f, 1.0f),
          highlightColor(1.2f, 1.2f, 1.2f),
          isHighlighted(false),
          canBeSelected(true),
          canBeGrabbed(true),
          allowRotation(false),
          name("Object"),
          layerIndex(1)
    {
    }

    // Constructor with common properties
    Object(const std::string& objectName, Model* mdl, float mass, const glm::vec3& dims)
        : model(mdl),
          weight(mass),
          dimensions(dims),
          friction(0.5f),
          baseColor(1.0f, 1.0f, 1.0f),
          highlightColor(1.2f, 1.2f, 1.2f),
          isHighlighted(false),
          canBeSelected(true),
          canBeGrabbed(true),
          allowRotation(false),
          name(objectName),
          layerIndex(1)
    {
    }

    // Set highlight state
    void SetHighlighted(bool highlighted)
    {
        isHighlighted = highlighted;
    }

    // Get current color based on highlight state
    glm::vec3 GetCurrentColor() const
    {
        if (isHighlighted)
            return highlightColor;
        return baseColor;
    }
};

#endif
