#include <stdio.h>
#include <stdlib.h>
#include "Renderer.h"
#include "Utils.h"
#include "Model.h"

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
        renderer::swap_buffers();
    }
}
