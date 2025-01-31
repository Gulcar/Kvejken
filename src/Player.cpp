#include "Player.h"
#include "ECS.h"
#include "Components.h"
#include "Input.h"
#include "Collision.h"
#include "Assets.h"
#include "Enemy.h"
#include "Interactable.h"
#include <glm/mat4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace kvejken
{
    constexpr float MAX_MOVE_SPEED = 6.0f;
    constexpr float ACCELERATION = 80.0f;
    constexpr float DECELERATION = 35.0f;

    constexpr float MAX_MOVE_SPEED_AIR = 9.0f;
    constexpr float ACCELERATION_AIR = ACCELERATION * 0.2f;
    constexpr float DECELERATION_AIR = DECELERATION * 0.1f;

    constexpr float MAX_MOVE_SPEED_SLIDE = 16.0f;
    constexpr float DECELERATION_SLIDE = DECELERATION * 0.1f;
    constexpr float SLIDE_BOOST = 1.2f;

    constexpr float MOUSE_SENS = 1.0f / 1000.0f;

    constexpr float PLAYER_GRAVITY = -25.0f;
    constexpr float MAX_Y_VELOCITY = -PLAYER_GRAVITY * 5.0f;
    constexpr float JUMP_STRENGTH = 9.0f;
    constexpr float COYOTE_TIME = 0.12f;

    namespace
    {
        struct WeaponInfo
        {
            float range;
            float attack_time;

            Model* model;
            float model_scale;
            glm::vec3 model_offset;
        };

        struct ItemInfo
        {
            int cost;

            Model* model;
            float model_scale;
            glm::vec3 model_offset;
        };

        std::unordered_map<RightHandItem, WeaponInfo> m_weapon_infos;
        std::unordered_map<LeftHandItem, ItemInfo> m_item_infos;
    }

    void init_weapons()
    {
        WeaponInfo& axe = m_weapon_infos[RightHandItem::Axe];
        axe.range = 2.0f;
        axe.attack_time = 0.5f;
        axe.model = assets::axe.get();
        axe.model_scale = 0.3f;
        axe.model_offset = glm::vec3(0.3f, -0.5f, -0.5f);

        WeaponInfo& hammer = m_weapon_infos[RightHandItem::Hammer];
        hammer.range = 2.4f;
        hammer.attack_time = 0.6f;
        hammer.model = assets::hammer.get();
        hammer.model_scale = 0.2f;
        hammer.model_offset = glm::vec3(0.35f, -0.4f, -0.5f);

        WeaponInfo& spiked_club = m_weapon_infos[RightHandItem::SpikedClub];
        spiked_club.range = 1.66f;
        spiked_club.attack_time = 0.4f;
        spiked_club.model = assets::spiked_club.get();
        spiked_club.model_scale = 0.15f;
        spiked_club.model_offset = glm::vec3(0.3f, -0.35f, -0.5f);


        ItemInfo& key = m_item_infos[LeftHandItem::Key];
        key.cost = COST_KEY;
        key.model = assets::key.get();
        key.model_scale = 0.3f;
        key.model_offset = glm::vec3(-0.4f, -0.4f, -1.1f);
    }

    void spawn_local_player(glm::vec3 position)
    {
        Player player = {};
        player.health = 100;
        player.local = true;
        player.points = 800; // TODO: nazaj na 0

        player.right_hand_item = RightHandItem::Axe;
        player.time_since_attack = 99.0f;
        player.left_hand_item = LeftHandItem::Key;

        Camera camera = {};
        camera.position = glm::vec3(0, 0.44f, 0);
        camera.direction = glm::vec3(0, 0, -1);
        camera.up = glm::vec3(0, 1, 0);
        camera.fovy = 50.0f;
        camera.z_near = 0.05f;
        camera.z_far = 150.0f;

        Transform transform = {};
        transform.position = position;
        transform.rotation = glm::quat(1, 0, 0, 0);
        transform.scale = 1.0f;

        Entity entity = ecs::create_entity();
        ecs::add_component(player, entity);
        ecs::add_component(camera, entity);
        ecs::add_component(transform, entity);
    }

    void update_players_movement(float delta_time, float game_time)
    {
        for (auto [player, transform] : ecs::get_components<Player, Transform>())
        {
            if (!player.local)
                continue;

            glm::vec3 forward = transform.rotation * glm::vec3(0, 0, -1);
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
            glm::vec3 move_dir = {};
            move_dir += forward * (float)input::key_axis(GLFW_KEY_S, GLFW_KEY_W);
            move_dir += right * (float)input::key_axis(GLFW_KEY_A, GLFW_KEY_D);
            move_dir.y = 0.0f;

            bool grounded = (game_time <= player.jump_allowed_time);
            bool sliding = input::key_held(GLFW_KEY_LEFT_SHIFT);

            if (move_dir != glm::vec3(0, 0, 0) && !sliding)
            {
                move_dir = glm::normalize(move_dir);

                float accel = (grounded) ? ACCELERATION : ACCELERATION_AIR;
                player.move_velocity += glm::vec2(move_dir.x, move_dir.z) * accel * delta_time;
            }
            else
            {
                float decel = (grounded) ? DECELERATION : DECELERATION_AIR;
                decel = (sliding) ? DECELERATION_SLIDE : decel;
                if (glm::length(player.move_velocity) > decel * delta_time)
                    player.move_velocity -= glm::normalize(player.move_velocity) * decel * delta_time;
                else
                    player.move_velocity = glm::vec2(0, 0);
            }

            float max_velocity = (grounded) ? MAX_MOVE_SPEED : MAX_MOVE_SPEED_AIR;
            max_velocity = (sliding) ? MAX_MOVE_SPEED_SLIDE : max_velocity;
            float move_velocity = glm::length(player.move_velocity);
            if (move_velocity > max_velocity)
            {
                player.move_velocity = player.move_velocity / move_velocity * max_velocity;
            }

            transform.position += glm::vec3(player.move_velocity.x, 0.0f, player.move_velocity.y) * delta_time;

            player.velocity_y += PLAYER_GRAVITY * delta_time;

            player.velocity_y = glm::clamp(player.velocity_y, -MAX_Y_VELOCITY, MAX_Y_VELOCITY);
            transform.position.y += player.velocity_y * delta_time;

            glm::vec3 velocity = glm::vec3(player.move_velocity.x, player.velocity_y, player.move_velocity.y);
            auto res = collision::sphere_collision(transform.position, 0.5f, velocity, 35.0f, 0.01f);
            if (res)
            {
                transform.position = res->new_center;

                player.velocity_y = res->new_velocity.y;
                player.move_velocity.x = res->new_velocity.x;
                player.move_velocity.y = res->new_velocity.z;

                if (res->ground_collision)
                {
                    if (player.velocity_y < 0.0f)
                        player.velocity_y = 0.0f;

                    player.jump_allowed_time = game_time + COYOTE_TIME;
                }
            }

            if (transform.position.y < -150.0f)
                transform.position.y = 150.0f;
        }
    }

    static void attack(glm::vec3 attack_position, float attack_radius, int* out_points)
    {
        Entity closest = -1;
        float closest_dist = attack_radius * attack_radius;

        for (auto [id, enemy, transform] : ecs::get_components_ids<Enemy, Transform>())
        {
            float dist2 = glm::distance2(attack_position, transform.position);
            if (dist2 < closest_dist)
            {
                closest_dist = dist2;
                closest = id;
            }
        }
        // TODO: particles

        if (closest != (Entity)-1)
        {
            ecs::queue_destroy_entity(closest);
            *out_points += 10;
        }
    }

    void update_players(float delta_time, float game_time)
    {
        int substeps = (int)(delta_time / 0.010f) + 1;
        float sub_delta = delta_time / substeps;
        for (int i = 0; i < substeps; i++)
            update_players_movement(sub_delta, game_time);

        for (auto [player, camera, transform] : ecs::get_components<Player, Camera, Transform>())
        {
            if (!player.local)
                continue;

            if (input::is_mouse_locked())
            {
                glm::vec2 mouse_delta = input::mouse_delta();
                player.look_yaw -= mouse_delta.x * MOUSE_SENS;
                player.look_pitch -= mouse_delta.y * MOUSE_SENS;
                player.look_pitch = glm::clamp(player.look_pitch, -PI / 2.0f + 0.01f, PI / 2.0f - 0.01f);
                transform.rotation = glm::quat(glm::vec3(player.look_pitch, player.look_yaw, 0.0f));
            }

            if (input::key_pressed(GLFW_KEY_SPACE) && game_time <= player.jump_allowed_time)
            {
                player.velocity_y += JUMP_STRENGTH;
                player.move_velocity = player.move_velocity * MAX_MOVE_SPEED_AIR / MAX_MOVE_SPEED;
                player.jump_allowed_time = -1.0f;
            }

            if (input::key_held(GLFW_KEY_LEFT_SHIFT))
            {
                camera.position.y = 0.29f;
            }
            else
            {
                camera.position.y = 0.44f;
            }

            if (input::key_pressed(GLFW_KEY_LEFT_SHIFT))
            {
                player.move_velocity *= SLIDE_BOOST;
            }

            // TODO: remove to je samo za debug
            if (input::key_pressed(GLFW_KEY_1)) player.right_hand_item = RightHandItem::Axe;
            if (input::key_pressed(GLFW_KEY_2)) player.right_hand_item = RightHandItem::Hammer;
            if (input::key_pressed(GLFW_KEY_3)) player.right_hand_item = RightHandItem::SpikedClub;

            player.right_hand_rotation = glm::slerp(player.right_hand_rotation, transform.rotation, 30.0f * delta_time);
            player.left_hand_rotation = glm::slerp(player.left_hand_rotation, transform.rotation, 40.0f * delta_time);

            if (player.right_hand_item != RightHandItem::None)
            {
                const WeaponInfo& weapon = m_weapon_infos[player.right_hand_item];

                player.time_since_attack += delta_time;
                if (input::mouse_pressed(GLFW_MOUSE_BUTTON_LEFT) && player.time_since_attack > weapon.attack_time)
                {
                    player.time_since_attack = 0.0f;
                    glm::vec3 attack_center = transform.position + camera.position + transform.rotation * glm::vec3(0, 0, -1);
                    attack(attack_center, weapon.range, &player.points);
                }

                glm::vec3 hand_position = transform.position + camera.position;
                hand_position.y += glm::length(player.move_velocity) * std::sin(game_time * 7.0f) / 300.0f;

                glm::vec3 weapon_offset = weapon.model_offset;
                glm::mat4 weapon_rotation(1.0f);
                glm::vec3 weapon_pivot = glm::vec3(0, -1.375f, 0);
                if (player.time_since_attack < weapon.attack_time)
                {
                    float t = player.time_since_attack / weapon.attack_time;
                    t = 1 - std::pow(1 - t, 3.0f);
                    weapon_offset += glm::mix(glm::vec3(0, 0.8f, 0.3f), glm::vec3(0, 0.4f, 0.7f), t);

                    glm::quat rot1 = glm::angleAxis(glm::radians(-50.0f), glm::vec3(0, 0, 1));
                    glm::quat rot2 = glm::angleAxis(glm::radians(140.0f), glm::vec3(0, 1, 0))
                        * glm::angleAxis(glm::radians(-50.0f), glm::vec3(0, 0, 1));
                    weapon_rotation = glm::toMat4(glm::slerp(rot1, rot2, t));
                }
                else if (player.time_since_attack < weapon.attack_time + 0.2f)
                {
                    float t = (player.time_since_attack - weapon.attack_time) / 0.2f;
                    t = 1 - std::pow(1 - t, 3.0f);
                    weapon_offset += glm::mix(glm::vec3(0, 0.4f, 0.7f), glm::vec3(0, 0, 0), t);

                    glm::quat rot1 = glm::angleAxis(glm::radians(140.0f), glm::vec3(0, 1, 0))
                        * glm::angleAxis(glm::radians(-50.0f), glm::vec3(0, 0, 1));
                    glm::quat rot2 = glm::quat(1, 0, 0, 0);
                    weapon_rotation = glm::toMat4(glm::slerp(rot1, rot2, t));
                }

                glm::mat4 t = glm::translate(glm::mat4(1.0f), hand_position)
                    * glm::toMat4(player.right_hand_rotation)
                    * glm::translate(glm::mat4(1.0f), weapon_offset + weapon_pivot)
                    * weapon_rotation
                    * glm::translate(glm::mat4(1.0f), -weapon_pivot)
                    * glm::scale(glm::mat4(1.0f), glm::vec3(weapon.model_scale));

                renderer::draw_model(weapon.model, t, Layer_FirstPerson);
            }

            if (player.left_hand_item != LeftHandItem::None)
            {
                ItemInfo& item = m_item_infos[player.left_hand_item];

                glm::mat4 t = glm::translate(glm::mat4(1.0f), transform.position + camera.position)
                    * glm::toMat4(player.left_hand_rotation)
                    * glm::translate(glm::mat4(1.0f), item.model_offset)
                    * glm::scale(glm::mat4(1.0f), glm::vec3(item.model_scale));

                renderer::draw_model(item.model, t, Layer_FirstPerson);
            }

            Interactable* closest_interactable = nullptr;
            float closest_dist = 1e30f;
            for (auto [interactable, interactable_transform] : ecs::get_components<Interactable, Transform>())
            {
                float dist = glm::distance2(interactable_transform.position, transform.position);
                if (dist < closest_dist && dist < interactable.max_player_dist)
                {
                    closest_interactable = &interactable;
                    closest_dist = dist;
                }
            }
            if (closest_interactable != nullptr)
            {
                closest_interactable->player_close = true;

                if (input::key_pressed(GLFW_KEY_E))
                {
                    if (closest_interactable->cost >= 0 && player.points >= closest_interactable->cost)
                    {
                        closest_interactable->player_interacted = true;
                        player.points -= closest_interactable->cost;
                    }
                    else if (player.left_hand_item != LeftHandItem::None && m_item_infos[player.left_hand_item].cost == closest_interactable->cost)
                    {
                        closest_interactable->player_interacted = true;
                        player.left_hand_item = LeftHandItem::None;
                    }
                }
            }
        }
    }
}
