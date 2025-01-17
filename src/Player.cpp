#include "Player.h"
#include "ECS.h"
#include "Components.h"
#include "Input.h"
#include "Collision.h"

namespace kvejken
{
    constexpr float MAX_MOVE_SPEED = 6.0f;
    constexpr float MAX_MOVE_SPEED_AIR = 9.0f;
    constexpr float ACCELERATION = 80.0f;
    constexpr float DECELERATION = 35.0f;
    constexpr float ACCELERATION_AIR = ACCELERATION * 0.2f;
    constexpr float DECELERATION_AIR = DECELERATION * 0.1f;

    constexpr float MOUSE_SENS = 1.0f / 1000.0f;

    constexpr float PLAYER_GRAVITY = -25.0f;
    constexpr float MAX_Y_VELOCITY = -PLAYER_GRAVITY * 5.0f;
    constexpr float JUMP_STRENGTH = 9.0f;
    constexpr float COYOTE_TIME = 0.12f;

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

            if (move_dir != glm::vec3(0, 0, 0))
            {
                move_dir = glm::normalize(move_dir);

                float accel = (grounded) ? ACCELERATION : ACCELERATION_AIR;
                player.move_velocity += glm::vec2(move_dir.x, move_dir.z) * accel * delta_time;
            }
            else
            {
                float decel = (grounded) ? DECELERATION : DECELERATION_AIR;
                if (glm::length(player.move_velocity) > decel * delta_time)
                    player.move_velocity -= glm::normalize(player.move_velocity) * decel * delta_time;
                else
                    player.move_velocity = glm::vec2(0, 0);
            }

            float max_velocity = (grounded) ? MAX_MOVE_SPEED : MAX_MOVE_SPEED_AIR;
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

    void update_players(float delta_time, float game_time)
    {
        int substeps = (int)(delta_time / 0.010f) + 1;
        float sub_delta = delta_time / substeps;
        for (int i = 0; i < substeps; i++)
            update_players_movement(sub_delta, game_time);

        for (auto [player, transform] : ecs::get_components<Player, Transform>())
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
        }
    }
}
