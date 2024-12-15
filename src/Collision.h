#pragma once
#include "Model.h"

namespace kvejken::collision
{
    void build_triangle_bvh(const Model& model, glm::vec3 position, glm::quat rotation, glm::vec3 scale);

    bool raycast(glm::vec3 position, glm::vec3 direction, float max_dist, glm::vec3* out_hit_position, float* out_dist);
}
