#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace kvejken
{
    enum class RightHandItem : uint8_t
    {
        None,
        Axe,
        Hammer,
        SpikedClub,
        Fireball,
        Crossbow,
    };

    enum class LeftHandItem : uint8_t
    {
        None,
        Key,
        Torch,
        Skull,
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

        LeftHandItem left_hand_item;
        RightHandItem right_hand_item;
        glm::quat left_hand_rotation, right_hand_rotation;

        float time_since_attack;
    };

    void init_weapons();

    void spawn_local_player(glm::vec3 position);

    void update_players(float delta_time, float game_time);
}
