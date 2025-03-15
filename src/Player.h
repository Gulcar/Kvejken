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

        ItemType left_hand_item;
        WeaponType right_hand_item;
        glm::quat left_hand_rotation, right_hand_rotation;

        float time_since_attack;
        float time_since_recv_damage;
        int death_message;

        int points;
        int progress; // za hitrost enemy spawnov
    };

    void spawn_local_player(glm::vec3 position);

    void update_players(float delta_time, float game_time);

    void damage_player(Player& player, int damage, glm::vec3 attack_pos);
}
