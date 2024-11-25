#include "Renderer.h"
#include "Model.h"
#include "Utils.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
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
#include "ECS.h"
#include "Components.h"

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

        uint32_t m_batch_vao, m_batch_vbo;
        uint32_t m_shader;

        std::map<std::string, Texture> m_textures;

        Camera m_camera = {};
        glm::mat4 m_view_proj = {};

        std::unique_ptr<Model> m_skybox = nullptr;
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
        m_camera.fovy = 50.0f;
        m_camera.z_near = 0.01f;
        m_camera.z_far = 100.0f;

        // opengl buffers
        glGenVertexArrays(1, &m_batch_vao);
        glBindVertexArray(m_batch_vao);

        glGenBuffers(1, &m_batch_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_batch_vbo);
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

        m_shader = load_shader_program("../../assets/shaders/vert.glsl", "../../assets/shaders/frag.glsl");
        glUseProgram(m_shader);

        int texture_indices[TEXTURES_PER_BATCH];
        for (int i = 0; i < TEXTURES_PER_BATCH; i++)
            texture_indices[i] = i;
        glUniform1iv(glGetUniformLocation(m_shader, "u_textures"), TEXTURES_PER_BATCH, texture_indices);

        glUniform1f(glGetUniformLocation(m_shader, "u_shading"), 1.0f);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        // glEnable(GL_BLEND);

        /*
        * renderer todo:
        * - fog
        * fancy:
        * - shadow mapping
        * - ssao
        * - bloom
        */

        glfwSwapInterval(0);

        stbi_set_flip_vertically_on_load(true);
    }

    void terminate()
    {
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
        return m_window_width / (float)m_window_height;
    }

    void poll_events()
    {
        glfwPollEvents();
    }

    static void draw_skybox(glm::mat4 proj, glm::mat4 view);

    void clear_screen()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto [camera, transform] = *(ecs::get_components<Camera, Transform>().begin());
        m_camera = camera;
        m_camera.position += transform.position;
        glm::vec3 dir = m_camera.target - m_camera.position;
        dir = transform.rotation * dir;
        m_camera.target = m_camera.position + dir;

        glm::mat4 proj = glm::perspective(glm::radians(m_camera.fovy), aspect_ratio(), m_camera.z_near, m_camera.z_far);
        glm::mat4 view = glm::lookAt(m_camera.position, m_camera.target, m_camera.up);
        m_view_proj = proj * view;

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

        glUniformMatrix4fv(glGetUniformLocation(m_shader, "u_view_proj"), 1, GL_FALSE, &m_view_proj[0][0]);
        glm::mat4 identity_normal = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "u_normal_mat"), 1, GL_FALSE, &identity_normal[0][0]);

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

        glm::mat4 proj = m_view_proj * transform;
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "u_view_proj"), 1, GL_FALSE, &proj[0][0]);
        glm::mat4 normal_matrix = glm::transpose(glm::inverse(transform));
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "u_normal_mat"), 1, GL_FALSE, &normal_matrix[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh->diffuse_texture().id);

        glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count());
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
                bv.normal = utils::pack_normals(normal_matrix * glm::vec4(vertex.normal, 1.0f));
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

    void draw_model(const Model* model, glm::vec3 position, glm::quat rotation, glm::vec3 scale)
    {
        for (int i = 0; i < model->meshes().size(); i++)
        {
            draw_mesh(&(model->meshes()[i]), position, rotation, scale);
        }
    }

    void draw_mesh(const Mesh* mesh, glm::vec3 position, glm::quat rotation, glm::vec3 scale)
    {
        // TODO: if not in camera view don't draw
        DrawOrderKey order;
        order.layer = 1;
        order.transparency = 0;
        order.shader_id = 0;
        order.texture_id = mesh->diffuse_texture().id % (1 << 5);

        float distance01 = glm::distance2(position, m_camera.position) / (m_camera.z_far * m_camera.z_far);
        if (distance01 > 1.0f) distance01 = 1.0f;
        if (!order.transparency)
            distance01 = 1.0f - distance01;
        constexpr int max_depth = (1 << 22) - 1; // for 22 bits
        order.depth = (int)(distance01 * max_depth);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::toMat4(rotation)
            * glm::scale(glm::mat4(1.0f), scale);

        m_draw_queue.push_back({ order, mesh, transform });
    }

    //void draw_sprite();

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
        glm::mat4 vp = proj * view;
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "u_view_proj"), 1, GL_FALSE, &vp[0][0]);

        glUniform1f(glGetUniformLocation(m_shader, "u_shading"), 0.0f);

        for (auto& mesh : m_skybox->meshes())
        {
            ASSERT(mesh.has_vertex_buffer());
            glBindVertexArray(mesh.vertex_array_id());

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.diffuse_texture().id);

            glDrawArrays(GL_TRIANGLES, 0, mesh.vertex_count());
        }

        glUniform1f(glGetUniformLocation(m_shader, "u_shading"), 1.0f);
        glEnable(GL_DEPTH_TEST);
    }
}
