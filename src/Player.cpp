#include "Player.h"
#include "ECS.h"
#include "Components.h"
#include "Input.h"
#include "Collision.h"
#include "Assets.h"
#include "Enemy.h"
#include "Interactable.h"
#include "Particles.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/noise.hpp>
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

    constexpr float MOUSE_SENS = 4.0f / 10000.0f;

    constexpr float PLAYER_GRAVITY = -25.0f;
    constexpr float MAX_Y_VELOCITY = -PLAYER_GRAVITY * 5.0f;
    constexpr float JUMP_STRENGTH = 9.0f;
    constexpr float COYOTE_TIME = 0.12f;

    const char* death_texts[] = {
        u8"Urml si!",
        u8"Neeeeeee!",
        u8"Izgubil si!",
        u8"Avč! To pa boli!",
        u8"AAAAAAhhhhhh!",
        u8"Zakaj sem moral umreti?",
        u8"Pa tako dobro je šlo!",
        u8"Kje se je vse zalomilo?",
        u8"Nič več življenja",
        u8"Slab si!",
    };

    void spawn_local_player(glm::vec3 position)
    {
        Player player = {};
        player.health = 100;
        player.local = true;
        player.points = 2000; // TODO: nazaj na 0
        player.right_hand_item = WeaponType::None;
        player.left_hand_item = ItemType::None;
        player.time_since_attack = 99.0f;
        player.time_since_recv_damage = 99.0f;
        player.progress = 0;

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

        PointLight light = {}; // ostale lastnosti so posodobljene vsak frame
        light.offset = camera.position;

        ParticleSpawner particles = {};
        particles.active = true;
        particles.spawn_rate = 100.0f;
        particles.min_size = 0.02f;
        particles.max_size = 0.03f;
        particles.min_time_alive = 0.3f;
        particles.max_time_alive = 0.45f;
        particles.origin = glm::vec3(0, 6, 0);
        particles.origin_radius = 0.1f;
        particles.min_velocity = 0.5f;
        particles.max_velocity = 0.75f;
        particles.velocity_offset = glm::vec3(0, 1, 0);
        particles.color_a = glm::vec3(1, 0.01f, 0);
        particles.color_b = glm::vec3(1, 0.03f, 0);
        particles.draw_layer = Layer::FirstPerson;

        Entity entity = ecs::create_entity();
        ecs::add_component(player, entity);
        ecs::add_component(camera, entity);
        ecs::add_component(transform, entity);
        ecs::add_component(light, entity);
        ecs::add_component(particles, entity);
    }

    void update_players_movement(float delta_time, float game_time)
    {
        for (auto [player, transform] : ecs::get_components<Player, Transform>())
        {
            if (!player.local || player.health <= 0)
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

    static void attack(Player& player, glm::vec3 blade_pos, float attack_radius, glm::vec3 camera_pos)
    {
        Entity closest = -1;
        float closest_dist = (attack_radius + 1.25f) * (attack_radius + 1.25f);
        glm::vec3 enemy_pos;

        for (auto [id, enemy, transform] : ecs::get_components_ids<Enemy, Transform>())
        {
            float dist2 = glm::distance2(blade_pos, transform.position);
            if (dist2 < closest_dist)
            {
                closest_dist = dist2;
                closest = id;
                enemy_pos = transform.position;
            }
        }

        // hit
        if (closest != (Entity)-1)
        {
            ParticleExplosionParameters particle_params;
            particle_params.min_count = 10;
            particle_params.max_count = 20;
            particle_params.min_size = 0.1f;
            particle_params.max_size = 0.2f;
            particle_params.min_time_alive = 0.2f;
            particle_params.max_time_alive = 0.4f;
            particle_params.origin = enemy_pos;
            particle_params.min_velocity = 15.0f;
            particle_params.max_velocity = 20.2f;
            particle_params.velocity_offset = glm::vec3(0.0f);
            particle_params.color_a = glm::vec3(0.6f, 0.0f, 0.0f);
            particle_params.color_b = glm::vec3(0.0f, 0.0f, 0.0f);
            particle_params.draw_layer = Layer::World;
            spawn_particle_explosion(particle_params);

            ecs::queue_destroy_entity(closest);
            player.points += 10;

            player.attack_hit = true;
            player.screen_shake += 0.3f;
        }
        // miss
        else if (auto environment_hit = collision::raycast(camera_pos, glm::normalize(blade_pos - camera_pos),
            glm::length(blade_pos - camera_pos) + attack_radius * 0.7f))
        {
            ParticleExplosionParameters particle_params;
            particle_params.min_count = 5;
            particle_params.max_count = 10;
            particle_params.min_size = 0.025f;
            particle_params.max_size = 0.050f;
            particle_params.min_time_alive = 0.2f;
            particle_params.max_time_alive = 0.4f;
            particle_params.origin = environment_hit->position;
            particle_params.min_velocity = 5.0f;
            particle_params.max_velocity = 10.0f;
            particle_params.velocity_offset = glm::vec3(0.0f);
            particle_params.color_a = glm::vec3(0.2f, 0.2f, 0.2f);
            particle_params.color_b = glm::vec3(0.0f, 0.0f, 0.0f);
            particle_params.draw_layer = Layer::World;
            spawn_particle_explosion(particle_params);

            player.attack_miss = true;
            player.screen_shake += 0.2f;
        }
    }

    static void update_player_weapon(Player& player, Transform& transform, Camera& camera, float delta_time, float game_time)
    {
        const WeaponInfo& weapon = get_weapon_info(player.right_hand_item);

        player.time_since_attack += delta_time;

        float weapon_return_time = (player.attack_hit || player.attack_miss) ? 0.7f : 2.0f;
        float attack_timeout = weapon.attack_time + weapon_return_time * 0.6f;

        // TODO:
        // - boljse animacije in napad iz leve
        // - enemy health

        bool can_combo = player.attack_hit && player.time_since_attack > weapon.attack_time
            && player.time_since_attack < weapon.attack_time + 0.2f && player.attack_combo < 3;
        bool can_combo_left = !player.attack_from_left && can_combo;
        bool can_combo_right = player.attack_from_left && can_combo;

        if (input::mouse_pressed(GLFW_MOUSE_BUTTON_LEFT) && (player.time_since_attack > attack_timeout || can_combo_right))
        {
            player.time_since_attack = 0.0f;
            player.attack_hit = false;
            player.attack_miss = false;

            player.attack_from_left = false;
            if (can_combo_right) player.attack_combo += 1;
            else player.attack_combo = 1;
        }
        else if (input::mouse_pressed(GLFW_MOUSE_BUTTON_RIGHT) && can_combo_left)
        {
            player.time_since_attack = 0.0f;
            player.attack_hit = false;
            player.attack_miss = false;

            player.attack_from_left = true;
            player.attack_combo += 1;
        }

        float left_mult = (player.attack_from_left && player.time_since_attack < weapon.attack_time + weapon_return_time) ? -1.0f : 1.0f;

        glm::vec3 hand_position = transform.position + camera.position;
        hand_position.y += glm::length(player.move_velocity) * std::sin(game_time * 7.0f) / 300.0f;

        glm::vec3 weapon_offset = glm::vec3(0.3f * left_mult, -0.5f, -0.5f);
        glm::mat4 weapon_rotation(1.0f);
        glm::vec3 weapon_pivot = glm::vec3(0, -1.375f, 0);
        if (player.time_since_attack < weapon.attack_time)
        {
            float t = player.time_since_attack / weapon.attack_time;
            t = 1 - std::pow(1 - t, 3.0f);
            weapon_offset += glm::mix(glm::vec3(0, 0.8f, 0.3f), glm::vec3(0, 0.4f, 0.7f), t);

            glm::quat rot1 = glm::angleAxis(glm::radians(-50.0f * left_mult), glm::vec3(0, 0, 1));
            glm::quat rot2 = glm::angleAxis(glm::radians(150.0f * left_mult), glm::vec3(0, 1, 0))
                * glm::angleAxis(glm::radians(-50.0f * left_mult), glm::vec3(0, 0, 1));
            weapon_rotation = glm::toMat4(glm::slerp(rot1, rot2, t));
        }
        else if (player.time_since_attack < weapon.attack_time + weapon_return_time)
        {
            float t = (player.time_since_attack - weapon.attack_time) / weapon_return_time;
            t = 1 - std::pow(1 - t, 3.0f);

            glm::vec3 return_offset = player.attack_from_left ? glm::vec3(0.6f, 0, 0) : glm::vec3(0, 0, 0);
            weapon_offset += glm::mix(glm::vec3(0, 0.4f, 0.7f), return_offset, t);

            glm::quat rot1 = glm::angleAxis(glm::radians(150.0f * left_mult), glm::vec3(0, 1, 0))
                * glm::angleAxis(glm::radians(-50.0f * left_mult), glm::vec3(0, 0, 1));
            glm::quat rot2 = player.attack_from_left ? glm::quat(0, 0, 1, 0) : glm::quat(1, 0, 0, 0);
            weapon_rotation = glm::toMat4(glm::slerp(rot1, rot2, t));
        }

        glm::mat4 t = glm::translate(glm::mat4(1.0f), hand_position)
            * glm::toMat4(player.right_hand_rotation)
            * glm::translate(glm::mat4(1.0f), weapon_offset + weapon_pivot)
            * weapon_rotation
            * glm::translate(glm::mat4(1.0f), weapon.model_offset - weapon_pivot)
            * glm::scale(glm::mat4(1.0f), glm::vec3(weapon.model_scale / 0.5f * 0.3f));

        renderer::draw_model(weapon.model, t, Layer::FirstPerson);

        if (player.time_since_attack > weapon.attack_time * 0.21f && player.time_since_attack < weapon.attack_time * 0.45f
            && !player.attack_hit && !player.attack_miss)
        {
            glm::vec3 blade_pos = glm::vec3(t * glm::vec4(0, 1.6f, 0, 1));
            attack(player, blade_pos, weapon.range, transform.position + camera.position);

            //renderer::draw_model(assets::test_cube.get(), blade_pos, glm::vec3(0), glm::vec3(weapon.range * 2.0f));
        }
    }

    void update_players(float delta_time, float game_time)
    {
        int substeps = (int)(delta_time / 0.010f) + 1;
        float sub_delta = delta_time / substeps;
        for (int i = 0; i < substeps; i++)
            update_players_movement(sub_delta, game_time);

        for (auto [player, player_transform] : ecs::get_components<Player, Transform>())
        {
            if (!player.local || player.health <= 0)
                continue;

            for (auto [enemy_id, enemy, enemy_transform] : ecs::get_components_ids<Enemy, Transform>())
            {
                if (glm::distance2(player_transform.position, enemy_transform.position) < 1.5f)
                {
                    damage_player(player, utils::rand(15, 30), enemy_transform.position);
                    if (player.health > 0)
                        ecs::queue_destroy_entity(enemy_id);
                }
            }
        }

        for (auto [id, player, camera, transform] : ecs::get_components_ids<Player, Camera, Transform>())
        {
            if (!player.local || player.health <= 0)
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
                camera.position = glm::vec3(0, 0.29f, 0);
            }
            else
            {
                camera.position = glm::vec3(0, 0.44f, 0);
            }

            if (player.screen_shake > 0.0f)
            {
                float magnitude = player.screen_shake / 20.0f * (1.0f + glm::length(glm::vec3(player.move_velocity, player.velocity_y)) / 3.0f);
                camera.position.x += glm::simplex(glm::vec2(10.0f, game_time * 20.0f)) * magnitude;
                camera.position.y += glm::simplex(glm::vec2(20.0f, game_time * 20.0f)) * magnitude;
                camera.position.z += glm::simplex(glm::vec2(30.0f, game_time * 20.0f)) * magnitude;

                player.screen_shake = std::max(player.screen_shake - delta_time, 0.0f);
            }

            if (input::key_pressed(GLFW_KEY_LEFT_SHIFT))
            {
                player.move_velocity *= SLIDE_BOOST;
            }

            player.right_hand_rotation = glm::slerp(player.right_hand_rotation, transform.rotation, 30.0f * delta_time);
            player.left_hand_rotation = glm::slerp(player.left_hand_rotation, transform.rotation, 40.0f * delta_time);

            if (player.right_hand_item != WeaponType::None)
            {
                update_player_weapon(player, transform, camera, delta_time, game_time);
            }

            if (player.left_hand_item != ItemType::None)
            {
                const ItemInfo& item = get_item_info(player.left_hand_item);

                glm::vec3 hand_position = transform.position + camera.position;
                hand_position.y -= glm::length(player.move_velocity) * std::sin(game_time * 7.0f) / 600.0f;

                glm::mat4 t = glm::translate(glm::mat4(1.0f), hand_position)
                    * glm::toMat4(player.left_hand_rotation)
                    * glm::translate(glm::mat4(1.0f), item.model_offset)
                    * glm::scale(glm::mat4(1.0f), glm::vec3(item.model_scale));

                renderer::draw_model(item.model, t, Layer::FirstPerson);

                ParticleSpawner& particle_spawner = ecs::get_component<ParticleSpawner>(id);
                PointLight& point_light = ecs::get_component<PointLight>(id);
                if (player.left_hand_item == ItemType::Torch)
                {
                    glm::vec3 torch_fire_pos = glm::vec3(t * glm::vec4(0.0f, 1.4f, 0.0f, 1.0f));
                    particle_spawner.active = true;
                    particle_spawner.origin = torch_fire_pos - transform.position;

                    glm::vec3 ray_origin = transform.position + camera.position;
                    glm::vec3 ray_dir = glm::normalize(torch_fire_pos - ray_origin);
                    point_light.offset = ray_dir * 0.4f + ray_origin - transform.position;
                }
                else
                {
                    particle_spawner.active = false;
                }
            }

            Interactable* closest_interactable = nullptr;
            float closest_dist = 1e30f;
            for (auto [interactable, interactable_transform] : ecs::get_components<Interactable, Transform>())
            {
                float dist = glm::distance2(interactable_transform.position, transform.position);
                float max_dist = interactable.max_player_dist * interactable.max_player_dist;
                if (dist < closest_dist && dist < max_dist)
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
                    else if (player.left_hand_item != ItemType::None && get_item_info(player.left_hand_item).cost == closest_interactable->cost)
                    {
                        closest_interactable->player_interacted = true;
                        player.left_hand_item = ItemType::None;
                    }
                }
            }

            std::string points = u8"točke: " + std::to_string(player.points);
            std::string health = "zdravje: " + std::to_string(player.health);
            renderer::draw_text(points.c_str(), glm::vec2(16, 48), 48);
            renderer::draw_text(health.c_str(), glm::vec2(16, 96), 48);

            if (player.time_since_recv_damage < 1.5f)
            {
                float opacity = 0.4f * (1.5f - player.time_since_recv_damage) * (1.5f - player.time_since_recv_damage) / (1.5f * 1.5f);
                renderer::draw_rect(glm::vec2(1920 / 2, 1080 / 2), glm::vec2(1920, 1080) * 10.0f, glm::vec4(1.0f, 0.0f, 0.0f, opacity));
                player.time_since_recv_damage += delta_time;
            }
        }

        for (auto [player, light, transform] : ecs::get_components<Player, PointLight, Transform>())
        {
            if (!player.local || player.health <= 0)
                continue;

            float sun = glm::smoothstep(1.5f, 2.7f, transform.position.y);
            if (transform.position.z < 50.0f)
                sun = 1.0f;
            renderer::set_sun_light(sun);

            if (player.left_hand_item == ItemType::Torch)
            {
                light.color = glm::vec3(1.0f, 0.6f, 0.4f);
                light.strength = 12.0f;
            }
            else
            {
                light.color = glm::vec3(1, 1, 1);
                light.strength = (1.0f - sun) * 1.4f;
                light.offset = glm::vec3(0, 0, 0);
            }
        }

        for (auto& [player, particle_spawner] : ecs::get_components<Player, ParticleSpawner>())
        {
            if (player.local && player.health <= 0)
            {
                renderer::draw_rect(glm::vec2(1920 / 2, 1080 / 2), glm::vec2(1920, 1080) * 10.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.3f));

                renderer::draw_text(death_texts[player.death_message], glm::vec2(1920 / 2, 1080 / 2), 128, glm::vec4(1.0f), Align::Center);

                particle_spawner.active = false;
            }
        }
    }

    void damage_player(Player& player, int damage, glm::vec3 attack_pos)
    {
        player.health -= damage;

        if (player.health <= 0)
        {
            player.health = 0;

            player.death_message = utils::rand(0, std::size(death_texts) - 1);
        }

        player.time_since_recv_damage = 0.0f;
        player.screen_shake += 0.65f;
        
        ParticleExplosionParameters particle_params;
        particle_params.min_count = 10;
        particle_params.max_count = 20;
        particle_params.min_size = 0.1f;
        particle_params.max_size = 0.2f;
        particle_params.min_time_alive = 0.2f;
        particle_params.max_time_alive = 0.4f;
        particle_params.origin = attack_pos;
        particle_params.min_velocity = 15.0f;
        particle_params.max_velocity = 20.2f;
        particle_params.velocity_offset = glm::vec3(0.0f);
        particle_params.color_a = glm::vec3(1.0f, 0.0f, 0.0f);
        particle_params.color_b = glm::vec3(1.0f, 1.0f, 0.0f);
        particle_params.draw_layer = Layer::World;
        spawn_particle_explosion(particle_params);
    }

    void add_screen_shake(float amount)
    {
        ecs::get_components<Player>().begin()->screen_shake += amount;
    }
}
