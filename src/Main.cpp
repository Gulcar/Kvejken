#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define ERROR_EXIT(...) { fprintf(stderr, "ERORR: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "!\n"); exit(1); }

void on_window_resize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    // Width = width; Height = height;
}

int main()
{
    printf("pozdravljen svet\n");

    if (!glfwInit())
        ERROR_EXIT("Failed to init glfw");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Kvejken", nullptr, nullptr);
    if (!window) ERROR_EXIT("Failed to create a glfw window");

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    printf("OpenGL version: %s\n", glGetString(GL_VERSION));

    glViewport(0, 0, 640, 480);
    glfwSetFramebufferSizeCallback(window, on_window_resize);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
