#include "UI.h"
#include "Input.h"
#include "Renderer.h"
#include "Settings.h"
#include <glm/vec2.hpp>
#include <cstdlib>
#include <algorithm>

namespace kvejken::ui
{
    namespace
    {
        Menu m_curr_menu = Menu::Main;

        std::vector<Menu> m_menu_history;

        int* changing_keybind = nullptr;
    }

    static void set_menu(Menu menu)
    {
        m_menu_history.push_back(m_curr_menu);
        m_curr_menu = menu;
    }

    static void set_previous_menu()
    {
        m_curr_menu = m_menu_history.back();
        m_menu_history.pop_back();
    }

    static void quit_menu()
    {
        m_menu_history.clear();
        m_curr_menu = Menu::None;
        input::lock_mouse();
    }

    static bool draw_number_input(const char* text, int* value, int y, int min_value, int max_value, int step = 1)
    {
        renderer::draw_text(text, glm::vec2(600, y), 64);
        int og = *value;

        if (renderer::draw_button("-", glm::vec2(1120, y), 64, glm::vec2(64, 64), glm::vec4(1.0f), Align::Center, (uint64_t)value))
            *value = std::clamp(*value - step, min_value, max_value);

        std::string value_str = std::to_string(*value);
        renderer::draw_text(value_str.c_str(), glm::vec2(1220, y), 64, glm::vec4(1.0f), Align::Center);

        if (renderer::draw_button("+", glm::vec2(1320, y), 64, glm::vec2(64, 64), glm::vec4(1.0f), Align::Center, (uint64_t)value + 1))
            *value = std::clamp(*value + step, min_value, max_value);

        return *value != og;
    }

    // vrne true ce je bilo spremenjeno
    static bool draw_on_off_input(const char* text, bool* value, int y)
    {
        renderer::draw_text(text, glm::vec2(600, y), 64);

        if (*value)
        {
            if (renderer::draw_button("Da", glm::vec2(1220, y), 64, glm::vec2(128, 64), glm::vec4(1.0f), Align::Center))
            {
                *value = false;
                return true;
            }
        }
        else
        {
            if (renderer::draw_button("Ne", glm::vec2(1220, y), 64, glm::vec2(128, 64), glm::vec4(1.0f), Align::Center))
            {
                *value = true;
                return true;
            }
        }

        return false;
    }

    static void draw_keybind_input(const char* text, int* value, int y)
    {
        renderer::draw_text(text, glm::vec2(600, y), 64);

        const char* key_name = input::key_name(*value);
        int width = 96;
        if (strnlen(key_name, 10) > 2) width = 256;

        if (renderer::draw_button(input::key_name(*value), glm::vec2(1220, y), 64, glm::vec2(width, 64), glm::vec4(1.0f), Align::Center))
        {
            changing_keybind = value;
        }
    }

    static void draw_main_menu()
    {
        int y = 300;

        renderer::draw_text("Kvejken", glm::vec2(1920 / 2, y), 169, glm::vec4(1.0f), Align::Center); y += 169;

        if (renderer::draw_button("Igraj", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            set_menu(Menu::Play);
        y += 80;

        if (renderer::draw_button("Nastavitve", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            set_menu(Menu::Settings);
        y += 80;

        if (renderer::draw_button("Izhod", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            std::exit(0);
        y += 80;
    }

    static void draw_play_menu()
    {
        int y = 469;

        if (renderer::draw_button("Lahko", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            quit_menu();
        y += 80;

        if (renderer::draw_button("Navadno", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            quit_menu();
        y += 80;

        if (renderer::draw_button(u8"Težko", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            quit_menu();
        y += 80;
        
        if (renderer::draw_button("Nazaj", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            set_previous_menu();
        y += 80;
    }

    static void draw_settings_menu()
    {
        int y = 339;

        draw_number_input(u8"Hitrost miške", &settings::get().mouse_speed, y, 1, 40); y += 80;
        if (draw_number_input("Svetlost", &settings::get().brightness, y, 1, 40))
            renderer::set_brightness(settings::get().brightness / 20.0f);
        y += 80;

        if (draw_on_off_input("VSync", &settings::get().vsync, y))
            renderer::set_vsync(settings::get().vsync);
        y += 80;

        draw_on_off_input(u8"Prikaži FPS", &settings::get().draw_fps, y); y += 80;

        if (renderer::draw_button("Urejanje tipk", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            set_menu(Menu::Keybinds);
        y += 80;

        if (renderer::draw_button("Nazaj", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
        {
            settings::save();
            set_previous_menu();
        }
        y += 80;
    }

    static void draw_keybinds_menu()
    {
        int y = 269;
        draw_keybind_input("Naprej", &settings::get().key_forward, y); y += 80;
        draw_keybind_input("Levo", &settings::get().key_left, y); y += 80;
        draw_keybind_input("Nazaj", &settings::get().key_backward, y); y += 80;
        draw_keybind_input("Desno", &settings::get().key_right, y); y += 80;
        draw_keybind_input("Skok", &settings::get().key_jump, y); y += 80;
        draw_keybind_input(u8"Poèep", &settings::get().key_slide, y); y += 80;
        draw_keybind_input("Interakcija", &settings::get().key_interact, y); y += 80;

        if (changing_keybind != nullptr)
        {
            renderer::draw_rect(glm::vec2(1920 / 2, 1080 / 2), glm::vec2(1920, 1080) * 10.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.9f));
            renderer::draw_text(u8"pritisni na želeno tipko", glm::vec2(1920 / 2, 1080 / 2), 64, glm::vec4(1.0f), Align::Center);

            if (input::last_key_pressed() != 0)
            {
                *changing_keybind = input::last_key_pressed();
                changing_keybind = nullptr;
            }
        }
        else if (renderer::draw_button("Nazaj", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            set_previous_menu();
    }

    static void draw_pause_menu()
    {
        int y = 300;

        renderer::draw_text("Pavza", glm::vec2(1920 / 2, y), 169, glm::vec4(1.0f), Align::Center); y += 169;

        if (renderer::draw_button("Nadaljuj", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            quit_menu();
        y += 80;

        if (renderer::draw_button("Nastavitve", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            set_menu(Menu::Settings);
        y += 80;

        if (renderer::draw_button("Zapusti igro", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
        {
            m_curr_menu = Menu::Main;
            m_menu_history.clear();
        }
        y += 80;
    }

    void draw_and_update_ui()
    {
        switch (m_curr_menu)
        {
        case Menu::None:
            if (input::key_pressed(GLFW_KEY_ESCAPE))
            {
                m_curr_menu = Menu::Pause;
                input::unlock_mouse();
            }
            break;

        case Menu::Main: draw_main_menu(); break;
        case Menu::Play: draw_play_menu(); break;
        case Menu::Settings: draw_settings_menu(); break;
        case Menu::Keybinds: draw_keybinds_menu(); break;
        case Menu::Pause: draw_pause_menu(); break;
        }
    }

    Menu current_menu()
    {
        return m_curr_menu;
    }
}
