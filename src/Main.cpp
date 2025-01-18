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
#include "Player.h"

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

    {
        Entity e0 = ecs::create_entity();
        Entity e1 = ecs::create_entity();

        struct A {};
        struct B {};
        ecs::add_component(A{}, e0);
        ecs::add_component(A{}, e1);
        ecs::add_component(B{}, e1);

        for (auto& [a, b] : ecs::get_components<A, B>())
        {
            printf("test");
        }
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

        update_players(delta_time, game_time);

        ecs::destroy_queued_entities();
        /*
        for (const auto& pool : ecs::component_pools())
            printf("component pool %s size %d\n", pool->component_name(), (int)pool->size());
        */

        renderer::clear_screen();

        /*
        renderer::draw_model(&test_cube, glm::vec3(std::sin(glfwGetTime()), 0, 0), glm::vec3(0, glfwGetTime(), 0), glm::vec3(1.0f));
        renderer::draw_model(&test_cube, glm::vec3(3, 0.4f, -5), glm::vec3(0, 0, 0), glm::vec3(0.5f));
        renderer::draw_model(&test_cube, glm::vec3(2, 0, -3), glm::vec3(0, 0, 0), glm::vec3(0.5f));
        for (int i = 0; i < 100; i++)
        {
            renderer::draw_model(&test_cube, glm::vec3(i, 0, -3), glm::vec3(0, 0, 0), glm::vec3(0.5f));
        }

        renderer::draw_model(&test_rock, glm::vec3(4, 0, 0), glm::vec3(0, -glfwGetTime(), 0), glm::vec3(1.0f));
        renderer::draw_model(&test_multiple, glm::vec3(-2, 0, -2), glm::vec3(0, -PI / 2.0f, 0), glm::vec3(0.5f));
        */

        renderer::draw_model(&terrain, glm::vec3(0, -5, 0), glm::vec3(0, 0, 0), glm::vec3(1.0f));

        int eel_index = (int)(std::fmodf(glfwGetTime(), 0.5f) / 0.5f * 12);
        renderer::draw_model(&eels[eel_index], glm::vec3(-2, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0.5f));

        for (auto [player, transform] : ecs::get_components<Player, Transform>())
        {
            if (!player.local)
                continue;

            for (int i = 0; i < 300; i++)
            {
                glm::vec3 dir = glm::normalize(glm::vec3(utils::randf(-0.2f, 0.2f), utils::randf(-0.2f, 0.2f), -1.0f));
                auto hit = collision::raycast(transform.position, transform.rotation * dir);
                if (hit)
                {
                    renderer::draw_model(&test_cube, hit->position, glm::vec3(0, 0, 0), glm::vec3(0.1f));
                }
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
