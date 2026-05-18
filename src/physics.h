#ifndef PHYSICS_H
#define PHYSICS_H

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "object_manager.h"

// LBP 2.5D Layer System
const float LAYER_THICKNESS = 0.3f;  // How thick each layer is
const int NUM_LAYERS = 3;             // 3 layers total
const float LAYER_DEPTH = LAYER_THICKNESS * NUM_LAYERS;  // Total Z-depth

// Layer Z positions (center of each layer)
const float LAYER_0_Z = -LAYER_DEPTH * 0.5f;  // Back layer
const float LAYER_1_Z = 0.0f;                  // Middle layer
const float LAYER_2_Z = LAYER_DEPTH * 0.5f;   // Front layer

class RigidBody
{
public:
    btRigidBody* body;
    glm::mat4 modelMatrix;
    int layerIndex;  // Which layer (0, 1, or 2)
    bool allowRotation;  // Can be rotated by tools
    const ObjectDef* objectDef;  // Reference to object template

    RigidBody(btRigidBody* btBody, int layer = 1, const ObjectDef* def = nullptr) 
        : body(btBody), layerIndex(layer), allowRotation(false), objectDef(def)
    {
        updateMatrix();
        LockToLayer();
    }

    void LockToLayer()
    {
        if (!body)
            return;

        // Allow rotation ONLY on Z axis (objects can tip/rotate on their layer)
        body->setAngularFactor(btVector3(0, 0, 1));

        // Lock Z linear motion only - allow X and Y for gravity and movement
        body->setLinearFactor(btVector3(1, 1, 0));

        // Constrain Z position to the layer
        float layerZ = GetLayerZ(layerIndex);
        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);
        trans.getOrigin().setZ(layerZ);
        body->getMotionState()->setWorldTransform(trans);

        // Wake the body up
        body->activate(true);
    }

    void EnableRotation(bool enable)
    {
        allowRotation = enable;
        if (body)
        {
            if (enable)
                body->setAngularFactor(btVector3(0, 0, 1));  // Only Z-axis rotation (turn in place)
            else
                body->setAngularFactor(btVector3(0, 0, 0));  // No rotation
        }
    }

    float GetLayerZ(int layer)
    {
        switch (layer)
        {
        case 0: return LAYER_0_Z;
        case 1: return LAYER_1_Z;
        case 2: return LAYER_2_Z;
        default: return LAYER_1_Z;
        }
    }

    void updateMatrix()
    {
        if (!body || !body->getMotionState())
            return;

        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);

        btScalar m[16];
        trans.getOpenGLMatrix(m);
        modelMatrix = glm::make_mat4(m);
    }

    glm::vec3 GetPosition()
    {
        if (!body || !body->getMotionState())
            return glm::vec3(0.0f);

        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);
        btVector3 pos = trans.getOrigin();
        return glm::vec3(pos.getX(), pos.getY(), GetLayerZ(layerIndex));
    }

    void SetPosition(glm::vec3 pos)
    {
        if (!body || !body->getMotionState())
            return;

        btTransform trans;
        body->getMotionState()->getWorldTransform(trans);
        trans.setOrigin(btVector3(pos.x, pos.y, GetLayerZ(layerIndex)));
        body->getMotionState()->setWorldTransform(trans);
        body->activate();
    }

    void ApplyForce(glm::vec3 force)
    {
        if (body)
            body->applyCentralForce(btVector3(force.x, force.y, 0.0f));  // No Z force
    }

    void ApplyImpulse(glm::vec3 impulse)
    {
        if (body)
        {
            body->activate(true);
            body->applyCentralImpulse(btVector3(impulse.x, impulse.y, 0.0f));  // No Z impulse
        }
    }
};

class PhysicsEngine
{
private:
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* broadphase;
    btSequentialImpulseConstraintSolver* solver;

public:
    btDynamicsWorld* dynamicsWorld;
    std::vector<RigidBody*> rigidBodies;

    PhysicsEngine()
    {
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);
        broadphase = new btDbvtBroadphase();
        solver = new btSequentialImpulseConstraintSolver();
        dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

