#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace kvejken
{
    enum class ItemType
    {
        None,
        Axe,
        Hammer,
        SpikedClub,
        Fireball,
        Crossbow,
    };

    struct Player
    {
        int health;
        bool local;

        glm::vec2 move_velocity;
        float velocity_y;
        float jump_allowed_time;

        float look_yaw;
        float look_pitch;

        ItemType left_hand_item, right_hand_item;
        glm::quat left_hand_rotation, right_hand_rotation;
    };

    void spawn_local_player(glm::vec3 position);

    void update_players(float delta_time, float game_time);
}
