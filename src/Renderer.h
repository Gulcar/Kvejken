#pragma once

namespace kvejken::renderer
{
    void create_window(const char* title, int width, int height);
    void terminate();

    bool is_window_open();
    int window_width();
    int window_height();

    void poll_events();
    void clear_screen();
    void swap_buffers();

    //void draw_model();
    //void draw_sprite();
}