        // Improve collision handling
        dynamicsWorld->getDispatchInfo().m_useContinuous = true;
        dynamicsWorld->getSolverInfo().m_numIterations = 10;
    }

    ~PhysicsEngine()
    {
        for (size_t i = 0; i < rigidBodies.size(); i++)
        {
            RigidBody* body = rigidBodies[i];
            dynamicsWorld->removeRigidBody(body->body);

            btMotionState* motionState = body->body->getMotionState();
            if (motionState)
                delete motionState;

            btCollisionShape* shape = body->body->getCollisionShape();
            if (shape)
                delete shape;

            delete body->body;
            delete body;
        }

        delete dynamicsWorld;
        delete solver;
        delete broadphase;
        delete dispatcher;
        delete collisionConfiguration;
    }

    RigidBody* AddObjectFromTemplate(const ObjectDef* objDef, glm::vec3 position)
    {
        if (!objDef)
            return nullptr;

        // Create collision shape based on hitbox type
        btCollisionShape* shape = nullptr;

        if (objDef->physics.hitbox.type == "convexHull" && !objDef->physics.hitbox.vertices.empty())
        {
            // Create convex hull from vertices
            btConvexHullShape* hullShape = new btConvexHullShape();
            for (const auto& vertex : objDef->physics.hitbox.vertices)
            {
                hullShape->addPoint(btVector3(vertex.x, vertex.y, vertex.z));
            }
            shape = hullShape;
        }
        else
        {
            // Default to box
            glm::vec3 dims = objDef->physics.hitbox.dimensions;
            shape = new btBoxShape(btVector3(dims.x * 0.5f, dims.y * 0.5f, dims.z * 0.5f));
        }

        // Apply hitbox offset to position
        glm::vec3 finalPos = position + objDef->physics.hitbox.offset;

        btTransform transform;
        transform.setIdentity();
        float layerZ = (objDef->interaction.layerIndex == 0) ? LAYER_0_Z : 
                       (objDef->interaction.layerIndex == 2) ? LAYER_2_Z : LAYER_1_Z;
        transform.setOrigin(btVector3(finalPos.x, finalPos.y, layerZ));

        btMotionState* motionState = new btDefaultMotionState(transform);
        btVector3 inertia(0, 0, 0);

        if (objDef->physics.mass != 0.0f)
            shape->calculateLocalInertia(objDef->physics.mass, inertia);

        btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(objDef->physics.mass, motionState, shape, inertia);
        btRigidBody* rigidBody = new btRigidBody(rigidBodyCI);

        // Apply physics properties from ObjectDef
        rigidBody->setDamping(0.1f, 0.0f);
        rigidBody->setRestitution(objDef->physics.bounciness);
        rigidBody->setFriction(objDef->physics.friction);
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);

        // Prevent sleeping
        rigidBody->setSleepingThresholds(0.0f, 0.0f);
        rigidBody->activate(true);

        // Add to world
        dynamicsWorld->addRigidBody(rigidBody, 1, -1);

        // Create wrapper with ObjectDef reference
        RigidBody* wrapper = new RigidBody(rigidBody, objDef->interaction.layerIndex, objDef);
        rigidBodies.push_back(wrapper);
        return wrapper;
    }

    RigidBody* AddPlane(float height)
    {
        btCollisionShape* shape = new btStaticPlaneShape(btVector3(0, 1, 0), height);

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(0, height, 0));

        btMotionState* motionState = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(0.0f, motionState, shape, btVector3(0, 0, 0));
        btRigidBody* rigidBody = new btRigidBody(rigidBodyCI);
        dynamicsWorld->addRigidBody(rigidBody);

        RigidBody* wrapper = new RigidBody(rigidBody, 1, nullptr);
        rigidBodies.push_back(wrapper);
        return wrapper;
    }

    void StepSimulation(float deltaTime)
    {
        if (dynamicsWorld)
        {
            dynamicsWorld->stepSimulation(deltaTime, 10);

            // After physics step, enforce Z layer constraints
            for (size_t i = 0; i < rigidBodies.size(); i++)
            {
                RigidBody* rb = rigidBodies[i];

                btTransform trans;
                rb->body->getMotionState()->getWorldTransform(trans);
                float layerZ = rb->GetLayerZ(rb->layerIndex);
                btVector3 pos = trans.getOrigin();

                // Only correct Z if it drifts - allow Y to fall freely!
                if (glm::abs(pos.getZ() - layerZ) > 0.01f)
                {
                    pos.setZ(layerZ);
                    trans.setOrigin(pos);
                    rb->body->getMotionState()->setWorldTransform(trans);
                    rb->body->setWorldTransform(trans);
                }

                rb->updateMatrix();
            }
        }
    }

    RigidBody* RaycastClosest(glm::vec3 rayStart, glm::vec3 rayEnd)
    {
        btVector3 from(rayStart.x, rayStart.y, rayStart.z);
        btVector3 to(rayEnd.x, rayEnd.y, rayEnd.z);

        btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
        dynamicsWorld->rayTest(from, to, rayCallback);

        if (rayCallback.hasHit())
        {
            btRigidBody* hitBody = const_cast<btRigidBody*>(btRigidBody::upcast(rayCallback.m_collisionObject));
            if (hitBody)
            {
                for (size_t i = 0; i < rigidBodies.size(); i++)
                {
                    if (rigidBodies[i]->body == hitBody)
                        return rigidBodies[i];
                }
            }
        }
        return nullptr;
    }
};

#endif
