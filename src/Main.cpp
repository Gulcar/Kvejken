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
#include "Enemy.h"
#include "Assets.h"
#include "Interactable.h"

using namespace kvejken;

enum class Menu
{
    Main,
    Play,
    Settings,
};

void draw_main_menu()
{
    int y = 300;

    renderer::draw_text("Kvejken", glm::vec2(1920 / 2, y), 169, glm::vec4(1.0f), Align::Center); y += 169;
    renderer::draw_button("Igraj", glm::vec2(1920 / 2, y), 64, glm::vec2(300, 64), glm::vec4(1.0f), Align::Center); y += 80;
    renderer::draw_button("Nastavitve", glm::vec2(1920 / 2, y), 64, glm::vec2(300, 64), glm::vec4(1.0f), Align::Center); y += 80;
    renderer::draw_button("Izhod", glm::vec2(1920 / 2, y), 64, glm::vec2(300, 64), glm::vec4(1.0f), Align::Center); y += 80;
}

int main()
{
    printf("pozdravljen svet\n");
    atexit(renderer::terminate);

    renderer::create_window("Kvejken", 1280, 720);
    renderer::load_font("assets/Shafarik-Regular.ttf");
    
    input::init(renderer::window_ptr());

    assets::load();
    atexit(assets::unload);

    init_enemies();
    init_weapon_item_infos();

    spawn_local_player(glm::vec3(0, 8, 0));

    spawn_interactables();

    collision::build_triangle_bvh(*assets::terrain, glm::vec3(0), glm::vec3(0), glm::vec3(1.0f));
    for (auto& mesh : assets::terrain->meshes())
    {
        if (mesh.vertices().size() > 1000)
            mesh.prepare_vertex_buffer();
    }

    renderer::set_skybox("assets/environment/skybox.obj");

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

        update_enemies(delta_time, game_time);

        update_interactables(delta_time, game_time);


        ecs::destroy_queued_entities();
        /*
        printf("ecs components:\n");
        for (const auto& pool : ecs::component_pools())
            printf("component pool %s size %d\n", pool->component_name(), (int)pool->size());
        */

        renderer::clear_screen();

        draw_main_menu();

        /*
        renderer::draw_model(assets::test_cube.get(), glm::vec3(std::sin(glfwGetTime()), 5, 0), glm::vec3(0, glfwGetTime(), 0), glm::vec3(1.0f));
        renderer::draw_model(assets::test_cube.get(), glm::vec3(3, 5.4f, -5), glm::vec3(0, 0, 0), glm::vec3(0.5f));
        renderer::draw_model(assets::test_cube.get(), glm::vec3(2, 5, -3), glm::vec3(0, 0, 0), glm::vec3(0.5f));
        for (int i = 0; i < 100; i++)
        {
            renderer::draw_model(assets::test_cube.get(), glm::vec3(i, 5, -3), glm::vec3(0, 0, 0), glm::vec3(0.5f));
        }

        renderer::draw_model(assets::test_rock.get(), glm::vec3(4, 5, 0), glm::vec3(0, -glfwGetTime(), 0), glm::vec3(1.0f));
        renderer::draw_model(assets::test_multiple.get(), glm::vec3(-2, 5, -2), glm::vec3(0, -PI / 2.0f, 0), glm::vec3(0.5f));
        */

        renderer::draw_model(assets::terrain.get(), glm::vec3(0), glm::vec3(0), glm::vec3(1.0f));

        /*
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
        */

        for (auto [model, transform] : ecs::get_components<Model*, Transform>())
        {
            renderer::draw_model(model, transform.position, transform.rotation, glm::vec3(transform.scale));
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
