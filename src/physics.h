#ifndef PHYSICS_H
#define PHYSICS_H

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

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

    RigidBody(btRigidBody* btBody, int layer = 1) : body(btBody), layerIndex(layer), allowRotation(false)
    {
        updateMatrix();
        LockToLayer();
    }

    void LockToLayer()
    {
        if (!body)
            return;

        // Lock all rotations by default (no spinning)
        body->setAngularFactor(btVector3(0, 0, 0));

        // Lock Z linear motion
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
                body->setAngularFactor(btVector3(0, 1, 0));  // Only Y-axis rotation (turn in place)
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

        // Only constrain Z to layer after physics update
        float layerZ = GetLayerZ(layerIndex);
        btVector3 pos = trans.getOrigin();
        trans.setOrigin(btVector3(pos.getX(), pos.getY(), layerZ));

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

    RigidBody* AddBox(glm::vec3 position, glm::vec3 scale, float mass = 1.0f, int layer = 1)
    {
        btCollisionShape* shape = new btBoxShape(btVector3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f));

        btTransform transform;
        transform.setIdentity();
        // Constrain to layer Z
        float layerZ = (layer == 0) ? LAYER_0_Z : (layer == 2) ? LAYER_2_Z : LAYER_1_Z;
        transform.setOrigin(btVector3(position.x, position.y, layerZ));

        btMotionState* motionState = new btDefaultMotionState(transform);
        btVector3 inertia(0, 0, 0);

        if (mass != 0.0f)
            shape->calculateLocalInertia(mass, inertia);

        btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
        btRigidBody* rigidBody = new btRigidBody(rigidBodyCI);
        dynamicsWorld->addRigidBody(rigidBody);

        RigidBody* wrapper = new RigidBody(rigidBody, layer);
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

        RigidBody* wrapper = new RigidBody(rigidBody, 1);  // Plane on middle layer
        rigidBodies.push_back(wrapper);
        return wrapper;
    }

    void StepSimulation(float deltaTime)
    {
        if (dynamicsWorld)
        {
            dynamicsWorld->stepSimulation(deltaTime, 10);
            for (size_t i = 0; i < rigidBodies.size(); i++)
            {
                rigidBodies[i]->updateMatrix();
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
