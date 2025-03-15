#include "Input.h"
#include <GLFW/glfw3.h>
#include <cstring>

namespace kvejken::input
{
    static bool m_keys_just_pressed[GLFW_KEY_LAST + 1];
    static bool m_keys_just_released[GLFW_KEY_LAST + 1];

    static bool m_mouse_just_pressed[GLFW_MOUSE_BUTTON_LAST + 1];
    static bool m_mouse_just_released[GLFW_MOUSE_BUTTON_LAST + 1];

    static GLFWwindow* m_window = nullptr;

    static glm::vec2 m_prev_mouse_pos = {};

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            m_keys_just_pressed[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            m_keys_just_released[key] = true;
        }
    }

    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            m_mouse_just_pressed[button] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            m_mouse_just_released[button] = true;
        }
    }

    void init(GLFWwindow* window)
    {
        m_window = window;
        glfwSetKeyCallback(m_window, key_callback);
        glfwSetMouseButtonCallback(m_window, mouse_button_callback);
        clear();

        // raw mouse motion bo vklopljen le ko bo GLFW_CURSOR_DISABLED
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        else
            printf("raw mouse motion not supported\n");
    }

    void clear()
    {
        memset(m_keys_just_pressed, 0, sizeof(m_keys_just_pressed));
        memset(m_keys_just_released, 0, sizeof(m_keys_just_released));

        memset(m_mouse_just_pressed, 0, sizeof(m_mouse_just_pressed));
        memset(m_mouse_just_released, 0, sizeof(m_mouse_just_released));

        m_prev_mouse_pos = mouse_screen_position();
    }

    bool key_held(int key)
    {
        return glfwGetKey(m_window, key);
    }

    bool key_pressed(int key)
    {
        return m_keys_just_pressed[key];
    }

    bool key_released(int key)
    {
        return m_keys_just_released[key];
    }

    int key_axis(int neg, int poz)
    {
        return glfwGetKey(m_window, poz) - glfwGetKey(m_window, neg);
    }

    bool mouse_held(int button)
    {
        return glfwGetMouseButton(m_window, button);
    }

    bool mouse_pressed(int button)
    {
        return m_mouse_just_pressed[button];
    }

    bool mouse_released(int button)
    {
        return m_mouse_just_released[button];
    }

    glm::vec2 mouse_delta()
    {
        return mouse_screen_position() - m_prev_mouse_pos;
    }

    glm::vec2 mouse_screen_position()
    {
        double x, y;
        glfwGetCursorPos(m_window, &x, &y);
        return glm::vec2((float)x, (float)y);
    }

    glm::vec3 mouse_world_position()
    {
        assert(false && "mouse_world_position ni narejeno verjetno ne bom rabil");
        return glm::vec3(0, 0, 0);
    }

    void lock_mouse()
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void unlock_mouse()
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    bool is_mouse_locked()
    {
        return glfwGetInputMode(m_window, GLFW_CURSOR) != GLFW_CURSOR_NORMAL;
    }
}
