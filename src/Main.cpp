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

    Model model("../../assets/test_cube.obj");

    while (renderer::is_window_open())
    {
        renderer::poll_events();

        renderer::clear_screen();

        renderer::draw_model(&model, glm::vec3(std::sin(glfwGetTime()), 0, 0), glm::vec3(1, 1, 1), glm::vec3(0, glfwGetTime(), 0));
        renderer::draw_queue();

        renderer::swap_buffers();
    }
}
