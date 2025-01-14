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

    std::optional<RaycastHit> raycast(glm::vec3 position, glm::vec3 direction, float max_dist)
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
            RaycastHit hit;
            hit.position = position + direction * closest_dist;
            hit.distance = closest_dist;
            return hit;
        }
        return std::nullopt;
    }

    glm::vec3 closest_point_on_line(glm::vec3 a, glm::vec3 b, glm::vec3 p)
    {
        glm::vec3 n = glm::normalize(b - a);
        float t = glm::dot(p - a, n);
        return a + glm::clamp(t, 0.0f, 1.0f) * n;
    }

    // https://wickedengine.net/2020/04/capsule-collision-detection/
    // TODO: mogoce bi lahko forsiral da se collision lahko resi samo v tisto smer kamor je trikotnik obrnjen,
    // da bi tezje clipal cez tla ali pa sel v steno, ker so vsi trikotnik ze pravilno obrneni za GL_CULL_FACE
    std::optional<Intersection> sphere_triangle_intersection(glm::vec3 center, float radius, glm::vec3 a, glm::vec3 b, glm::vec3 c)
    {
        glm::vec3 tri_normal = glm::normalize(glm::cross(b - a, c - a));
        float dist = glm::dot(center - a, tri_normal);

        if (dist < -radius || dist > radius)
            return std::nullopt;

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
                return std::nullopt;
            }
        }

        float len = glm::length(intersection_vec);

        Intersection out;
        out.normal = intersection_vec / len;
        out.depth = radius - len;
        return out;
    }

    std::optional<ResolvedCollision> sphere_collision(glm::vec3 center, float radius, glm::vec3 velocity, float max_ground_angle, float slide_threshold)
    {
        bool any = false;
        bool ground_collison = false;

        bool only_y_movement = glm::length(glm::vec2(velocity.x, velocity.z)) < slide_threshold;
        float ground_normal_y = std::cos(max_ground_angle);

        for (const auto& tri : m_triangles)
        {
            auto intersection = sphere_triangle_intersection(center, radius, tri.v1, tri.v2, tri.v3);
            if (intersection)
            {
                any = true;

                if (only_y_movement)
                {
                    center.y += (intersection->normal * intersection->depth).y;
                }
                else
                {
                    center += intersection->normal * intersection->depth;
                }

                if (intersection->normal.y > ground_normal_y)
                    ground_collison = true;

                float vel_on_normal = glm::dot(velocity, -intersection->normal);
                velocity += glm::max(vel_on_normal, 0.0f) * intersection->normal;
            }
        }

        if (any)
        {
            ResolvedCollision out;
            out.new_center = center;
            out.new_velocity = velocity;
            out.ground_collision = ground_collison;
            return out;
        }
        return std::nullopt;
    }
}
