#pragma once
#include <glm/glm.hpp>

extern float gravity;

struct PhysicsBody
{
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec3 size;
    bool      grounded = false;
};

struct Sphere
{
    glm::vec3 pos;
    glm::vec3 vel;
    float     radius;
    float     mass;
};

void UpdatePhysics(PhysicsBody& b, float dt);
void UpdateSphere(Sphere& s, float dt);

bool AABBCollide(const PhysicsBody& A, const PhysicsBody& B);
void ResolveAABB(PhysicsBody& A, const PhysicsBody& B);
void ResolveSphereSphere(Sphere& A, Sphere& B);
void ResolveSphereAABB(Sphere& s, PhysicsBody& box);