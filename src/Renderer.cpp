#include "Renderer.h"
#include "Model.h"
#include "Utils.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <stb_rect_pack.h>
#include <stb_truetype.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <map>
#include <memory>
#include <cstring>
#include "ECS.h"
#include "Components.h"
#include "Shader.h"
#include "Input.h"

namespace kvejken::renderer
{
    namespace
    {
        GLFWwindow* m_window = nullptr;
        int m_window_width = 0;
        int m_window_height = 0;

        struct DrawOrderKey
        {
            // bits are defined from LSB to MSB
            uint32_t depth : 21; // closer to camera is a higher number (backwards if transparent)
            uint32_t texture_id : 5;
            uint32_t shader_id : 3;
            uint32_t transparency : 1;
            uint32_t layer : 2; // from enum Layer
        };
        static_assert(sizeof(DrawOrderKey) == sizeof(uint32_t));

        struct DrawCommand
        {
            DrawOrderKey order;
            const Mesh* mesh;
            glm::mat4 transform;
            uint32_t color;
        };
        std::vector<DrawCommand> m_draw_queue;

        struct BatchVertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::u16vec2 texture_coords;
            uint32_t color;
            uint8_t texture_index;
        };
        constexpr size_t VERTICES_PER_BATCH = 4128;
        static_assert(VERTICES_PER_BATCH % 3 == 0);
        constexpr size_t VERTEX_BUFFER_SIZE = VERTICES_PER_BATCH * sizeof(BatchVertex);
        constexpr size_t TEXTURES_PER_BATCH = 16; // https://stackoverflow.com/a/46428637
        std::vector<BatchVertex> m_batched_vertices;
        std::vector<uint32_t> m_batched_textures;

        uint32_t m_batch_vao, m_batch_vbo;
        Shader m_shader;

        std::map<std::string, Texture> m_textures;

        Camera m_camera = {};
        glm::mat4 m_view_proj = {};

        constexpr float UI_WIDTH = 1920;
        constexpr float UI_HEIGHT = 1080;
        glm::mat4 m_ui_view_proj = {};
        glm::mat4 m_ui_view_proj_inv = {};

        struct UIVertex
        {
            glm::vec2 position;
            glm::vec2 texture_coords;
            uint32_t color;
            bool is_text;
            uint8_t texture_index;
        };
        std::vector<UIVertex> m_ui_batched_vertices;

        struct UITextureChange
        {
            size_t vertex_index;
            uint32_t texture_id;
        };
        std::vector<UITextureChange> m_ui_texture_changes;

        constexpr size_t UI_VERTICES_PER_BATCH = 2048;
        constexpr size_t UI_VERTEX_BUFFER_SIZE = UI_VERTICES_PER_BATCH * sizeof(UIVertex);
        uint32_t m_ui_batch_vao, m_ui_batch_vbo;
        Shader m_ui_shader;

        constexpr int FONT_ATLAS_WIDTH = 1024;
        constexpr int FONT_ATLAS_FONT_SIZE = 64;
        Texture m_font_atlas = {};
        std::vector<stbtt_packedchar> m_font_atlas_coords;

        Texture m_white_texture = {};

        std::unique_ptr<Model> m_skybox = nullptr;
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

        // default camera
        m_camera.position = glm::vec3(-2.5f, 1.7f, 3.0f);
        m_camera.direction = glm::normalize(glm::vec3(0, 0, 0) - m_camera.position);
        m_camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
        m_camera.fovy = 50.0f;
        m_camera.z_near = 0.01f;
        m_camera.z_far = 100.0f;

        // opengl buffers for 3d vertices
        glGenVertexArrays(1, &m_batch_vao);
        glBindVertexArray(m_batch_vao);

