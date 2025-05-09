﻿#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Interactable.h"

namespace kvejken
{
    enum Objective
    {
        None,
        PickUpWeapon,
        EnterCastle,
        EnterBasement,
        FindTorch,
        LightTorch,
        FindSkull,
        SkullOnThrone,
        Done,
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

        float forced_movement_time;
        glm::vec3 forced_pos;
        glm::vec3 forced_look_at;

        float screen_shake;

        ItemType left_hand_item;
        WeaponType right_hand_item;
        glm::quat left_hand_rotation, right_hand_rotation;
        float left_hand_time_since_pickup;
        float right_hand_time_since_pickup;

        float time_since_attack;
        bool attack_hit;
        bool attack_miss;
        bool attack_from_left;
        int attack_combo;

        float time_since_recv_damage;
        int death_message;

        int points;
        int progress; // za hitrost enemy spawnov
        Objective curr_objective;
    };

    void spawn_local_player(glm::vec3 position);
    void reset_player();

    void update_players(float delta_time, float game_time);

    void damage_player(Player& player, int damage, glm::vec3 attack_pos);

    void add_screen_shake(float amount);
    void objective_complete(Objective completed_objective);
}

