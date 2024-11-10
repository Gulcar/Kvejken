#include "Renderer.h"
#include "Model.h"
#include "Utils.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <map>

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
            uint32_t depth : 22; // closer to camera is a higher number (backwards if transparent)
            uint32_t texture_id : 5;
            uint32_t shader_id : 3;
            uint32_t transparency : 1;
            uint32_t layer : 1; // hud or world
        };
        static_assert(sizeof(DrawOrderKey) == sizeof(uint32_t));

        struct DrawCommand
        {
            DrawOrderKey order;
            const Mesh* mesh;
            glm::mat4 transform;
        };
        std::vector<DrawCommand> m_draw_queue;

        struct BatchVertex
        {
            glm::vec3 position;
            uint32_t normal;
            glm::u16vec2 texture_coords;
            uint8_t texture_index;
        };
        constexpr size_t VERTICES_PER_BATCH = 4128;
        static_assert(VERTICES_PER_BATCH % 3 == 0);
        constexpr size_t VERTEX_BUFFER_SIZE = VERTICES_PER_BATCH * sizeof(BatchVertex);
        constexpr size_t TEXTURES_PER_BATCH = 16; // https://stackoverflow.com/a/46428637
        std::vector<BatchVertex> m_batched_vertices;
        std::vector<uint32_t> m_batched_textures;

        uint32_t m_vao, m_vbo;
        uint32_t m_shader;

        std::map<std::string, Texture> m_textures;

        Camera m_camera = {};
        glm::mat4 m_view_proj = {};
    }

    static uint32_t load_shader(std::string_view source, GLenum type)
    {
        uint32_t shader = glCreateShader(type);
        const char* char_source = source.data();
        glShaderSource(shader, 1, &char_source, 0);
        glCompileShader(shader);
        return shader;
    }

    static uint32_t load_shader_program(std::string_view vertex_src_file, std::string_view fragment_src_file)
    {
        uint32_t vertex_shader = load_shader(utils::read_file_to_string(vertex_src_file), GL_VERTEX_SHADER);
        uint32_t fragment_shader = load_shader(utils::read_file_to_string(fragment_src_file), GL_FRAGMENT_SHADER);

        uint32_t program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        int link_status;
        glGetProgramiv(program, GL_LINK_STATUS, &link_status);
        if (!link_status)
        {
            int log_length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
            char* log = new char[log_length + 1];
            glGetProgramInfoLog(program, log_length + 1, 0, log);
            ERROR_EXIT("failed to link shader program:\n%s", log);
        }

        glDetachShader(program, vertex_shader);
        glDetachShader(program, fragment_shader);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        return program;
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
        m_camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
        m_camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
        m_camera.fovy = 60.0f;
        m_camera.z_near = 0.01f;
        m_camera.z_far = 100.0f;

        // opengl buffers
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, VERTEX_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, texture_coords));
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(BatchVertex), (void*)offsetof(BatchVertex, texture_index));

        m_batched_vertices.reserve(VERTICES_PER_BATCH);

        m_shader = load_shader_program("../../assets/vert.glsl", "../../assets/frag.glsl");
        glUseProgram(m_shader);

        int texture_indices[TEXTURES_PER_BATCH];
        for (int i = 0; i < TEXTURES_PER_BATCH; i++)
            texture_indices[i] = i;
        glUniform1iv(glGetUniformLocation(m_shader, "u_textures"), TEXTURES_PER_BATCH, texture_indices);

        glEnable(GL_DEPTH_TEST);

        stbi_set_flip_vertically_on_load(true);
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

    float aspect_ratio()
    {
        return m_window_width / (float)m_window_height;
    }

    void poll_events()
    {
        glfwPollEvents();
    }

    void clear_screen()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(m_camera.fovy), aspect_ratio(), m_camera.z_near, m_camera.z_far);
        glm::mat4 view = glm::lookAt(m_camera.position, m_camera.target, m_camera.up);
        m_view_proj = proj * view;
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "u_view_proj"), 1, GL_FALSE, &m_view_proj[0][0]);
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

        glBufferSubData(GL_ARRAY_BUFFER, 0, m_batched_vertices.size() * sizeof(BatchVertex), m_batched_vertices.data());
        glDrawArrays(GL_TRIANGLES, 0, m_batched_vertices.size());

        m_batched_vertices.clear();
        m_batched_textures.clear();
    }

    void draw_queue()
    {
        if (m_draw_queue.size() == 0)
            return;

        std::sort(m_draw_queue.begin(), m_draw_queue.end(), draw_sort_predicate);

        uint32_t shader_id = m_draw_queue[0].order.shader_id;

        for (int i = 0; i < m_draw_queue.size(); i++)
        {
            const Mesh* mesh = m_draw_queue[i].mesh;

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
                bv.normal = vertex.normal;
                bv.texture_coords = vertex.texture_coords;
                bv.texture_index = texture_index;
                m_batched_vertices.push_back(bv);
            }
        }

        draw_batch();

        m_draw_queue.clear();
        m_batched_vertices.clear();
    }

    void swap_buffers()
    {
        glfwSwapBuffers(m_window);
    }

    Texture load_texture(const char* file_path)
    {
        auto it = m_textures.find(file_path);
        if (it != m_textures.end())
            return it->second;

        Texture tex = {};
        int num_components;
        uint8_t* data = stbi_load(file_path, &tex.width, &tex.height, &num_components, 0);
        if (data == nullptr)
            ERROR_EXIT("Failed to load texture '%s' (%s)", file_path, stbi_failure_reason());
        if (tex.width % 4 != 0 || tex.height % 4 != 0)
            ERROR_EXIT("Texture size is not a multiple of 4 (w=%d, h=%d)", tex.width, tex.height);

        glGenTextures(1, &tex.id);
        glBindTexture(GL_TEXTURE_2D, tex.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        GLenum internal_format, format;
        switch (num_components)
        {
        case 1: internal_format = GL_R8;    format = GL_RED; break;
        case 3: internal_format = GL_RGB8;  format = GL_RGB; break;
        case 4: internal_format = GL_RGBA8; format = GL_RGBA; break;
        default:
            ERROR_EXIT("Invalid number of channels on image (%d)", num_components);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, tex.width, tex.height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);

        m_textures[file_path] = tex;
        return tex;
    }

    void draw_model(const Model* model, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation)
    {
        for (int i = 0; i < model->meshes().size(); i++)
        {
            draw_mesh(&(model->meshes()[i]), position, scale, rotation);
        }
    }

    void draw_mesh(const Mesh* mesh, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation)
    {
        // TODO: if not in camera view don't draw
        DrawOrderKey order;
        order.layer = 1;
        order.transparency = 0;
        order.shader_id = 0;
        order.texture_id = mesh->diffuse_texture().id % (1 << 5);

        float distance01 = glm::distance2(position, m_camera.position) / (m_camera.z_far * m_camera.z_far);
        constexpr int max_depth = (2 << 22) - 1; // for 22 bits
        if (!order.transparency)
            distance01 = 1.0f - distance01;
        order.depth = (int)(distance01 * max_depth);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z)
            * glm::scale(glm::mat4(1.0f), scale);

        m_draw_queue.push_back({ order, mesh, transform });
    }

    //void draw_sprite();
}
