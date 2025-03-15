#pragma once

#include <glm/vec3.hpp>

namespace kvejken
{
    struct Enemy
    {
        int health;
        float animation_time;
    };

    void init_enemies();

    float time_btw_spawns(float game_time, int player_progress);
    void spawn_enemy(glm::vec3 position, glm::vec3 rot_dir);

    void update_enemies(float delta_time, float game_time);

    void draw_enemy_spawns(float game_time);
}
