#include <stdio.h>
#include <stdlib.h>
#include "Renderer.h"
#include "Utils.h"
#include "Model.h"
#include <GLFW/glfw3.h>

using namespace kvejken;

int main()
{
    printf("pozdravljen svet\n");
    atexit(renderer::terminate);

    renderer::create_window("Kvejken", 1280, 720);

    Model test_cube("../../assets/test_cube.obj");
    Model test_rock("../../assets/test_rock.obj");

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

        renderer::draw_queue();

        renderer::swap_buffers();
    }
}
