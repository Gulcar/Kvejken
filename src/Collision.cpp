#include "Collision.h"
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>

namespace kvejken::collision
{
    namespace
    {
        struct Triangle
        {
            glm::vec3 v1, v2, v3;
        };

        std::vector<Triangle> m_triangles;
    }

    void build_triangle_bvh(const Model& model, glm::vec3 position, glm::quat rotation, glm::vec3 scale)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::toMat4(rotation)
            * glm::scale(glm::mat4(1.0f), scale);

        for (const auto& mesh : model.meshes())
        {
            for (int i = 0; i < mesh.vertices().size(); i += 3)
            {
                m_triangles.push_back(Triangle{
                    transform * glm::vec4(mesh.vertices()[i].position, 1.0f),
                    transform * glm::vec4(mesh.vertices()[i + 1].position, 1.0f),
                    transform * glm::vec4(mesh.vertices()[i + 2].position, 1.0f)
                });
            }
        }
    }

    bool raycast(glm::vec3 position, glm::vec3 direction, glm::vec3* out_hit_position, float *out_dist)
    {
        float closest_dist = 9999.0f;

        for (const auto& tri : m_triangles)
        {
            glm::vec2 bary_coords;
            float distance;
            if (glm::intersectRayTriangle(position, direction, tri.v1, tri.v2, tri.v3, bary_coords, distance))
            {
                if (distance > 0.0f && distance < closest_dist)
                {
                    closest_dist = distance;
                }
            }
        }

        if (closest_dist != 9999.0f)
        {
            if (out_hit_position) *out_hit_position = position + direction * closest_dist;
            if (out_dist) *out_dist = closest_dist;
            return true;
        }
        else
        {
            return false;
        }
    }
}
