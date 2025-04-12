#pragma once

namespace kvejken::settings
{
    struct SettingsData
    {
        int mouse_speed;
        int brightness;
        bool vsync;
        bool draw_fps;

        int key_forward;
        int key_backward;
        int key_left;
        int key_right;
        int key_jump;
        int key_interact;
        int key_slide;
    };

    SettingsData& get();

    void load();
    void save();

    inline int difficulty = 0;
}
