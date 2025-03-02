#pragma once

namespace kvejken::ui
{
    enum class Menu
    {
        None,
        Main,
        Play,
        Settings,
        Pause,
    };

    void draw_and_update_ui();

    Menu current_menu();
}
