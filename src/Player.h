#pragma once

#include <glm/vec2.hpp>

namespace kvejken
{
    struct Player
    {
        int health;
        bool local;

        glm::vec2 move_velocity;
        float velocity_y;
        float jump_allowed_time;

        float look_yaw;
        float look_pitch;
    };

    void update_players(float delta_time, float game_time);
}
