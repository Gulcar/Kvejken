#include <stdio.h>
#include <stdlib.h>
#include "Renderer.h"
#include "Utils.h"

using namespace kvejken;

int main()
{
    printf("pozdravljen svet\n");

    renderer::create_window("Kvejken", 1280, 720);

    while (renderer::is_window_open())
    {
        renderer::poll_events();
        renderer::clear_screen();
        renderer::swap_buffers();
    }

    renderer::terminate();
}
