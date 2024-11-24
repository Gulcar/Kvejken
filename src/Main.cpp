#include <stdio.h>
#include <stdlib.h>
#include "Renderer.h"
#include "Utils.h"
#include "Model.h"
#include "ECS.h"
#include "Components.h"
#include <GLFW/glfw3.h>

using namespace kvejken;

int main()
{
    printf("pozdravljen svet\n");
    atexit(renderer::terminate);

    renderer::create_window("Kvejken", 1280, 720);

    Model test_cube("../../assets/test/test_cube.obj");
    Model test_rock("../../assets/test/test_rock.obj");
    Model test_multiple("../../assets/test/test_multiple.obj");

    Model terrain("../../assets/environment/terrain.obj");

    std::vector<Model> eels;
    for (int i = 1; i <= 12; i++)
        eels.emplace_back("../../assets/enemies/eel" + std::to_string(i) + ".obj");

    renderer::set_skybox("../../assets/environment/skybox.obj");

    Entity player = ecs::create_entity();

    Transform transform = {};
    transform.position = glm::vec3(0, 1.5f, 0);
    transform.scale = 1.0f;
    transform.rotation = glm::quat(0, 0, 0, 1);
    ecs::add_component(transform, player);

    Camera camera = {};
    camera.position = glm::vec3(0, 0, 0);
    camera.target = glm::vec3(0, 0, -1);
    camera.up = glm::vec3(0, 1, 0);
    camera.fovy = 50.0f;
    camera.z_near = 0.01f;
    camera.z_far = 100.0f;
    ecs::add_component(camera, player);

    Player p;
    p.health = 100;
    p.local = true;
    ecs::add_component(p, player);

    Entity test = ecs::create_entity();
    ecs::add_component(Transform{ glm::vec3(1, 2, 3) }, test);
    ecs::add_component(Player{ 1 }, test);
    Entity test2 = ecs::create_entity();
    ecs::add_component(Transform{ glm::vec3(4, 5, 6) }, test2);
    ecs::add_component(Player{ 2 }, test2);

    Entity test3 = ecs::create_entity();
    ecs::add_component(Transform{ glm::vec3(7, 8, 9) }, test3);
    Entity test4 = ecs::create_entity();
    ecs::add_component(Player{ 4 }, test4);

    for (auto& [player, transform] : ecs::get_components<Player, Transform>())
    {
        printf("p: %d, %t: (%f, %f, %f)\n", player.health, transform.position.x, transform.position.y, transform.position.z);
        player.health += 1;
        transform.position.y += 0.1f;
    }

    for (auto& [transform, player] : ecs::get_components<Transform, Player>())
    {
        printf("p: %d, %t: (%f, %f, %f)\n", player.health, transform.position.x, transform.position.y, transform.position.z);
    }

    int frame_count = 0;

    while (renderer::is_window_open())
    {
        renderer::poll_events();

        renderer::clear_screen();

        renderer::draw_model(&test_cube, glm::vec3(std::sin(glfwGetTime()), 0, 0), glm::vec3(1, 1, 1), glm::vec3(0, glfwGetTime(), 0));
        renderer::draw_model(&test_cube, glm::vec3(3, 0.4f, -5), glm::vec3(0.5f), glm::vec3(0, 0, 0));
        renderer::draw_model(&test_cube, glm::vec3(2, 0, -3), glm::vec3(0.5f), glm::vec3(0, 0, 0));
        for (int i = 0; i < 100; i++)
        {
            renderer::draw_model(&test_cube, glm::vec3(i, 0, -3), glm::vec3(0.5f), glm::vec3(0, 0, 0));
        }

        renderer::draw_model(&test_rock, glm::vec3(4, 0, 0), glm::vec3(1, 1, 1), glm::vec3(0, -glfwGetTime(), 0));
        renderer::draw_model(&test_multiple, glm::vec3(-2, 0, -2), glm::vec3(0.5f), glm::vec3(0, -PI / 2.0f, 0));
        renderer::draw_model(&terrain, glm::vec3(0, -5, 0), glm::vec3(1), glm::vec3(0));

        int eel_index = (int)(std::fmodf(glfwGetTime(), 0.5f) / 0.5f * 12);
        renderer::draw_model(&eels[eel_index], glm::vec3(-2, 0, 0), glm::vec3(0.5f), glm::vec3(0));

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
