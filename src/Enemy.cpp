#include "Enemy.h"
#include "ECS.h"
#include "Player.h"
#include "Components.h"
#include "Renderer.h"
#include "Model.h"
#include "Collision.h"
#include "Assets.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace kvejken
{
    namespace
    {
        float m_time_to_spawn = time_btw_spawns(0.0f);

        constexpr float ENEMY_ANIM_TIME = 0.5f;

        constexpr float MOVE_SPEED = 4.0f;
        constexpr float TURN_SPEED = 8.0f;
        constexpr float RAYCAST_DIST = 4.0f;
        std::vector<glm::vec3> m_raycast_dirs;
    }

    void init_enemies()
    {
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

            m_raycast_dirs.emplace_back(x, y, z);
        }
    }

    // TODO: lahko bi namesto (ali pa ce bi oboje gledal) casa gledal
    // koliko sob je ze odprl oz. kako dalec je v igri
    float time_btw_spawns(float game_time)
    {
        // cas pada zaradi e na -x in valovi zaradi sinusa
        return 15.0f * std::exp(-game_time / 400.0f) + 5.0f * std::sin(game_time / 10.0f) + 5.0f;
    }

    // TODO: dodaj vec
    // TODO: mislim da nekatere spawna v tla
    constexpr glm::vec3 SPAWN_POINTS[] = {
        glm::vec3(-34.6f, 2.5f, 1.3f),
        glm::vec3(-32.6f, 3.2f, -9.0f),
        glm::vec3(-24.3f, 3.4f, -16.9f),
        glm::vec3(-8.7f, 1.7f, -27.3f),
        glm::vec3(8.5f, 2.3f, -22.3f),
        glm::vec3(17.0f, 2.1f, -19.5f),
        glm::vec3(26.9f, 1.6f, -10.8f),
        glm::vec3(29.7f, 1.9f, -1.6f),
        glm::vec3(24.7f, 3.14f, 12.9f),
        glm::vec3(15.6f, 3.98f, 24.5f),
        glm::vec3(5.1f, 4.1f, 30.3f),
        glm::vec3(-12.5f, 3.68f, 25.7f),
        glm::vec3(-29.1f, 2.68f, 21.0f),
    };
    static glm::vec3 get_spawn_point(glm::vec3 player_position, glm::vec3 player_direction)
    {
        int nearest = 0;
        float dist = 1e30f;

        for (int i = 0; i < std::size(SPAWN_POINTS); i++)
        {
            float dist2 = glm::distance2(SPAWN_POINTS[i], player_position);

            if (dist2 < dist && dist2 > 5.0f * 5.0f &&
                glm::dot(glm::normalize(SPAWN_POINTS[i] - player_position), player_direction) < 0.1f)
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
        transform.scale = 0.57f;

        Entity entity = ecs::create_entity();
        ecs::add_component(enemy, entity);
        ecs::add_component(transform, entity);
        ecs::add_component(&assets::eel_anim[0], entity);
        printf("spawn enemy id %u\n", entity);
    }

    static void add_dir_to_steering_map(std::vector<float>& steering_map, glm::quat rotation, glm::vec3 direction, float strength)
    {
        direction = glm::normalize(direction);

        for (int i = 0; i < steering_map.size(); i++)
        {
            float d = glm::dot(rotation * m_raycast_dirs[i], direction);
            steering_map[i] += glm::max(d, 0.0f) * strength;
        }
    }

    void update_enemies(float delta_time, float game_time)
    {
        const Transform& player_transform = (*ecs::get_components<Player, Transform>().begin()).second;

        m_time_to_spawn -= delta_time;
        if (m_time_to_spawn <= 0.0f)
        {
            m_time_to_spawn = time_btw_spawns(game_time);

            glm::vec3 spawn_point = get_spawn_point(player_transform.position, player_transform.rotation * glm::vec3(0, 0, -1));
            glm::vec3 direction = player_transform.position - spawn_point;
            spawn_enemy(spawn_point, direction);
        }

        for (auto spawn : SPAWN_POINTS)
            renderer::draw_model(assets::spawn.get(), spawn, glm::angleAxis(game_time, glm::vec3(0, 1, 0)), glm::vec3(0.8f));

        for (auto& [enemy, model, transform] : ecs::get_components<Enemy, Model*, Transform>())
        {
            enemy.animation_time += delta_time * utils::randf(0.9f, 1.1f);
            if (enemy.animation_time >= ENEMY_ANIM_TIME)
                enemy.animation_time -= ENEMY_ANIM_TIME;

            int anim = (int)(enemy.animation_time / ENEMY_ANIM_TIME * assets::eel_anim.size());
            model = &assets::eel_anim[anim];

            //transform.rotation = glm::quatLookAt(glm::normalize(transform.position - player_transform.position), glm::vec3(0, 1, 0));

            static std::vector<float> steering_map;
            steering_map.clear();
            steering_map.resize(m_raycast_dirs.size(), 0.0f);

            for (int i = 0; i < m_raycast_dirs.size(); i++)
            {
                auto hit = collision::raycast(transform.position, transform.rotation * m_raycast_dirs[i], RAYCAST_DIST);
                if (hit)
                {
                    float danger01 = (1.0f - (hit->distance / RAYCAST_DIST));
                    steering_map[i] -= 280 * std::pow(danger01, 2.5f);
                }
            }

            glm::vec3 forward = transform.rotation * glm::vec3(0, 0, 1);
            add_dir_to_steering_map(steering_map, transform.rotation, forward, 10.0f);

            float player_dist = glm::distance(player_transform.position, transform.position);
            float player_follow_strength = 50.0f * (1.0f - glm::smoothstep(2.0f, 15.0f, player_dist)) + 50.0f;
            add_dir_to_steering_map(steering_map, transform.rotation, player_transform.position - transform.position, player_follow_strength);

            for (auto& [enemy2, transform2] : ecs::get_components<Enemy, Transform>())
            {
                if (transform.position != transform2.position)
                {
                    float dist2 = glm::distance2(transform.position, transform2.position);
                    if (dist2 < 6.0f * 6.0f)
                    {
                        float danger01 = (1.0f - (std::sqrt(dist2) / 6.0f));
                        add_dir_to_steering_map(steering_map, transform.rotation, transform2.position - transform.position, -80 * danger01 * danger01);
                    }
                }
            }

            glm::vec3 best_direction(0);
            for (int i = 0; i < steering_map.size(); i++)
            {
                best_direction += steering_map[i] * (transform.rotation * m_raycast_dirs[i]);
            }

            if (best_direction != glm::vec3(0))
                best_direction = glm::normalize(best_direction);
            else
                best_direction = -forward;

            glm::quat new_rotation = glm::quatLookAt(-best_direction, glm::vec3(0, 1, 0));
            transform.rotation = glm::slerp(transform.rotation, new_rotation, TURN_SPEED * delta_time);

            forward = transform.rotation * glm::vec3(0, 0, 1);
            float speed_mult = 1.0f;

            auto hit = collision::raycast(transform.position, forward, 2.0f);
            if (hit)
            {
                if (hit->distance < 1.1f)
                    speed_mult = 0.1f;
                else
                    speed_mult = hit->distance - 1.0f;
            }

            transform.position += forward * MOVE_SPEED * speed_mult * delta_time;

            /*
            for (auto point : m_raycast_dirs)
            {
                point = transform.rotation * (point * 2.0f) + transform.position;
                renderer::draw_model(&m_model_anim[0], point, glm::vec3(0), glm::vec3(0.05f));
            }
            */
        }
    }
}