        glGenBuffers(1, &m_batch_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_batch_vbo);
        glBufferData(GL_ARRAY_BUFFER, VERTEX_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, texture_coords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, color));
        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 1, GL_UNSIGNED_BYTE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, texture_index));

        m_batched_vertices.reserve(VERTICES_PER_BATCH);

        m_shader = Shader("assets/shaders/vert.glsl", "assets/shaders/frag.glsl");
        glUseProgram(m_shader.id());

        int texture_indices[TEXTURES_PER_BATCH];
        for (int i = 0; i < TEXTURES_PER_BATCH; i++)
            texture_indices[i] = i;
        m_shader.set_uniform("u_textures", texture_indices, TEXTURES_PER_BATCH);

        m_shader.set_uniform("u_shading", 1.0f);
        m_shader.set_uniform("u_sun_light", 1.0f);


        // opengl buffers for 2d vertices
        glGenVertexArrays(1, &m_ui_batch_vao);
        glBindVertexArray(m_ui_batch_vao);

        glGenBuffers(1, &m_ui_batch_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_ui_batch_vbo);
        glBufferData(GL_ARRAY_BUFFER, UI_VERTEX_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)offsetof(UIVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)offsetof(UIVertex, texture_coords));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UIVertex), (void*)offsetof(UIVertex, color));
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(UIVertex), (void*)offsetof(UIVertex, is_text));
        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 1, GL_UNSIGNED_BYTE, sizeof(UIVertex), (void*)offsetof(UIVertex, texture_index));

        m_ui_batched_vertices.reserve(UI_VERTICES_PER_BATCH);

        m_ui_shader = Shader("assets/shaders/ui_vert.glsl", "assets/shaders/ui_frag.glsl");
        glUseProgram(m_ui_shader.id());
        m_ui_shader.set_uniform("u_textures", texture_indices, TEXTURES_PER_BATCH);

        glUseProgram(m_shader.id());

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_FRAMEBUFFER_SRGB);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /*
        * renderer todo:
        * - fog
        * fancy:
        * - shadow mapping
        * - ssao
        * - bloom
        */

        stbi_set_flip_vertically_on_load(true);

        uint8_t white_pixel[] = { 0xff, 0xff, 0xff, 0xff };
        m_white_texture = load_texture(white_pixel, 1, 1, 4);
    }

    void terminate()
    {
        m_skybox = nullptr;
        glfwTerminate();
        m_window = nullptr;
    }

    GLFWwindow* window_ptr()
    {
        return m_window;
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

    float aspect_ratio()
    {
        if (m_window_height > 0)
            return m_window_width / (float)m_window_height;
        return 1;
    }

    glm::vec2 mouse_ui_position()
    {
        glm::vec2 sp = input::mouse_screen_position();
        sp.x = (sp.x / window_width()) * 2.0f - 1.0f;
        sp.y = -((sp.y / window_height()) * 2.0f - 1.0f);
        glm::vec4 up = m_ui_view_proj_inv * glm::vec4(sp, 0.0f, 1.0f);
        return glm::vec2(up.x, up.y);
    }

    void set_fullscreen()
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }

    void set_windowed(glm::ivec2 position, glm::ivec2 size)
    {
        glfwSetWindowMonitor(m_window, nullptr, position.x, position.y, size.x, size.y, 0);
    }

    bool is_fullscreen()
    {
        return glfwGetWindowMonitor(m_window) != nullptr;
    }

    void set_vsync(bool enable)
    {
        glfwSwapInterval(enable ? 1 : 0);
    }

    glm::ivec2 get_window_position()
    {
        glm::ivec2 pos;
        glfwGetWindowPos(m_window, &pos.x, &pos.y);
        return pos;
    }

    void set_window_position(glm::ivec2 position)
    {
        glfwSetWindowPos(m_window, position.x, position.y);
    }

    void poll_events()
    {
        glfwPollEvents();
    }

    static void draw_skybox(glm::mat4 proj, glm::mat4 view);

    static void update_point_lights()
    {
        constexpr int MAX_POINT_LIGHTS = 4;
        static std::vector<glm::vec3> point_lights_pos;
        static std::vector<glm::vec3> point_lights_color;
        static std::vector<float> point_lights_strength;
        point_lights_pos.clear();
        point_lights_color.clear();
        point_lights_strength.clear();

        for (const auto [light, transform] : ecs::get_components<PointLight, Transform>())
        {
            point_lights_pos.push_back(transform.position + light.offset);
            point_lights_color.push_back(light.color);
            point_lights_strength.push_back(light.strength);
        }

        int count = point_lights_pos.size();
        if (count > MAX_POINT_LIGHTS)
        {
            // TODO: vrjetno ne bom rabil ampak ce zelim vec lightov bi moral uporabljat samo najblizje
            printf("too many point lights (count %d)\n", count);
            count = MAX_POINT_LIGHTS;
        }

        m_shader.set_uniform("u_num_point_lights", count);
        if (count > 0)
        {
            m_shader.set_uniform("u_point_lights_pos", point_lights_pos.data(), count);
            m_shader.set_uniform("u_point_lights_color", point_lights_color.data(), count);
            m_shader.set_uniform("u_point_lights_strength", point_lights_strength.data(), count);
        }
    }

    void clear_screen()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto [camera, transform] = *(ecs::get_components<Camera, Transform>().begin());
        m_camera = camera;
        m_camera.position += transform.position;
        m_camera.direction = transform.rotation * m_camera.direction;

        glm::mat4 proj = glm::perspective(glm::radians(m_camera.fovy), aspect_ratio(), m_camera.z_near, m_camera.z_far);
        glm::mat4 view = glm::lookAt(m_camera.position, m_camera.position + m_camera.direction, m_camera.up);
        m_view_proj = proj * view;

        if (aspect_ratio() > UI_WIDTH / UI_HEIGHT)
        {
            // extra width
            float b = (window_width() * UI_HEIGHT / window_height()) - UI_WIDTH;
            m_ui_view_proj = glm::ortho(-b / 2, UI_WIDTH + b / 2, UI_HEIGHT, 0.0f);
        }
        else
        {
            // extra height
            float b = (window_height() * UI_WIDTH / window_width()) - UI_HEIGHT;
            m_ui_view_proj = glm::ortho(0.0f, UI_WIDTH, UI_HEIGHT + b / 2, -b / 2);
        }

        m_ui_view_proj_inv = glm::inverse(m_ui_view_proj);

        update_point_lights();

        if (m_skybox)
            draw_skybox(proj, view);
    }

    static bool draw_sort_predicate(const DrawCommand& a, const DrawCommand& b)
    {
        return *(uint32_t*)(&a.order) > *(uint32_t*)(&b.order);
    }

    static void draw_batch()
    {
        if (m_batched_vertices.size() == 0)
            return;

        for (int i = 0; i < m_batched_textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_batched_textures[i]);
        }

        m_shader.set_uniform("u_model_view_proj", m_view_proj);
        m_shader.set_uniform("u_normal_mat", glm::mat4(1.0f));
        m_shader.set_uniform("u_model", glm::mat4(1.0f));

        glBindVertexArray(m_batch_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_batch_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_batched_vertices.size() * sizeof(BatchVertex), m_batched_vertices.data());
        glDrawArrays(GL_TRIANGLES, 0, m_batched_vertices.size());

        m_batched_vertices.clear();
        m_batched_textures.clear();
    }

    static void draw_single_mesh(const Mesh* mesh, const glm::mat4& transform)
    {
        glBindVertexArray(mesh->vertex_array_id());

        m_shader.set_uniform("u_model_view_proj", m_view_proj * transform);
        m_shader.set_uniform("u_model", transform);
        m_shader.set_uniform("u_normal_mat", glm::transpose(glm::inverse(transform)));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh->diffuse_texture().id);

        glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count());
    }

    static void draw_ui_batch(int start_vertex, int count)
    {
        if (count == 0)
            return;

        for (int i = 0; i < m_batched_textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_batched_textures[i]);
        }

        glBindVertexArray(m_ui_batch_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_ui_batch_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(UIVertex), m_ui_batched_vertices.data() + start_vertex);
        glDrawArrays(GL_TRIANGLES, 0, count);

        m_batched_textures.clear();
    }

    void draw_queue()
    {
        // sortiraj tako da bodo DrawOrderKey po vrednosti padajoci
        std::sort(m_draw_queue.begin(), m_draw_queue.end(), draw_sort_predicate);

        uint32_t shader_id = m_draw_queue[0].order.shader_id;

        for (int i = 0; i < m_draw_queue.size(); i++)
        {
            const Mesh* mesh = m_draw_queue[i].mesh;

            if (i == 0 || m_draw_queue[i - 1].order.layer != m_draw_queue[i].order.layer)
            {
                draw_batch();

                switch (m_draw_queue[i].order.layer)
                {
                case (uint32_t)Layer::World:
                    break;

                case (uint32_t)Layer::FirstPerson:
                    glClear(GL_DEPTH_BUFFER_BIT);
                    break;

                case (uint32_t)Layer::UserInterface:
                    break;
                }
            }

            if (mesh->has_vertex_buffer())
            {
                draw_single_mesh(mesh, m_draw_queue[i].transform);
                continue;
            }

            // TODO: if (shader_id != m_draw_queue[i].first.shader_id)

            uint8_t texture_index = 255;
            for (int i = 0; i < m_batched_textures.size(); i++)
            {
                if (m_batched_textures[i] == mesh->diffuse_texture().id)
                {
                    texture_index = i;
                    break;
                }
            }

            if (texture_index == 255)
            {
                if (m_batched_textures.size() >= TEXTURES_PER_BATCH)
                    draw_batch();
                texture_index = m_batched_textures.size();
                m_batched_textures.push_back(mesh->diffuse_texture().id);
            }

            glm::mat4 normal_matrix = glm::transpose(glm::inverse(m_draw_queue[i].transform));

            for (const auto& vertex : mesh->vertices())
            {
                if (m_batched_vertices.size() >= VERTICES_PER_BATCH)
                {
                    draw_batch();
                    texture_index = 0;
                    m_batched_textures.push_back(mesh->diffuse_texture().id);
                }

                BatchVertex bv;
                bv.position = m_draw_queue[i].transform * glm::vec4(vertex.position, 1.0f);
                bv.normal = normal_matrix * glm::vec4(vertex.normal, 1.0f);
                bv.texture_coords = vertex.texture_coords;
                bv.color = glm::packUnorm4x8(glm::unpackUnorm4x8(vertex.color) * glm::unpackUnorm4x8(m_draw_queue[i].color));
                bv.texture_index = texture_index;
                m_batched_vertices.push_back(bv);
            }
        }

        draw_batch();

        m_draw_queue.clear();
        m_batched_vertices.clear();

        // konec risanja 3d zdaj samo se UI
        glUseProgram(m_ui_shader.id());
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        m_ui_shader.set_uniform("u_view_proj", m_ui_view_proj);

        int tex_change_i = -1;
        uint8_t texture_index = 0;
        int start_vertex = 0;
        for (int i = 0; i < m_ui_batched_vertices.size(); i++)
        {
            if (tex_change_i + 1 < m_ui_texture_changes.size() && m_ui_texture_changes[tex_change_i + 1].vertex_index == i)
            {
                texture_index = 255;

                for (int j = 0; j < m_batched_textures.size(); j++)
                {
                    if (m_batched_textures[j] == m_ui_texture_changes[tex_change_i + 1].texture_id)
                    {
                        texture_index = j;
                        break;
                    }
                }

                if (texture_index == 255)
                {
                    if (m_batched_textures.size() >= TEXTURES_PER_BATCH)
                    {
                        draw_ui_batch(start_vertex, i - start_vertex);
                        start_vertex = i + 1;
                    }
                    texture_index = m_batched_textures.size();
                    m_batched_textures.push_back(m_ui_texture_changes[tex_change_i + 1].texture_id);
                }
                
                tex_change_i++;
            }

            if (i - start_vertex >= UI_VERTICES_PER_BATCH)
            {
                draw_ui_batch(start_vertex, i - start_vertex);
                start_vertex = i + 1;
                texture_index = 0;
                m_batched_textures.push_back(m_ui_texture_changes[tex_change_i].texture_id);
            }

            m_ui_batched_vertices[i].texture_index = texture_index;
        }
        
        draw_ui_batch(start_vertex, m_ui_batched_vertices.size() - start_vertex);
        m_ui_batched_vertices.clear();
        m_ui_texture_changes.clear();

        glUseProgram(m_shader.id());
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    void swap_buffers()
    {
        glfwSwapBuffers(m_window);
    }

    Texture load_texture(const char* file_path, bool srgb)
    {
        auto it = m_textures.find(file_path);
        if (it != m_textures.end())
            return it->second;

        int width, height;
        int num_components;
        uint8_t* data = stbi_load(file_path, &width, &height, &num_components, 0);
        if (data == nullptr)
        {
            std::string new_path = "../../" + std::string(file_path);
            data = stbi_load(new_path.c_str(), &width, &height, &num_components, 0);
        }

        if (data == nullptr)
            ERROR_EXIT("Failed to load texture '%s' (%s)", file_path, stbi_failure_reason());

        Texture tex = load_texture(data, width, height, num_components, srgb);

        stbi_image_free(data);

        m_textures[file_path] = tex;
        return tex;
    }

    Texture load_texture(const uint8_t* data, int width, int height, int num_components, bool srgb)
    {
        if ((width % 4 != 0 || height % 4 != 0) && (width != 1 || height != 1))
            ERROR_EXIT("Texture size is not a multiple of 4 (w=%d, h=%d)", width, height);

        Texture tex = {};
        tex.width = width;
        tex.height = height;

        glGenTextures(1, &tex.id);
        glBindTexture(GL_TEXTURE_2D, tex.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        GLenum internal_format, format = 0;
        if (srgb)
        {
            switch (num_components)
            {
            case 1: internal_format = GL_R8;            format = GL_RED; break;
            case 3: internal_format = GL_SRGB8;         format = GL_RGB; break;
            case 4: internal_format = GL_SRGB8_ALPHA8;  format = GL_RGBA; break;
            }
        }
        else
        {
            switch (num_components)
            {
            case 1: internal_format = GL_R8;    format = GL_RED; break;
            case 3: internal_format = GL_RGB8;  format = GL_RGB; break;
            case 4: internal_format = GL_RGBA8; format = GL_RGBA; break;
            }
        }

        if (format == 0)
            ERROR_EXIT("Invalid number of channels on image (%d)", num_components);

        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, tex.width, tex.height, 0, format, GL_UNSIGNED_BYTE, data);

        return tex;
    }

    void draw_model(const Model* model, glm::vec3 position, glm::quat rotation, glm::vec3 scale, Layer layer, glm::vec4 color)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::toMat4(rotation)
            * glm::scale(glm::mat4(1.0f), scale);

        for (int i = 0; i < model->meshes().size(); i++)
        {
            draw_mesh(&(model->meshes()[i]), transform, layer, color);
        }
    }

    void draw_mesh(const Mesh* mesh, glm::vec3 position, glm::quat rotation, glm::vec3 scale, Layer layer, glm::vec4 color)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::toMat4(rotation)
            * glm::scale(glm::mat4(1.0f), scale);

        draw_mesh(mesh, transform, layer, color);
    }

    void draw_model(const Model* model, const glm::mat4& transform, Layer layer, glm::vec4 color)
    {
        for (int i = 0; i < model->meshes().size(); i++)
        {
            draw_mesh(&(model->meshes()[i]), transform, layer, color);
        }
    }

    void draw_mesh(const Mesh* mesh, const glm::mat4& transform, Layer layer, glm::vec4 color)
    {
        if (mesh->diffuse_texture() == TEXTURE_COLLISION_ONLY)
            return; // ne risi collision_only objektov

        // TODO: if not in camera view don't draw
        DrawOrderKey order;
        order.layer = (int)layer;
        order.transparency = 0;
        order.shader_id = 0;
        order.texture_id = mesh->diffuse_texture().id % (1 << 5);

        glm::vec3 position = transform[3];

        float distance01 = glm::distance2(position, m_camera.position) / (m_camera.z_far * m_camera.z_far);
        if (distance01 > 1.0f) distance01 = 1.0f;
        if (!order.transparency)
            distance01 = 1.0f - distance01;
        constexpr int max_depth = (1 << 21) - 1; // for 21 bits
        order.depth = (int)(distance01 * max_depth);

        m_draw_queue.push_back({ order, mesh, transform, utils::vec_to_rgba8(color) });
    }
    
    constexpr uint32_t decode_2_byte_utf8(char byte1, char byte2)
    {
        uint32_t a = (uint32_t)byte1;
        uint32_t b = (uint32_t)byte2;

        if (a < 0b1100'0000 || b < 0b1000'0000)
            return 0;

        uint32_t z = b & 0b0000'1111;
        uint32_t y = ((a & 0b0000'0011) << 2) | ((b & 0b0011'0000) >> 4);
        uint32_t x = (a & 0b0001'1100) >> 2;

        return x * 256 + y * 16 + z;
    }

    uint32_t decode_2_byte_utf8(const char* c)
    {
        ASSERT(strlen(c) == 2);
        return decode_2_byte_utf8(c[0], c[1]);
    }

    void load_font(const char* font_file)
    {
        stbtt_pack_context stbtt_ctx;
        uint8_t* pixels = new uint8_t[FONT_ATLAS_WIDTH * FONT_ATLAS_WIDTH];

        if (stbtt_PackBegin(&stbtt_ctx, pixels, FONT_ATLAS_WIDTH, FONT_ATLAS_WIDTH, 0, 1, nullptr) == 0)
            ERROR_EXIT("failed to begin stbtt pack context");

        stbtt_PackSetOversampling(&stbtt_ctx, 2, 2);

        uint8_t* ttf_data = utils::read_file_bytes(font_file);

        std::vector<int> codepoints;
        for (int i = 32; i <= 126; i++) codepoints.push_back(i);
        codepoints.push_back(decode_2_byte_utf8(u8"Č"));
        codepoints.push_back(decode_2_byte_utf8(u8"č"));
        codepoints.push_back(decode_2_byte_utf8(u8"Š"));
        codepoints.push_back(decode_2_byte_utf8(u8"š"));
        codepoints.push_back(decode_2_byte_utf8(u8"Ž"));
        codepoints.push_back(decode_2_byte_utf8(u8"ž"));

        std::vector<stbtt_packedchar> packed_chars(codepoints.size());

        stbtt_pack_range range = {};
        range.font_size = STBTT_POINT_SIZE(FONT_ATLAS_FONT_SIZE);
        range.array_of_unicode_codepoints = codepoints.data();
        range.num_chars = codepoints.size();
        range.chardata_for_range = packed_chars.data();

        if (stbtt_PackFontRanges(&stbtt_ctx, ttf_data, 0, &range, 1) == 0)
            ERROR_EXIT("failed to pack font ranges");

        stbtt_PackEnd(&stbtt_ctx);

        m_font_atlas = load_texture(pixels, FONT_ATLAS_WIDTH, FONT_ATLAS_WIDTH, 1);
        m_font_atlas_coords = std::move(packed_chars);

        delete[] ttf_data;
        delete[] pixels;
    }

    void draw_text(const char* text, glm::vec2 position, int size, glm::vec4 color, Align horizontal_align)
    {
        if (m_ui_texture_changes.size() == 0 || m_ui_texture_changes.back().texture_id != m_font_atlas.id)
            m_ui_texture_changes.push_back({ m_ui_batched_vertices.size(), m_font_atlas.id });

        float x = position.x;
        float y = position.y;
        float scale = size / (float)FONT_ATLAS_FONT_SIZE;

        uint32_t rgba8 = utils::vec_to_rgba8(color);

        float measured_width = 0.0f;
        int start_vertex = m_ui_batched_vertices.size();

        for (int i = 0; text[i] != '\0'; i++)
        {			
            const stbtt_packedchar* pc = nullptr;
            int char_bytes = 1;

            // ce je ascii vrednost
            if (text[i] >= 32 && text[i] <= 126)
            {
                pc = &m_font_atlas_coords[text[i] - 32];
            }
            else // ce ne preveri samo slovenske crke iz utf8
            {
                //uint32_t c = decode_2_byte_utf8(text[i], text[i + 1]);

                if (strncmp(text + i, u8"Č", 2) == 0)
                    pc = &m_font_atlas_coords[95];
                else if (strncmp(text + i, u8"č", 2) == 0)
                    pc = &m_font_atlas_coords[96];
                else if (strncmp(text + i, u8"Š", 2) == 0)
                    pc = &m_font_atlas_coords[97];
                else if (strncmp(text + i, u8"š", 2) == 0)
                    pc = &m_font_atlas_coords[98];
                else if (strncmp(text + i, u8"Ž", 2) == 0)
                    pc = &m_font_atlas_coords[99];
                else if (strncmp(text + i, u8"ž", 2) == 0)
                    pc = &m_font_atlas_coords[100];
                else
                    ERROR_EXIT("invalid character in draw_text");

                char_bytes = 2;
            }

            if (text[i + char_bytes - 1] == '\0')
                measured_width += pc->xoff2 * scale;
            else
                measured_width += pc->xadvance * scale;
            
            if (text[i] == ' ')
            {
                x += pc->xadvance * scale;
                continue;
            }

            float x0 = x + pc->xoff * scale;
            float y0 = y + pc->yoff * scale;
            float x1 = x + pc->xoff2 * scale;
            float y1 = y + pc->yoff2 * scale;

            float s0 = pc->x0 / (float)FONT_ATLAS_WIDTH;
            float t0 = pc->y0 / (float)FONT_ATLAS_WIDTH;
            float s1 = pc->x1 / (float)FONT_ATLAS_WIDTH;
            float t1 = pc->y1 / (float)FONT_ATLAS_WIDTH;

            m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x0, y0), glm::vec2(s0, t0), rgba8, true, 0 });
            m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x1, y1), glm::vec2(s1, t1), rgba8, true, 0 });
            m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x1, y0), glm::vec2(s1, t0), rgba8, true, 0 });

            m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x0, y0), glm::vec2(s0, t0), rgba8, true, 0 });
            m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x0, y1), glm::vec2(s0, t1), rgba8, true, 0 });
            m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x1, y1), glm::vec2(s1, t1), rgba8, true, 0 });

            x += pc->xadvance * scale;

            i += char_bytes - 1;
        }

        if (horizontal_align != Align::Left)
        {
            float align_offset = 0.0f;
            if (horizontal_align == Align::Center) align_offset = measured_width / 2.0f;
            else if (horizontal_align == Align::Right) align_offset = measured_width;

            for (int i = start_vertex; i < m_ui_batched_vertices.size(); i++)
            {
                m_ui_batched_vertices[i].position.x -= align_offset;
            }
        }
    }

    bool draw_button(const char* text, glm::vec2 position, int size, glm::vec2 rect_size, glm::vec4 color, Align horizontal_align, uint64_t repeats_id)
    {
        glm::vec2 rect_pos = position;
        if (horizontal_align == Align::Left) rect_pos.x += rect_size.x / 2.0f - size / 8.0f;
        else if (horizontal_align == Align::Right) rect_pos.x -= rect_size.x / 2.0f - size / 8.0f;

        rect_pos.y -= size / 2.8f;

        bool hover = utils::point_in_rect(mouse_ui_position(), rect_pos, rect_size);
        if (hover)
            draw_rect(rect_pos, rect_size, color * glm::vec4(1.0f, 1.0f, 1.0f, 0.05f));

        draw_text(text, position, size, color, horizontal_align);

        if (repeats_id != 0)
        {
            struct MouseHold
            {
                float pressed_time;
                int repeats;
            };
            static std::map<uint64_t, MouseHold> mouse_hold;

            MouseHold& curr = mouse_hold[repeats_id];

            if (hover && input::mouse_pressed(GLFW_MOUSE_BUTTON_LEFT))
            {
                curr.pressed_time = glfwGetTime();
            }
            else if (!hover || !input::mouse_held(GLFW_MOUSE_BUTTON_LEFT))
            {
                curr.pressed_time = 0.0f;
                curr.repeats = 0;
            }

            if (curr.pressed_time != 0.0f)
            {
                float diff = glfwGetTime() - curr.pressed_time;
                if (curr.repeats == 0 && diff > 0.6f)
                {
                    curr.repeats++;
                    return true;
                }
                if (curr.repeats > 0 && diff > curr.repeats * 0.1f + 0.6f)
                {
                    curr.repeats++;
                    return true;
                }
            }
        }

        return hover && input::mouse_pressed(GLFW_MOUSE_BUTTON_LEFT);
    }

    void draw_rect(glm::vec2 position, glm::vec2 size, glm::vec4 color)
    {
        draw_rect(m_white_texture, position, size, glm::vec2(0, 0), glm::vec2(1, 1), color);
    }

    void draw_rect(const Texture& texture, glm::vec2 position, glm::vec2 size, glm::vec4 color)
    {
        draw_rect(texture, position, size, glm::vec2(0, 0), glm::vec2(texture.width, texture.height), color);
    }

    void draw_rect(const Texture& texture, glm::vec2 position, glm::vec2 size, glm::vec2 tex_min_coord, glm::vec2 tex_max_coord, glm::vec4 color)
    {
        if (m_ui_texture_changes.size() == 0 || m_ui_texture_changes.back().texture_id != texture.id)
            m_ui_texture_changes.push_back({ m_ui_batched_vertices.size(), texture.id });

        uint32_t rgba8 = utils::vec_to_rgba8(color);

        float x0 = position.x - size.x / 2.0f;
        float x1 = position.x + size.x / 2.0f;
        float y0 = position.y - size.y / 2.0f;
        float y1 = position.y + size.y / 2.0f;

        float s0 = tex_min_coord.s / texture.width;
        float s1 = tex_max_coord.s / texture.width;
        float t0 = tex_min_coord.t / texture.height;
        float t1 = tex_max_coord.t / texture.height;

        m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x0, y0), glm::vec2(s0, t1), rgba8, false, 0 });
        m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x1, y1), glm::vec2(s1, t0), rgba8, false, 0 });
        m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x1, y0), glm::vec2(s1, t1), rgba8, false, 0 });

        m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x0, y0), glm::vec2(s0, t1), rgba8, false, 0 });
        m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x0, y1), glm::vec2(s0, t0), rgba8, false, 0 });
        m_ui_batched_vertices.push_back(UIVertex{ glm::vec2(x1, y1), glm::vec2(s1, t0), rgba8, false, 0 });
    }

    void set_skybox(const std::string& skybox_obj_path)
    {
        m_skybox = std::make_unique<Model>(skybox_obj_path);

        for (auto& mesh : m_skybox->meshes())
        {
            if (!mesh.has_vertex_buffer())
                mesh.prepare_vertex_buffer();
        }
    }

    static void draw_skybox(glm::mat4 proj, glm::mat4 view)
    {
        glDisable(GL_DEPTH_TEST);

        // odstrani premik, zato da je skybox vedno centriran na kameri
        view = glm::mat4(glm::mat3(view));
        m_shader.set_uniform("u_model_view_proj", proj * view);

        m_shader.set_uniform("u_shading", 0.0f);

        for (auto& mesh : m_skybox->meshes())
        {
            ASSERT(mesh.has_vertex_buffer());
            glBindVertexArray(mesh.vertex_array_id());

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.diffuse_texture().id);

            glDrawArrays(GL_TRIANGLES, 0, mesh.vertex_count());
        }

        m_shader.set_uniform("u_shading", 1.0f);
        glEnable(GL_DEPTH_TEST);
    }

    void set_sun_light(float strength)
    {
        m_shader.set_uniform("u_sun_light", strength);
    }

    void set_brightness(float value)
    {
        m_shader.set_uniform("u_brightness", value);
    }
}

