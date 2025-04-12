#pragma once
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

struct GLFWwindow;

namespace kvejken
{
    class Model;
    class Mesh;

    struct Camera
    {
        glm::vec3 position;
        glm::vec3 direction;
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

        bool operator==(const Texture& o) const
        {
            return this->id == o.id && this->height == o.height && this->width == o.width;
        }
    };
    constexpr Texture TEXTURE_COLLISION_ONLY = Texture{ 0 };

    enum class Layer
    {
        World = 3,
        FirstPerson = 2,
        UserInterface = 0,
    };

    enum class Align
    {
        Left,
        Center,
        Right,
    };

    struct PointLight
    {
        glm::vec3 offset;
        glm::vec3 color;
        float strength;
    };
}

namespace kvejken::renderer
{
    void create_window(const char* title, int width, int height);
    void terminate();
    GLFWwindow* window_ptr();

    bool is_window_open();
    int window_width();
    int window_height();
    float aspect_ratio();
    glm::vec2 mouse_ui_position();

    void poll_events();
    void clear_screen();
    void draw_queue();
    void swap_buffers();

    Texture load_texture(const char* file_path, bool srgb = true);
    Texture load_texture(const uint8_t* data, int width, int height, int num_components, bool srgb = true);

    void draw_model(const Model* model, glm::vec3 position, glm::quat rotation, glm::vec3 scale, Layer layer = Layer::World, glm::vec4 color = glm::vec4(1.0f));
    void draw_mesh(const Mesh* mesh, glm::vec3 position, glm::quat rotation, glm::vec3 scale, Layer layer = Layer::World, glm::vec4 color = glm::vec4(1.0f));

    void draw_model(const Model* model, const glm::mat4& transform, Layer layer = Layer::World, glm::vec4 color = glm::vec4(1.0f));
    void draw_mesh(const Mesh* mesh, const glm::mat4& transform, Layer layer = Layer::World, glm::vec4 color = glm::vec4(1.0f));

    void load_font(const char* font_file);
    void draw_text(const char* text, glm::vec2 position, int size, glm::vec4 color = glm::vec4(1.0f), Align horizontal_align = Align::Left);
    bool draw_button(const char* text, glm::vec2 position, int size, glm::vec2 rect_size, glm::vec4 color = glm::vec4(1.0f), Align horizontal_align = Align::Left, bool allow_repeats = false);

    void draw_rect(glm::vec2 position, glm::vec2 size, glm::vec4 color);
    void draw_rect(const Texture& texture, glm::vec2 position, glm::vec2 size, glm::vec4 color = glm::vec4(1.0f));
    void draw_rect(const Texture& texture, glm::vec2 position, glm::vec2 size, glm::vec2 tex_min_coord, glm::vec2 tex_max_coord, glm::vec4 color = glm::vec4(1.0f));

    void set_skybox(const std::string& skybox_obj_path);
    void set_sun_light(float strength);
}
