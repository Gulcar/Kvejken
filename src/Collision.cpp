#include "Collision.h"
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include "Utils.h"
#include "ECS.h"
#include "Components.h"
#include <chrono>

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
            AABB bounds;
            uint32_t left_child, right_child;
            bool is_leaf;
        };

        std::vector<BVHNode> m_bvh_nodes;
        std::vector<Triangle> m_triangles;

        std::thread m_bvh_building_thread;
        std::atomic_bool m_bvh_building_thread_done = false;
        std::chrono::steady_clock::time_point m_bvh_build_start_time;
    }

    static void update_node_bounds(BVHNode& node)
    {
        ASSERT(node.is_leaf);
        node.bounds.min = glm::vec3(1e30f);
        node.bounds.max = glm::vec3(-1e30f);

        for (int i = node.left_child; i <= node.right_child; i++)
        {
            node.bounds.min = glm::min(node.bounds.min, m_triangles[i].v1, m_triangles[i].v2, m_triangles[i].v3);
            node.bounds.max = glm::max(node.bounds.max, m_triangles[i].v1, m_triangles[i].v2, m_triangles[i].v3);
        }
    }

    float evaluate_sah(BVHNode& node, float split_pos, int axis)
    {
        ASSERT(node.is_leaf);

        AABB left_bounds = { glm::vec3(1e30f), glm::vec3(-1e30f) };
        AABB right_bounds = { glm::vec3(1e30f), glm::vec3(-1e30f) };
        int left_count = 0;
        int right_count = 0;

        for (int i = node.left_child; i <= node.right_child; i++)
        {
            if (m_triangles[i].center[axis] > split_pos)
            {
                right_count++;
                right_bounds.min = glm::min(right_bounds.min, m_triangles[i].v1, m_triangles[i].v2, m_triangles[i].v3);
                right_bounds.max = glm::max(right_bounds.max, m_triangles[i].v1, m_triangles[i].v2, m_triangles[i].v3);
            }
            else
            {
                left_count++;
                left_bounds.min = glm::min(left_bounds.min, m_triangles[i].v1, m_triangles[i].v2, m_triangles[i].v3);
                left_bounds.max = glm::max(left_bounds.max, m_triangles[i].v1, m_triangles[i].v2, m_triangles[i].v3);
            }
        }

        glm::vec3 l_ext = left_bounds.max - left_bounds.min;
        glm::vec3 r_ext = right_bounds.max - right_bounds.min;
        return left_count * (l_ext.x * l_ext.y + l_ext.y * l_ext.z + l_ext.x * l_ext.z)
            + right_count * (r_ext.x * r_ext.y + r_ext.y * r_ext.z + r_ext.x * r_ext.z);
    }

    void find_best_split(BVHNode& node, float* out_sah, float* out_split_pos, int* out_axis)
    {
        ASSERT(node.is_leaf);
        float best_sah = 1e30f;
        float best_split_pos = 0;
        int best_axis = 0;

        glm::vec3 size = node.bounds.max - node.bounds.min;

#ifdef NDEBUG
        constexpr int NUM_SAH_TESTS = 128;
#else
        constexpr int NUM_SAH_TESTS = 6;
#endif
        for (int i = 0; i < NUM_SAH_TESTS; i++)
        {
            float t = (i + 1) / (float)(NUM_SAH_TESTS + 1);

            for (int j = 0; j < 3; j++)
            {
                float split_pos = node.bounds.min[j] + size[j] * t;
                float sah = evaluate_sah(node, split_pos, j);
                if (sah < best_sah)
                {
                    best_sah = sah;
                    best_split_pos = split_pos;
                    best_axis = j;
                }
            }
        }

        *out_sah = best_sah;
        *out_split_pos = best_split_pos;
        *out_axis = best_axis;
    }

    // https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
    static void subdivide_node(uint32_t node_index)
    {
        BVHNode& node = m_bvh_nodes[node_index];
        ASSERT(node.is_leaf);

        float sah = 0;
        float split_pos = 0;
        int axis = 0;
        find_best_split(node, &sah, &split_pos, &axis);

        glm::vec3 exts = node.bounds.max - node.bounds.min;
        float parent_sah = (node.right_child - node.left_child + 1) * (exts.x * exts.y + exts.y * exts.z + exts.x * exts.z);
        if (sah > parent_sah)
            return;

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

        update_node_bounds(m_bvh_nodes[left_index]);
        update_node_bounds(m_bvh_nodes[right_index]);

        subdivide_node(left_index);
        subdivide_node(right_index);
    }

    static void build_triangle_bvh_thread()
    {
        BVHNode root;
        root.left_child = 0;
        root.right_child = m_triangles.size() - 1;
        root.is_leaf = true;
        m_bvh_nodes.push_back(root);
        update_node_bounds(m_bvh_nodes[0]);

        subdivide_node(0);

        m_bvh_building_thread_done = true;
    }

    void build_triangle_bvh(const Model& model, glm::vec3 position, glm::quat rotation, glm::vec3 scale)
    {
        ASSERT(m_bvh_building_thread.joinable() == false);

        m_bvh_nodes.clear();
        m_triangles.clear();

        size_t total_vertices = 0;
        for (const auto& mesh : model.meshes()) {
            total_vertices += mesh.vertices().size();
        }
        m_triangles.reserve(total_vertices / 3);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::toMat4(rotation)
            * glm::scale(glm::mat4(1.0f), scale);

        int skipped = 0;

        for (const auto& mesh : model.meshes())
        {
            for (int i = 0; i < mesh.vertices().size(); i += 3)
            {
                glm::vec3 v1 = transform * glm::vec4(mesh.vertices()[i].position, 1.0f);
                glm::vec3 v2 = transform * glm::vec4(mesh.vertices()[i + 1].position, 1.0f);
                glm::vec3 v3 = transform * glm::vec4(mesh.vertices()[i + 2].position, 1.0f);

                if (glm::distance2(v1, v2) < 0.064f * 0.064f ||
                    glm::distance2(v2, v3) < 0.064f * 0.064f ||
                    glm::distance2(v1, v3) < 0.064f * 0.064f)
                {
                    skipped++;
                    continue;
                }

                m_triangles.push_back(Triangle{
                    v1, v2, v3,
                    (v1 + v2 + v3) / 3.0f
                });
            }
        }

        printf("triangles ignored for collision: %d\n", skipped);
        
        m_bvh_building_thread_done = false;
        m_bvh_building_thread = std::thread(build_triangle_bvh_thread);
        m_bvh_build_start_time = std::chrono::steady_clock::now();
    }

    static void print_bvh_build_thread_time()
    {
        auto stop_time = std::chrono::steady_clock::now();
        std::chrono::duration<float> duration = stop_time - m_bvh_build_start_time;
        printf("triangle bvh built  %.2f ms\n", duration.count() * 1000.0f);
    }

    void check_bvh_build_thread()
    {
        if (m_bvh_building_thread_done && m_bvh_building_thread.joinable())
        {
            m_bvh_building_thread.join();
            print_bvh_build_thread_time();
        }
    }

    // https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
    std::optional<float> ray_aabb_intersection(const AABB& aabb, glm::vec3 position, glm::vec3 direction, float max_dist)
    {
        float tx1 = (aabb.min.x - position.x) / direction.x, tx2 = (aabb.max.x - position.x) / direction.x;
        float tmin = std::min(tx1, tx2), tmax = std::max(tx1, tx2);
        float ty1 = (aabb.min.y - position.y) / direction.y, ty2 = (aabb.max.y - position.y) / direction.y;
        tmin = std::max(tmin, std::min(ty1, ty2)), tmax = std::min(tmax, std::max(ty1, ty2));
        float tz1 = (aabb.min.z - position.z) / direction.z, tz2 = (aabb.max.z - position.z) / direction.z;
        tmin = std::max(tmin, std::min(tz1, tz2)), tmax = std::min(tmax, std::max(tz1, tz2));
        if (tmax >= tmin && tmin <= max_dist && tmax > 0)
            return tmin;
        return std::nullopt;
    }

    bool sphere_aabb_intersection(const AABB& aabb, glm::vec3 center, float radius)
    {
        glm::vec3 closest_in_aabb = glm::clamp(center, aabb.min, aabb.max);
        return glm::distance2(closest_in_aabb, center) < radius * radius;
    }

    static std::pair<Triangle, Triangle> rect_to_tris(const RectCollider& rect, const Transform& transform)
    {
        glm::vec3 right = transform.rotation * glm::vec3(1, 0, 0);
        glm::vec3 up = transform.rotation * glm::vec3(0, 1, 0);
        glm::vec3 center = transform.position + transform.rotation * rect.center_offset;

        float hw = rect.width / 2.0f;
        float hh = rect.height / 2.0f;

        glm::vec3 v1 = center + hw * right + hh * up;
        glm::vec3 v2 = center + hw * right - hh * up;
        glm::vec3 v3 = center - hw * right - hh * up;
        glm::vec3 v4 = center - hw * right + hh * up;

        return {
            Triangle{ v1, v2, v3, (v1+v2+v3)/3.0f },
            Triangle{ v3, v4, v1, (v3+v4+v1)/3.0f }
        };
    }

    static float raycast_bvh(uint32_t node_index, const glm::vec3& position, const glm::vec3& direction, float max_dist)
    {
        BVHNode* node = &m_bvh_nodes[node_index];
        static std::vector<BVHNode*> stack;
        stack.clear();

        while (node != nullptr)
        {
            if (node->is_leaf)
            {
                for (int i = node->left_child; i <= node->right_child; i++)
                {
                    Triangle& tri = m_triangles[i];

                    glm::vec2 bary_coords;
                    float distance;
                    if (glm::intersectRayTriangle(position, direction, tri.v1, tri.v2, tri.v3, bary_coords, distance))
                    {
                        if (distance > 0.0f && distance < max_dist)
                            max_dist = distance;
                    }
                }

                if (stack.size() > 0)
                {
                    node = stack.back();
                    stack.pop_back();
                }
                else
                {
                    node = nullptr;
                }
            }
            else
            {
                BVHNode* left = &m_bvh_nodes[node->left_child];
                BVHNode* right = &m_bvh_nodes[node->right_child];

                std::optional<float> left_dist = ray_aabb_intersection(left->bounds, position, direction, max_dist);
                std::optional<float> right_dist = ray_aabb_intersection(right->bounds, position, direction, max_dist);

                if (left_dist && right_dist)
                {
                    if (*left_dist < *right_dist)
                    {
                        node = left;
                        stack.push_back(right);
                    }
                    else
                    {
                        node = right;
                        stack.push_back(left);
                    }
                }
                else if (left_dist)
                {
                    node = left;
                }
                else if (right_dist)
                {
                    node = right;
                }
                else if (stack.size() > 0)
                {
                    node = stack.back();
                    stack.pop_back();
                }
                else
                {
                    node = nullptr;
                }
            }
        }

        return max_dist;
    }

    std::optional<RaycastHit> raycast(glm::vec3 position, glm::vec3 direction, float max_dist)
    {
        if (m_bvh_building_thread.joinable())
        {
            m_bvh_building_thread.join();
            print_bvh_build_thread_time();
        }

        float closest_dist = raycast_bvh(0, position, direction, max_dist);

        for (const auto [rect, transform] : ecs::get_components<RectCollider, Transform>())
        {
            auto [t1, t2] = rect_to_tris(rect, transform);
            glm::vec2 bary_coords;
            float distance;
            if (glm::intersectRayTriangle(position, direction, t1.v1, t1.v2, t1.v3, bary_coords, distance))
            {
                if (distance > 0.0f && distance < max_dist)
                    max_dist = distance;
            }
            if (glm::intersectRayTriangle(position, direction, t2.v1, t2.v2, t2.v3, bary_coords, distance))
            {
                if (distance > 0.0f && distance < max_dist)
                    max_dist = distance;
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
        glm::vec3 n = b - a;
        float t = glm::dot(p - a, n) / glm::dot(n, n);
        return a + glm::clamp(t, 0.0f, 1.0f) * n;
    }

    // https://wickedengine.net/2020/04/capsule-collision-detection/
    std::optional<Intersection> sphere_triangle_intersection(glm::vec3 center, float radius, glm::vec3 a, glm::vec3 b, glm::vec3 c)
    {
        glm::vec3 tri_normal = glm::normalize(glm::cross(b - a, c - a));
        float dist = glm::dot(center - a, tri_normal);

        if (dist < 0.0f)
            return std::nullopt;
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

#ifdef KVEJKEN_DEBUG_PHYSICS
    #define DEBUG_VECTOR(v) debug_file << #v << " [" << v.x << ", " << v.y << ", " << v.z << "]\n"
    #define DEBUG_VAR(v) debug_file << #v << " " << v << "\n"
#else
    #define DEBUG_VECTOR(v) 
    #define DEBUG_VAR(v) 
#endif

    std::optional<ResolvedCollision> sphere_collision(glm::vec3 center, float radius, glm::vec3 velocity, float max_ground_angle, float slide_threshold)
    {
        if (m_bvh_building_thread.joinable())
        {
            m_bvh_building_thread.join();
            print_bvh_build_thread_time();
        }

#ifdef KVEJKEN_DEBUG_PHYSICS
        static std::ofstream debug_file("physics_debug.txt");
        ASSERT(debug_file.is_open() && debug_file.good());

        debug_file << "\n-------------------------------------------------\n";
        static size_t idx = 0;
        debug_file << "sphere_collision " << idx++ << "\n";
        DEBUG_VECTOR(center);
        DEBUG_VAR(radius);
        DEBUG_VECTOR(velocity);
        DEBUG_VAR(max_ground_angle);
        DEBUG_VAR(slide_threshold);
        debug_file << "\n";
#else
        static utils::NullStreamBuf null_buf;
        static std::ostream debug_file(&null_buf);
#endif

        bool any = false;
        bool ground_collison = false;

        bool only_y_movement = glm::length(glm::vec2(velocity.x, velocity.z)) < slide_threshold;
        float ground_normal_y = std::cos(glm::radians(max_ground_angle));

        DEBUG_VAR(only_y_movement);
        DEBUG_VAR(ground_normal_y);
        debug_file << "\n";

        BVHNode* node = &m_bvh_nodes[0];
        static std::vector<BVHNode*> stack;
        stack.clear();

        static std::vector<std::pair<const Triangle*, float>> close_triangles;
        close_triangles.clear();


        while (node != nullptr)
        {
            if (node->is_leaf)
            {
                for (int i = node->left_child; i <= node->right_child; i++)
                {
                    const auto& tri = m_triangles[i];
                    if (auto collision = sphere_triangle_intersection(center, radius * 1.6f, tri.v1, tri.v2, tri.v3))
                    {
                        DEBUG_VECTOR(tri.v1);
                        DEBUG_VECTOR(tri.v2);
                        DEBUG_VECTOR(tri.v3);
                        DEBUG_VAR(collision->depth);
                        close_triangles.push_back({ &tri, collision->depth });
                    }
                }

                if (stack.size() > 0)
                {
                    node = stack.back();
                    stack.pop_back();
                }
                else
                {
                    node = nullptr;
                }
            }
            else
            {
                BVHNode* left = &m_bvh_nodes[node->left_child];
                BVHNode* right = &m_bvh_nodes[node->right_child];

                bool left_inside = sphere_aabb_intersection(left->bounds, center, radius * 1.6f); // malo vecji radij ker se center premika
                bool right_inside = sphere_aabb_intersection(right->bounds, center, radius * 1.6f);

                if (left_inside && right_inside)
                {
                    node = left;
                    stack.push_back(right);
                }
                else if (left_inside)
                {
                    node = left;
                }
                else if (right_inside)
                {
                    node = right;
                }
                else if (stack.size() > 0)
                {
                    node = stack.back();
                    stack.pop_back();
                }
                else
                {
                    node = nullptr;
                }
            }
        }

        static std::vector<Triangle> temp_triangles;
        temp_triangles.clear();

        for (const auto [rect, transform] : ecs::get_components<RectCollider, Transform>())
        {
            auto [t1, t2] = rect_to_tris(rect, transform);
            if (auto collision = sphere_triangle_intersection(center, radius * 1.6f, t1.v1, t1.v2, t1.v3))
            {
                temp_triangles.push_back(t1);
                close_triangles.push_back({ &temp_triangles.back(), collision->depth});
            }
            if (auto collision = sphere_triangle_intersection(center, radius * 1.6f, t2.v1, t2.v2, t2.v3))
            {
                temp_triangles.push_back(t2);
                close_triangles.push_back({ &temp_triangles.back(), collision->depth });
            }
        }

        // sortiraj blizje trikotnike tako da najprej pregledam tiste z manj penetracije
        std::sort(close_triangles.begin(), close_triangles.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

        debug_file << "sort\n";

        for (const auto& [tri, depth] : close_triangles)
        {
            auto intersection = sphere_triangle_intersection(center, radius, tri->v1, tri->v2, tri->v3);
            DEBUG_VECTOR(tri->v1);
            DEBUG_VECTOR(tri->v2);
            DEBUG_VECTOR(tri->v3);
            DEBUG_VAR(depth);
            DEBUG_VAR(intersection.has_value());
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

                DEBUG_VECTOR(center);

                if (intersection->normal.y > ground_normal_y)
                {
                    ground_collison = true;
                    DEBUG_VAR(ground_collison);
                }
                else
                {
                    debug_file << "ground_collision 0\n";
                }

                DEBUG_VECTOR(velocity);

                float vel_on_normal = glm::dot(velocity, -intersection->normal);
                velocity -= glm::max(vel_on_normal, 0.0f) * (-intersection->normal);

                DEBUG_VECTOR(intersection->normal);
                DEBUG_VAR(vel_on_normal);
                DEBUG_VECTOR(glm::max(vel_on_normal, 0.0f) * intersection->normal);
                DEBUG_VECTOR(velocity);
            }

            debug_file << "\n";
        }

        if (any)
        {
            ResolvedCollision out;
            out.new_center = center;
            out.new_velocity = velocity;
            out.ground_collision = ground_collison;
            DEBUG_VECTOR(out.new_center);
            DEBUG_VECTOR(out.new_velocity);
            DEBUG_VAR(out.ground_collision);
            return out;
        }
        debug_file << "no collision\n";
        return std::nullopt;
    }
}

