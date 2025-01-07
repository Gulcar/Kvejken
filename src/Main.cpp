#include <stdio.h>
#include <stdlib.h>
#include "Renderer.h"
#include "Utils.h"
#include "Model.h"
#include "ECS.h"
#include "Components.h"
#include "Input.h"
#include "Collision.h"
#include <GLFW/glfw3.h>

using namespace kvejken;

int main()
{
    printf("pozdravljen svet\n");
    atexit(renderer::terminate);

    renderer::create_window("Kvejken", 1280, 720);
    
    input::init(renderer::window_ptr());

    Model test_cube("../../assets/test/test_cube.obj");
    Model test_rock("../../assets/test/test_rock.obj");
    Model test_multiple("../../assets/test/test_multiple.obj");

    Model terrain("../../assets/environment/terrain.obj", false);
    collision::build_triangle_bvh(terrain, glm::vec3(0, -5, 0), glm::vec3(0, 0, 0), glm::vec3(1.0f));
    for (auto& mesh : terrain.meshes())
    {
        if (mesh.vertices().size() > 1000)
            mesh.prepare_vertex_buffer();
    }

    std::vector<Model> eels;
    for (int i = 1; i <= 12; i++)
        eels.emplace_back("../../assets/enemies/eel" + std::to_string(i) + ".obj");

    renderer::set_skybox("../../assets/environment/skybox.obj");

    {
        Entity entity = ecs::create_entity();

        Transform transform = {};
        transform.position = glm::vec3(0, 3, 0);
        transform.rotation = glm::quat(1, 0, 0, 0);
        transform.scale = 1.0f;
        ecs::add_component(transform, entity);

        Player player = {};
        player.health = 100;
        player.local = true;
        ecs::add_component(player, entity);

        Camera camera = {};
        camera.position = glm::vec3(0, 0.45f, 0);
        camera.direction = glm::vec3(0, 0, -1);
        camera.up = glm::vec3(0, 1, 0);
        camera.fovy = 50.0f;
        camera.z_near = 0.1f;
        camera.z_far = 150.0f;
        ecs::add_component(camera, entity);
    }

    int frame_count = 0;
    float prev_time = glfwGetTime();

    while (renderer::is_window_open())
    {
        input::clear();
        renderer::poll_events();

        float game_time = glfwGetTime();
        float delta_time = game_time - prev_time;
        if (delta_time > 1.0f / 15.0f)
        {
            printf("WARNING: high delta_time (%.2f ms)\n", delta_time * 1000.0f);
            delta_time = 1.0f / 15.0f;
        }
        prev_time += delta_time;

        if (input::mouse_pressed(GLFW_MOUSE_BUTTON_LEFT))
            input::lock_mouse();
        if (input::key_pressed(GLFW_KEY_ESCAPE))
            input::unlock_mouse();

        // TODO: premakni playerja v vecih stepih da ne bo padel skozi tla
        for (auto [player, transform] : ecs::get_components<Player, Transform>())
        {
            if (!player.local)
                continue;

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

        renderer::clear_screen();

        renderer::draw_model(&test_cube, glm::vec3(std::sin(glfwGetTime()), 0, 0), glm::vec3(0, glfwGetTime(), 0), glm::vec3(1.0f));
        renderer::draw_model(&test_cube, glm::vec3(3, 0.4f, -5), glm::vec3(0, 0, 0), glm::vec3(0.5f));
        renderer::draw_model(&test_cube, glm::vec3(2, 0, -3), glm::vec3(0, 0, 0), glm::vec3(0.5f));
        for (int i = 0; i < 100; i++)
        {
            renderer::draw_model(&test_cube, glm::vec3(i, 0, -3), glm::vec3(0, 0, 0), glm::vec3(0.5f));
        }

        renderer::draw_model(&test_rock, glm::vec3(4, 0, 0), glm::vec3(0, -glfwGetTime(), 0), glm::vec3(1.0f));
        renderer::draw_model(&test_multiple, glm::vec3(-2, 0, -2), glm::vec3(0, -PI / 2.0f, 0), glm::vec3(0.5f));
        renderer::draw_model(&terrain, glm::vec3(0, -5, 0), glm::vec3(0, 0, 0), glm::vec3(1.0f));

        int eel_index = (int)(std::fmodf(glfwGetTime(), 0.5f) / 0.5f * 12);
        renderer::draw_model(&eels[eel_index], glm::vec3(-2, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0.5f));

        for (auto [player, transform] : ecs::get_components<Player, Transform>())
        {
            if (!player.local)
                continue;

            auto hit = collision::raycast(transform.position, transform.rotation * glm::vec3(0, 0, -1));
            if (hit)
            {
                renderer::draw_model(&test_cube, hit->position, glm::vec3(0, 0, 0), glm::vec3(0.1f));
            }
        }

        renderer::draw_queue();

        renderer::swap_buffers();

        frame_count++;
        if (frame_count % 512 == 0)
        {
            float frametime = glfwGetTime() / frame_count;
            printf("frametime: %f ms (%d fps)\n", frametime * 1000, (int)(1 / frametime));
        }
    }
}
