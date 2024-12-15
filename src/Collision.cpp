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
        // TODO: bvh

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

    bool raycast(glm::vec3 position, glm::vec3 direction, float max_dist, glm::vec3* out_hit_position, float* out_dist)
    {
        float closest_dist = max_dist;

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

        if (closest_dist < max_dist)
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

    glm::vec3 closest_point_on_line(glm::vec3 a, glm::vec3 b, glm::vec3 p)
    {
        glm::vec3 n = glm::normalize(b - a);
        float t = glm::dot(p - a, n);
        return a + glm::clamp(t, 0.0f, 1.0f) * n;
    }

    // https://wickedengine.net/2020/04/capsule-collision-detection/
    bool sphere_triangle_intersection(glm::vec3 center, float radius, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3* out_pen_normal, float* out_pen_depth)
    {
        glm::vec3 tri_normal = glm::normalize(glm::cross(b - a, c - a));
        float dist = glm::dot(center - a, tri_normal);

        if (dist < -radius || dist > radius)
            return false;

        glm::vec3 p = center - tri_normal * dist;

        glm::vec3 c0 = glm::cross(p - a, b - a);
        glm::vec3 c1 = glm::cross(p - b, c - b);
        glm::vec3 c2 = glm::cross(p - c, a - c);
        bool inside = glm::dot(c0, tri_normal) <= 0 && glm::dot(c1, tri_normal) <= 0 && glm::dot(c2, tri_normal) <= 0;

        glm::vec3 intersection_vec;

        if (inside)
        {
            intersection_vec = center - p;
        }
        else
        {
            float radiussq = radius * radius;

            // Edge 1:
            glm::vec3 point1 = closest_point_on_line(a, b, center);
            glm::vec3 v1 = center - point1;
            float distsq1 = dot(v1, v1);

            // Edge 2:
            glm::vec3 point2 = closest_point_on_line(b, c, center);
            glm::vec3 v2 = center - point2;
            float distsq2 = dot(v2, v2);

            // Edge 3:
            glm::vec3 point3 = closest_point_on_line(c, a, center);
            glm::vec3 v3 = center - point3;
            float distsq3 = dot(v3, v3);

            if (distsq1 <= distsq2 && distsq1 <= distsq3 && distsq1 < radiussq)
            {
                intersection_vec = v1;
            }
            else if (distsq2 <= distsq3 && distsq2 <= distsq1 && distsq2 < radiussq)
            {
                intersection_vec = v2;
            }
            else if (distsq3 <= distsq2 && distsq3 <= distsq1 && distsq3 < radiussq)
            {
                intersection_vec = v3;
            }
            else
            {
                return false;
            }
        }

        float len = glm::length(intersection_vec);
        if (out_pen_normal) *out_pen_normal = intersection_vec / len;
        if (out_pen_depth) *out_pen_depth = radius - len;
        return true;
    }

    bool sphere_collision(glm::vec3 center, float radius, std::vector<glm::vec3>* out_penetrations, glm::vec3* out_biggest_pen, glm::vec3* out_new_center)
    {
        float biggest = 0.0f;
        bool any = false;

        for (const auto& tri : m_triangles)
        {
            glm::vec3 normal;
            float depth;
            if (sphere_triangle_intersection(center, radius, tri.v1, tri.v2, tri.v3, &normal, &depth))
            {
                any = true;
                glm::vec3 pen = normal * depth;

                if (out_penetrations)
                    out_penetrations->push_back(pen);

                if (depth > biggest)
                {
                    biggest = depth;
                    if (out_biggest_pen)
                        *out_biggest_pen = pen;
                }

                if (out_new_center)
                    center += pen;
            }
        }

        if (out_new_center)
            *out_new_center = center;

        return any;
    }
}
