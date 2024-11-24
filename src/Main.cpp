#include <stdio.h>
#include <stdlib.h>
#include "Renderer.h"
#include "Utils.h"
#include "Model.h"
#include "ECS.h"
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

    int frame_count = 0;

    /*
    printf("%d\n", component_id<float>());
    printf("%d\n", component_id<float>());
    printf("%d\n", component_id<int>());
    printf("%d\n", create_entity());
    printf("%d\n", create_entity());
    printf("%d\n", create_entity());
    */

    Entity e1, e2, e3, e4;
    e1 = ecs::create_entity();
    e2 = ecs::create_entity();
    e3 = ecs::create_entity();
    e4 = ecs::create_entity();

    ecs::add_component(Camera{}, e1);
    ecs::add_component(Camera{}, e2);
    ecs::add_component(glm::vec3(1.0f, 2.0f, 3.0f), e3);
    ecs::add_component(glm::vec3(4.0f, 5.0f, 6.0f), e4);
    ecs::add_component(glm::vec2(), e3);

    auto& c1 = ecs::get_components<Camera>();
    auto& c2 = ecs::get_components<glm::vec3>();
    auto& c3 = ecs::get_components<glm::vec2>();

    for (const auto& v3 : c2)
    {
        printf("(%f, %f, %f)\n", v3.x, v3.y, v3.z);
    }

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
