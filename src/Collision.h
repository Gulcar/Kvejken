#pragma once
#include "Model.h"
#include <vector>

namespace kvejken::collision
{
    void build_triangle_bvh(const Model& model, glm::vec3 position, glm::quat rotation, glm::vec3 scale);

    bool raycast(glm::vec3 position, glm::vec3 direction, float max_dist, glm::vec3* out_hit_position, float* out_dist);

    glm::vec3 closest_point_on_line(glm::vec3 a, glm::vec3 b, glm::vec3 p);

    bool sphere_triangle_intersection(glm::vec3 center, float radius, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3* out_pen_normal, float* out_pen_depth);
    bool sphere_collision(glm::vec3 center, float radius, std::vector<glm::vec3>* out_penetrations, glm::vec3* out_biggest_pen, glm::vec3* out_new_center);
}
