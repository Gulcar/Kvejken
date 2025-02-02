#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Interactable.h"

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

        LeftHandItem left_hand_item;
        RightHandItem right_hand_item;
        glm::quat left_hand_rotation, right_hand_rotation;

        float time_since_attack;

        int points;
    };

    void spawn_local_player(glm::vec3 position);

    void update_players(float delta_time, float game_time);
}
