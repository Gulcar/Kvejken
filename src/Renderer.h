#pragma once
#include <glm/vec3.hpp>

namespace kvejken
{
    class Model;
    class Mesh;

    struct Camera
    {
        glm::vec3 position;
        glm::vec3 target;
        glm::vec3 up;
        float fovy;
        float z_near;
        float z_far;
    };

    struct Texture
    {
        uint32_t id;
        int width;
        int height;
    };
}

namespace kvejken::renderer
{
    void create_window(const char* title, int width, int height);
    void terminate();

    bool is_window_open();
    int window_width();
    int window_height();
    float aspect_ratio();

    void poll_events();
    void clear_screen();
    void draw_queue();
    void swap_buffers();

    Texture load_texture(const char* file_path);

    void draw_model(const Model* model, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation);
    void draw_mesh(const Mesh* mesh, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation);
    //void draw_sprite();
}
