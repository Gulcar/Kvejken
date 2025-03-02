#include "UI.h"
#include "Input.h"
#include "Renderer.h"
#include <glm/vec2.hpp>
#include <cstdlib>

namespace kvejken::ui
{
    namespace
    {
        Menu m_curr_menu = Menu::Main;

        std::vector<Menu> m_menu_history;
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
        int y = 469;

        renderer::draw_button(u8"Hitrost miške", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center); y += 80;
        renderer::draw_button("V-sync", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center); y += 80;
        renderer::draw_button("Goljufija", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center); y += 80;

        if (renderer::draw_button("Nazaj", glm::vec2(1920 / 2, y), 64, glm::vec2(400, 64), glm::vec4(1.0f), Align::Center))
            set_previous_menu();
        y += 80;
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
        case Menu::Pause: draw_pause_menu(); break;
        }
    }

    Menu current_menu()
    {
        return m_curr_menu;
    }
}
