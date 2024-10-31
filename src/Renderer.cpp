#include "Renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Utils.h"

namespace kvejken::renderer
{
    namespace
    {
        GLFWwindow* m_window = nullptr;
        int m_window_width = 0;
        int m_window_height = 0;
    }

    static void on_window_resize(GLFWwindow* window, int width, int height)
    {
        m_window_width = width;
        m_window_height = height;
        glViewport(0, 0, width, height);
    }

    void create_window(const char* title, int width, int height)
    {
        ASSERT(m_window == nullptr);
        m_window_width = width;
        m_window_height = height;

        if (!glfwInit())
            ERROR_EXIT("Failed to init glfw");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //glfwWindowHint(GLFW_SAMPLES, 4);

        m_window = glfwCreateWindow(width, height, "Kvejken", nullptr, nullptr);
        if (!m_window) ERROR_EXIT("Failed to create a glfw window");

        glfwMakeContextCurrent(m_window);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        printf("OpenGL version: %s\n", glGetString(GL_VERSION));

        glViewport(0, 0, width, height);
        glfwSetFramebufferSizeCallback(m_window, on_window_resize);
    }

    void terminate()
    {
        glfwTerminate();
        m_window = nullptr;
    }

    bool is_window_open()
    {
        return m_window != nullptr &&
            glfwWindowShouldClose(m_window) == false;
    }

    int window_width()
    {
        return m_window_width;
    }

    int window_height()
    {
        return m_window_height;
    }

    void poll_events()
    {
        glfwPollEvents();
    }

    void clear_screen()
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void swap_buffers()
    {
        glfwSwapBuffers(m_window);
    }

    //void draw_model();
    //void draw_sprite();
}
