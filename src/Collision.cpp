#include "Collision.h"
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include "Utils.h"

namespace kvejken::collision
{
    namespace
    {
        struct Triangle
        {
            glm::vec3 v1, v2, v3;
            glm::vec3 center;
        };

        struct BVHNode
        {
            glm::vec3 bounds_min, bounds_max;
            uint32_t left_child, right_child;
            bool is_leaf;
        };

        std::vector<BVHNode> m_bvh_nodes;
        std::vector<Triangle> m_triangles;
    }

    static void update_node_bounds(uint32_t node_index)
    {
        BVHNode& node = m_bvh_nodes[node_index];
        ASSERT(node.is_leaf);
        node.bounds_min = glm::vec3(1e30f);
        node.bounds_max = glm::vec3(-1e30f);

        for (int i = node.left_child; i <= node.right_child; i++)
        {
            node.bounds_min = glm::min(node.bounds_min, m_triangles[i].v1, m_triangles[i].v2, m_triangles[i].v3);
            node.bounds_max = glm::max(node.bounds_max, m_triangles[i].v1, m_triangles[i].v2, m_triangles[i].v3);
        }
    }

    // https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
    static void subdivide_node(uint32_t node_index)
    {
        BVHNode& node = m_bvh_nodes[node_index];
        ASSERT(node.is_leaf);

        constexpr int MAX_TRI_PER_NODE = 4;
        if (node.right_child - node.left_child + 1 <= MAX_TRI_PER_NODE)
            return;

        glm::vec3 size = node.bounds_max - node.bounds_min;
        int axis = 0;
        if (size.y > size.x) axis = 1;
        if (size.z > size[axis]) axis = 2;
        
        float split_pos = node.bounds_min[axis] + size[axis] * 0.5f;

        // partition
        int i = node.left_child;
        int j = node.right_child;
        while (i <= j)
        {
            if (m_triangles[i].center[axis] > split_pos)
            {
                std::swap(m_triangles[i], m_triangles[j]);
                j--;
            }
            else
            {
                i++;
            }
        }

        if (i == node.left_child || j == node.right_child)
            return;

        BVHNode left, right;
        left.left_child = node.left_child;
        left.right_child = i - 1;
        left.is_leaf = true;

        right.left_child = i;
        right.right_child = node.right_child;
        right.is_leaf = true;

        node.is_leaf = false;
        uint32_t left_index = m_bvh_nodes.size();
        uint32_t right_index = m_bvh_nodes.size() + 1;
        node.left_child = left_index;
        node.right_child = right_index;
        m_bvh_nodes.push_back(left); // pazi invalidejta node reference
        m_bvh_nodes.push_back(right);

        update_node_bounds(left_index);
        update_node_bounds(right_index);

        subdivide_node(left_index);
        subdivide_node(right_index);
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
                glm::vec3 v1 = transform * glm::vec4(mesh.vertices()[i].position, 1.0f);
                glm::vec3 v2 = transform * glm::vec4(mesh.vertices()[i + 1].position, 1.0f);
                glm::vec3 v3 = transform * glm::vec4(mesh.vertices()[i + 2].position, 1.0f);
                m_triangles.push_back(Triangle{
                    v1, v2, v3,
                    (v1 + v2 + v3) / 3.0f
                });
            }
        }

        m_bvh_nodes.clear();

        BVHNode root;
        root.left_child = 0;
        root.right_child = m_triangles.size() - 1;
        root.is_leaf = true;
        m_bvh_nodes.push_back(root);
        update_node_bounds(0);

        subdivide_node(0);
    }

    // https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
    bool ray_aabb_intersection(glm::vec3 bmin, glm::vec3 bmax, glm::vec3 position, glm::vec3 direction, float max_dist)
    {
        float tx1 = (bmin.x - position.x) / direction.x, tx2 = (bmax.x - position.x) / direction.x;
        float tmin = std::min(tx1, tx2), tmax = std::max(tx1, tx2);
        float ty1 = (bmin.y - position.y) / direction.y, ty2 = (bmax.y - position.y) / direction.y;
        tmin = std::max(tmin, std::min(ty1, ty2)), tmax = std::min(tmax, std::max(ty1, ty2));
        float tz1 = (bmin.z - position.z) / direction.z, tz2 = (bmax.z - position.z) / direction.z;
        tmin = std::max(tmin, std::min(tz1, tz2)), tmax = std::min(tmax, std::max(tz1, tz2));
        return tmax >= tmin && tmin <= max_dist && tmax > 0;
    }

    static float raycast_bvh(uint32_t node_index, const glm::vec3& position, const glm::vec3& direction, float max_dist)
    {
        BVHNode& node = m_bvh_nodes[node_index];

        if (node.is_leaf)
        {
            for (int i = node.left_child; i <= node.right_child; i++)
            {
                Triangle& tri = m_triangles[i];

                glm::vec2 bary_coords;
                float distance;
                if (glm::intersectRayTriangle(position, direction, tri.v1, tri.v2, tri.v3, bary_coords, distance))
                {
                    if (distance > 0.0f && distance < max_dist)
                    {
                        max_dist = distance;
                    }
                }
            }
        }
        else
        {
            BVHNode& left = m_bvh_nodes[node.left_child];
            BVHNode& right = m_bvh_nodes[node.right_child];

            if (ray_aabb_intersection(left.bounds_min, left.bounds_max, position, direction, max_dist))
            {
                max_dist = raycast_bvh(node.left_child, position, direction, max_dist);
            }
            if (ray_aabb_intersection(right.bounds_min, right.bounds_max, position, direction, max_dist))
            {
                max_dist = raycast_bvh(node.right_child, position, direction, max_dist);
            }
        }

        return max_dist;
    }

    std::optional<RaycastHit> raycast(glm::vec3 position, glm::vec3 direction, float max_dist)
    {
        float closest_dist = raycast_bvh(0, position, direction, max_dist);

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
        float ground_normal_y = std::cos(glm::radians(max_ground_angle));

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
