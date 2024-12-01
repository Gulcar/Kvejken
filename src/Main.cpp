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
        transform.position = glm::vec3(0, 1.5f, 0);
        transform.rotation = glm::quat(1, 0, 0, 0);
        transform.scale = 1.0f;
        ecs::add_component(transform, entity);

        Player player = {};
        player.health = 100;
        player.local = true;
        ecs::add_component(player, entity);

        Camera camera = {};
        camera.position = glm::vec3(0, 0, 0);
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
        prev_time = game_time;

        if (input::mouse_pressed(GLFW_MOUSE_BUTTON_LEFT))
            input::lock_mouse();
        if (input::key_pressed(GLFW_KEY_ESCAPE))
            input::unlock_mouse();

        for (auto [player, transform] : ecs::get_components<Player, Transform>())
        {
            if (!player.local)
                continue;

            glm::vec3 forward = transform.rotation * glm::vec3(0, 0, -1);
            glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));
            glm::vec3 move_dir = {};
            move_dir += forward * (float)input::key_axis(GLFW_KEY_S, GLFW_KEY_W);
            move_dir += right * (float)input::key_axis(GLFW_KEY_A, GLFW_KEY_D);
            move_dir.y = 0.0f;

            bool moving = false;
            if (move_dir != glm::vec3(0, 0, 0))
            {
                move_dir = glm::normalize(move_dir);

                float dist = 100.0f;
                // TODO: check wider
                collision::raycast(transform.position, move_dir, nullptr, &dist);
                if (dist > 0.5f)
                {
                    player.move_velocity += glm::vec2(move_dir.x, move_dir.z) * 70.0f * delta_time;
                    moving = true;
                }
            }

            if (moving == false)
            {
                if (glm::length(player.move_velocity) < 40.0f * delta_time)
                    player.move_velocity = glm::vec2(0, 0);
                else
                    player.move_velocity -= glm::normalize(player.move_velocity) * 40.0f * delta_time;
            }

            float move_velocity = glm::length(player.move_velocity);
            if (move_velocity > 6.0f)
            {
                player.move_velocity = player.move_velocity / move_velocity * 6.0f;
            }

            transform.position += glm::vec3(player.move_velocity.x, 0.0f, player.move_velocity.y) * delta_time;

            if (input::is_mouse_locked())
            {
                glm::vec2 mouse_delta = input::mouse_delta();
                transform.rotation = glm::angleAxis(-mouse_delta.x / 1000.0f, glm::vec3(0, 1, 0)) * transform.rotation;
                transform.rotation = glm::angleAxis(-mouse_delta.y / 1000.0f, right) * transform.rotation;
            }

            float dist = 100.0f;
            collision::raycast(transform.position, glm::vec3(0, -1, 0), nullptr, &dist);
            if (dist > 1.0f)
            {
                player.velocity_y -= 25.0f * delta_time;
            }
            else
            {
                transform.position.y += 1.0f - dist;
                player.velocity_y = -0.01f;
                player.jump_allowed_time = game_time + 0.12f;
            }

            if (input::key_pressed(GLFW_KEY_SPACE) && game_time <= player.jump_allowed_time)
            {
                player.velocity_y += 9.0f;
            }

            transform.position.y += player.velocity_y * delta_time;
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

            glm::vec3 hit_pos;
            if (collision::raycast(transform.position, transform.rotation * glm::vec3(0, 0, -1), &hit_pos, nullptr))
            {
                renderer::draw_model(&test_cube, hit_pos, glm::vec3(0, 0, 0), glm::vec3(0.5f));
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
