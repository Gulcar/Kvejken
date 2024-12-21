#pragma once
#include "Model.h"
#include <vector>
#include <optional>

namespace kvejken::collision
{
    void build_triangle_bvh(const Model& model, glm::vec3 position, glm::quat rotation, glm::vec3 scale);

    struct RaycastHit
    {
        glm::vec3 position;
        float distance;
    };
    std::optional<RaycastHit> raycast(glm::vec3 position, glm::vec3 direction, float max_dist = 9999.0f);

    glm::vec3 closest_point_on_line(glm::vec3 a, glm::vec3 b, glm::vec3 p);

    struct Intersection
    {
        glm::vec3 normal;
        float depth;
    };
    std::optional<Intersection> sphere_triangle_intersection(glm::vec3 center, float radius, glm::vec3 a, glm::vec3 b, glm::vec3 c);

    struct ResolvedCollision
    {
        glm::vec3 new_center;
        glm::vec3 new_velocity;
        bool ground_collision;
    };
    std::optional<ResolvedCollision> sphere_collision(glm::vec3 center, float radius, glm::vec3 velocity = glm::vec3(0.0f), float slide_threshold = 0.01f);
}
