#pragma once

#include <glm/vec2.hpp>

namespace kvejken::settings
{
    struct SettingsData
    {
        int mouse_speed;
        int brightness;
        bool fullscreen;
        bool vsync;
        bool draw_fps;

        int key_forward;
        int key_backward;
        int key_left;
        int key_right;
        int key_jump;
        int key_interact;
        int key_slide;

        glm::vec2 window_pos;
        glm::vec2 window_size;
    };

    SettingsData& get();

    void load();
    void save();

    inline int difficulty = 0;
}
