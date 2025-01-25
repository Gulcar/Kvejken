#include "Enemy.h"
#include "ECS.h"
#include "Player.h"
#include "Components.h"
#include "Renderer.h"
#include "Model.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace kvejken
{
    namespace
    {
        float time_to_spawn = 10.0f;
        std::vector<Model> enemy_model_anim;
    }

    void init_enemies()
    {
        for (int i = 1; i <= 12; i++)
            enemy_model_anim.emplace_back("../../assets/enemies/eel" + std::to_string(i) + ".obj");
    }

    float time_btw_spawns(float game_time)
    {
        // TODO: balance
        if (game_time < 30.0f)  return 10.0f;
        if (game_time < 60.0f)  return 5.0f;
        if (game_time < 90.0f)  return 3.0f;
        if (game_time < 120.0f)  return 2.0f;
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
        transform.rotation = utils::dir_to_quat(rot_dir);
        transform.scale = 1.0f;

        Entity entity = ecs::create_entity();
        ecs::add_component(enemy, entity);
        ecs::add_component(transform, entity);
        ecs::add_component(&enemy_model_anim[0], entity);

        printf("spawn_enemy\n");
    }

    void update_enemies(float delta_time, float game_time)
    {
        time_to_spawn -= delta_time;
        if (time_to_spawn <= 0.0f)
        {
            time_to_spawn = time_btw_spawns(game_time);

            Transform& player_transform = (*ecs::get_components<Player, Transform>().begin()).second;

            glm::vec3 spawn_point = closest_spawn_point(player_transform.position);
            glm::vec3 direction = player_transform.position - spawn_point;
            spawn_enemy(spawn_point, direction);
        }
    }
}
