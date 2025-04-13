#include "Settings.h"
#include "Utils.h"
#include "InputDefines.h"
#include <fstream>

namespace kvejken::settings
{
    SettingsData m_data;

    SettingsData& get()
    {
        return m_data;
    }

    void load()
    {
        std::ifstream file("kvejken_settings.bin", std::ios::binary);
        if (file.is_open())
        {
            file.read((char*)&m_data, sizeof(m_data));
            file.close();
        }
        else
        {
            m_data.mouse_speed = 20;
            m_data.brightness = 20;
            m_data.fullscreen = true;
            m_data.vsync = false;
            m_data.draw_fps = false;

            m_data.key_forward = GLFW_KEY_W;
            m_data.key_backward = GLFW_KEY_S;
            m_data.key_left = GLFW_KEY_A;
            m_data.key_right = GLFW_KEY_D;
            m_data.key_jump = GLFW_KEY_SPACE;
            m_data.key_interact = GLFW_KEY_E;
            m_data.key_slide = GLFW_KEY_LEFT_SHIFT;

            m_data.window_pos = glm::vec2(100, 100);
            m_data.window_size = glm::vec2(1280, 720);
        }
    }

    void save()
    {
        std::ofstream file("kvejken_settings.bin", std::ios::binary);
        ASSERT(file.is_open() && file.good());

        file.write((const char*)&m_data, sizeof(m_data));
        file.close();
    }
}
