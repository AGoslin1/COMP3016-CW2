#include "Physics.h"
#include <cmath>

float gravity = -9.81f;

void UpdatePhysics(PhysicsBody& b, float dt)
{
    b.vel.y += gravity * dt;
    if (b.vel.y < -20) b.vel.y = -20;

    b.pos += b.vel * dt;

    if (b.pos.y - b.size.y < 0)
    {
        b.pos.y = b.size.y;
        b.vel.y = 0;
        b.grounded = true;
    }
    else b.grounded = false;
}

void UpdateSphere(Sphere& s, float dt)
{
    s.vel.y += gravity * dt;
    if (s.vel.y < -20) s.vel.y = -20;

    s.pos += s.vel * dt;

    if (s.pos.y - s.radius < 0)
    {
        s.pos.y = s.radius;
        s.vel.y *= -0.4f;
    }
}

bool AABBCollide(const PhysicsBody& A, const PhysicsBody& B)
{
    return (std::fabs(A.pos.x - B.pos.x) <= (A.size.x + B.size.x)) &&
        (std::fabs(A.pos.y - B.pos.y) <= (A.size.y + B.size.y)) &&
        (std::fabs(A.pos.z - B.pos.z) <= (A.size.z + B.size.z));
}

void ResolveAABB(PhysicsBody& A, const PhysicsBody& B)
{
    glm::vec3 diff = A.pos - B.pos;

    if (std::fabs(diff.x) > std::fabs(diff.z))
    {
        float overlap = (A.size.x + B.size.x) - std::fabs(diff.x);
        A.pos.x += (diff.x > 0 ? overlap : -overlap);
    }
    else
    {
        float overlap = (A.size.z + B.size.z) - std::fabs(diff.z);
        A.pos.z += (diff.z > 0 ? overlap : -overlap);
    }
}

void ResolveSphereSphere(Sphere& A, Sphere& B)
{
    glm::vec3 diff = B.pos - A.pos;
    float dist2 = glm::dot(diff, diff);
    float minDist = A.radius + B.radius;

    if (dist2 >= minDist * minDist) return;

    float dist = std::sqrt(dist2);
    if (dist < 0.0001f) dist = 0.0001f;

    glm::vec3 normal = diff / dist;
    float penetration = minDist - dist;

    A.pos -= normal * (penetration * 0.5f);
    B.pos += normal * (penetration * 0.5f);

    glm::vec3 relVel = B.vel - A.vel;
    float velN = glm::dot(relVel, normal);
    if (velN > 0) return;

    float j = -(1 + 0.4f) * velN / (1 / A.mass + 1 / B.mass);
    glm::vec3 impulse = j * normal;

    A.vel -= impulse / A.mass;
    B.vel += impulse / B.mass;
}

void ResolveSphereAABB(Sphere& s, PhysicsBody& box)
{
    glm::vec3 minB = box.pos - box.size;
    glm::vec3 maxB = box.pos + box.size;

    glm::vec3 closest = glm::clamp(s.pos, minB, maxB);
    glm::vec3 diff = s.pos - closest;

    float dist2 = glm::dot(diff, diff);
    if (dist2 > s.radius * s.radius) return;

    float dist = std::sqrt(dist2);
    if (dist < 0.0001f) dist = 0.0001f;

    glm::vec3 normal = diff / dist;
    float penetration = s.radius - dist;

    s.pos += normal * penetration;

    float vN = glm::dot(s.vel, normal);
    s.vel -= normal * vN;
    s.vel *= 0.6f;
}