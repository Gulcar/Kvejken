#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "InputDefines.h"

struct GLFWwindow;

namespace kvejken::input
{
    void init(GLFWwindow* window);
    void clear();

    bool key_held(int key);
    bool key_pressed(int key);
    bool key_released(int key);

    int key_axis(int neg, int poz);

    bool mouse_held(int button);
    bool mouse_pressed(int button);
    bool mouse_released(int button);

    glm::vec2 mouse_delta();
    glm::vec2 mouse_screen_position();
    glm::vec3 mouse_world_position();

    void lock_mouse();
    void unlock_mouse();
    bool is_mouse_locked();
}
