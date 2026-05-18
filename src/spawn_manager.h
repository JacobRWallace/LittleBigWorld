#ifndef SPAWN_MANAGER_H
#define SPAWN_MANAGER_H

#include "object_manager.h"
#include "physics.h"
#include <map>
#include <vector>
#include <glm/glm.hpp>

struct SpawnedObject
{
    int instanceId;
    int objectDefId;
    RigidBody* rigidBody;
    Model* model;

    SpawnedObject(int id, int defId, RigidBody* body, Model* mdl)
        : instanceId(id), objectDefId(defId), rigidBody(body), model(mdl) {}
};

class SpawnManager
{
private:
    ObjectManager* objectManager;
    PhysicsEngine* physics;

    std::vector<SpawnedObject> spawnedObjects;
    int nextInstanceId = 0;

    // Spawn mode state
    bool isSpawning = false;
    const ObjectDef* selectedObjectDef = nullptr;

public:
    SpawnManager(ObjectManager* objMgr, PhysicsEngine* phys)
        : objectManager(objMgr), physics(phys) {}

    ~SpawnManager()
    {
        // Models are cleaned up externally
    }

    // Start spawn mode for an object
    void StartSpawning(const std::string& objectName)
    {
        selectedObjectDef = objectManager->Get(objectName);
        if (selectedObjectDef)
        {
            isSpawning = true;
            std::cout << "Spawn mode: " << selectedObjectDef->displayName << " (Press ESC to cancel)" << std::endl;
        }
    }

    // Cancel spawn mode
    void CancelSpawning()
    {
        isSpawning = false;
        selectedObjectDef = nullptr;
        std::cout << "Spawn mode cancelled" << std::endl;
    }

    // Spawn an object at world position
    bool SpawnAtPosition(glm::vec3 worldPos, Model* model)
    {
        if (!isSpawning || !selectedObjectDef)
            return false;

        // Create physics body from template
        RigidBody* body = physics->AddObjectFromTemplate(selectedObjectDef, worldPos);
        if (!body)
            return false;

        // Create spawned object instance
        int instanceId = nextInstanceId++;
        spawnedObjects.emplace_back(instanceId, selectedObjectDef->id, body, model);

        std::cout << "Spawned: " << selectedObjectDef->displayName 
                  << " (ID: " << selectedObjectDef->id << ", Instance: " << instanceId << ")" << std::endl;

        return true;
    }

    // Get all spawned objects
    const std::vector<SpawnedObject>& GetSpawnedObjects() const
    {
        return spawnedObjects;
    }

    // Check if in spawn mode
    bool IsSpawning() const
    {
        return isSpawning;
    }

    // Get selected object for spawn mode
    const ObjectDef* GetSelectedObject() const
    {
        return selectedObjectDef;
    }

    // Get spawned object by instance ID
    SpawnedObject* GetObjectByInstanceId(int id)
    {
        for (auto& obj : spawnedObjects)
        {
            if (obj.instanceId == id)
                return &obj;
        }
        return nullptr;
    }

    // Get spawned objects by object definition ID
    std::vector<SpawnedObject*> GetObjectsByDefId(int defId)
    {
        std::vector<SpawnedObject*> results;
        for (auto& obj : spawnedObjects)
        {
            if (obj.objectDefId == defId)
                results.push_back(&obj);
        }
        return results;
    }

    // Remove spawned object
    bool RemoveObject(int instanceId)
    {
        for (auto it = spawnedObjects.begin(); it != spawnedObjects.end(); ++it)
        {
            if (it->instanceId == instanceId)
            {
                spawnedObjects.erase(it);
                std::cout << "Removed object instance: " << instanceId << std::endl;
                return true;
            }
        }
        return false;
    }

    // Get total spawned count
    int GetSpawnedCount() const
    {
        return spawnedObjects.size();
    }

    // Get count of specific object type
    int GetSpawnedCountByDefId(int defId) const
    {
        int count = 0;
        for (const auto& obj : spawnedObjects)
        {
            if (obj.objectDefId == defId)
                count++;
        }
        return count;
    }
};

#endif
