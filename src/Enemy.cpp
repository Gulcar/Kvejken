#include "Enemy.h"
#include "ECS.h"
#include "Player.h"
#include "Components.h"
#include "Renderer.h"
#include "Model.h"
#include "Collision.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace kvejken
{
    namespace
    {
        float time_to_spawn = 0.0f;

        constexpr float ENEMY_ANIM_TIME = 0.5f;
        std::vector<Model> enemy_model_anim;

        constexpr float MOVE_SPEED = 4.0f;
        constexpr float TURN_SPEED = 8.0f;
        constexpr float RAYCAST_DIST = 4.0f;
        std::vector<glm::vec3> raycast_dirs;
    }

    void init_enemies()
    {
        for (int i = 1; i <= 12; i++)
            enemy_model_anim.emplace_back("../../assets/enemies/eel" + std::to_string(i) + ".obj");

        constexpr int NUM_POINTS_ON_SPHERE = 18;
        constexpr float MIN_POINT_Z = -0.3f;

        // https://stackoverflow.com/a/44164075
        for (int i = 0; i < NUM_POINTS_ON_SPHERE; i++)
        {
            float phi = std::acos(1.0f - 2.0f * i / (float)NUM_POINTS_ON_SPHERE);
            float theta = PI * (1.0f + std::sqrt(5.0f)) * i;

            float z = std::cos(phi);
            if (z < MIN_POINT_Z)
                continue;

            float x = std::cos(theta) * std::sin(phi);
            float y = std::sin(theta) * std::sin(phi);

            raycast_dirs.emplace_back(x, y, z);
        }
    }

    float time_btw_spawns(float game_time)
    {
        // TODO: balance
        if (game_time < 30.0f)  return 9915.0f;
        if (game_time < 60.0f)  return 10.0f;
        if (game_time < 90.0f)  return 5.0f;
        if (game_time < 120.0f)  return 3.0f;
        return 1.0f;
    }

    // TODO: dodaj vec
    constexpr glm::vec3 SPAWN_POINTS[] = {
        glm::vec3(-11, 5, -5),
        glm::vec3(12, 5, 2),
    };
    glm::vec3 closest_spawn_point(glm::vec3 position)
    {
        int nearest = 0;
        float dist = 1e30f;

        for (int i = 0; i < std::size(SPAWN_POINTS); i++)
        {
            float dist2 = glm::distance2(SPAWN_POINTS[i], position);
            if (dist2 < dist)
            {
                dist = dist2;
                nearest = i;
            }
        }

        return SPAWN_POINTS[nearest];
    }

    void spawn_enemy(glm::vec3 position, glm::vec3 rot_dir)
    {
        Enemy enemy;
        enemy.health = 100;
        enemy.animation_time = 0.0f;

        Transform transform;
        transform.position = position;
        transform.rotation = glm::quatLookAt(glm::normalize(-rot_dir), glm::vec3(0, 1, 0));
        transform.scale = 0.7f;

        Entity entity = ecs::create_entity();
        ecs::add_component(enemy, entity);
        ecs::add_component(transform, entity);
        ecs::add_component(&enemy_model_anim[0], entity);

        printf("spawn_enemy\n");
    }

    static void add_dir_to_steering_map(std::vector<float>& steering_map, glm::quat rotation, glm::vec3 direction, float strength)
    {
        direction = glm::normalize(direction);

        for (int i = 0; i < steering_map.size(); i++)
        {
            float d = glm::dot(rotation * raycast_dirs[i], direction);
            steering_map[i] += glm::max(d, 0.0f) * strength;
        }
    }

    void update_enemies(float delta_time, float game_time)
    {
        const Transform& player_transform = (*ecs::get_components<Player, Transform>().begin()).second;

        time_to_spawn -= delta_time;
        if (time_to_spawn <= 0.0f)
        {
            time_to_spawn = time_btw_spawns(game_time);

            glm::vec3 spawn_point = closest_spawn_point(player_transform.position);
            glm::vec3 direction = player_transform.position - spawn_point;
            spawn_enemy(spawn_point, direction);
        }

        for (auto& [enemy, model, transform] : ecs::get_components<Enemy, Model*, Transform>())
        {
            enemy.animation_time += delta_time;
            if (enemy.animation_time >= ENEMY_ANIM_TIME)
                enemy.animation_time -= ENEMY_ANIM_TIME;

            int anim = (int)(enemy.animation_time / ENEMY_ANIM_TIME * enemy_model_anim.size());
            model = &enemy_model_anim[anim];

            //transform.rotation = glm::quatLookAt(glm::normalize(transform.position - player_transform.position), glm::vec3(0, 1, 0));

            static std::vector<float> steering_map;
            steering_map.clear();
            steering_map.resize(raycast_dirs.size(), 0.0f);

            for (int i = 0; i < raycast_dirs.size(); i++)
            {
                auto hit = collision::raycast(transform.position, transform.rotation * raycast_dirs[i], RAYCAST_DIST);
                if (hit)
                {
                    float danger01 = (1.0f - (hit->distance / RAYCAST_DIST));
                    steering_map[i] -= 404 * std::pow(danger01, 2.5f);
                }
            }

            glm::vec3 forward = transform.rotation * glm::vec3(0, 0, 1);
            add_dir_to_steering_map(steering_map, transform.rotation, forward, 20.0f);

            add_dir_to_steering_map(steering_map, transform.rotation, player_transform.position - transform.position, 100.0f);

            glm::vec3 best_direction(0);
            for (int i = 0; i < steering_map.size(); i++)
            {
                best_direction += steering_map[i] * (transform.rotation * raycast_dirs[i]);
            }

            if (best_direction != glm::vec3(0))
                best_direction = glm::normalize(best_direction);
            else
                best_direction = -forward;

            glm::quat new_rotation = glm::quatLookAt(-best_direction, glm::vec3(0, 1, 0));
            transform.rotation = glm::slerp(transform.rotation, new_rotation, TURN_SPEED * delta_time);

            forward = transform.rotation * glm::vec3(0, 0, 1);
            transform.position += forward * MOVE_SPEED * delta_time;

            /*
            for (auto point : raycast_dirs)
            {
                point = transform.rotation * (point * 2.0f) + transform.position;
                renderer::draw_model(&enemy_model_anim[0], point, glm::vec3(0), glm::vec3(0.05f));
            }
            */
        }
    }
}
