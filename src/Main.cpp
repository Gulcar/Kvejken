#include <stdio.h>
#include <stdlib.h>
#include "Renderer.h"
#include "Utils.h"
#include "Model.h"
#include "ECS.h"
#include "Components.h"
#include "Input.h"
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

    Model terrain("../../assets/environment/terrain.obj");

    std::vector<Model> eels;
    for (int i = 1; i <= 12; i++)
        eels.emplace_back("../../assets/enemies/eel" + std::to_string(i) + ".obj");

    renderer::set_skybox("../../assets/environment/skybox.obj");

    {
        Entity entity = ecs::create_entity();

        Transform transform = {};
        transform.position = glm::vec3(0, 1.5f, 0);
        transform.scale = 1.0f;
        transform.rotation = glm::quat(1, 0, 0, 0);
        ecs::add_component(transform, entity);

        Player player;
        player.health = 100;
        player.local = true;
        ecs::add_component(player, entity);

        Camera camera = {};
        camera.position = glm::vec3(0, 0, 0);
        camera.target = glm::vec3(0, 0, -1);
        camera.up = glm::vec3(0, 1, 0);
        camera.fovy = 50.0f;
        camera.z_near = 0.01f;
        camera.z_far = 100.0f;
        ecs::add_component(camera, entity);
    }

    int frame_count = 0;
    float prev_time = glfwGetTime();

    while (renderer::is_window_open())
    {
        renderer::poll_events();
        float time = glfwGetTime();
        float delta_time = time - prev_time;
        prev_time = time;

        for (auto [player, transform] : ecs::get_components<Player, Transform>())
        {
            if (!player.local)
                continue;
            transform.position.x += input::key_axis(GLFW_KEY_A, GLFW_KEY_D) * delta_time;
            transform.position.z += input::key_axis(GLFW_KEY_W, GLFW_KEY_S) * delta_time;
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
