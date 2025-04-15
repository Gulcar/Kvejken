#pragma once
#include "Renderer.h"
#include <string>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace kvejken
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::u16vec2 texture_coords;
        uint32_t color;
    };

    class Mesh
    {
    public:
        Mesh(const std::vector<Vertex>& vertices, Texture texture, bool gen_vertex_buffer);
        Mesh(std::vector<Vertex>&& vertices, Texture texture, bool gen_vertex_buffer);

        Mesh(const Mesh& other) = delete;
        Mesh& operator=(const Mesh& other) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        ~Mesh();

        void prepare_vertex_buffer();

        const std::vector<Vertex>& vertices() const { return m_vertices; }
        const Texture& diffuse_texture() const { return m_diffuse_texture; }

        bool has_vertex_buffer() const { return m_vbo != (uint32_t)(-1); }
        uint32_t vertex_array_id() const { return m_vao; }
        uint32_t vertex_count() const { return m_vertex_count; }

    private:
        std::vector<Vertex> m_vertices;
        Texture m_diffuse_texture;
        uint32_t m_vao = -1, m_vbo = -1;
        uint32_t m_vertex_count;
    };

    class Model
    {
    public:
        Model(const std::string& file_path, bool allow_vbo = true);

        std::vector<Mesh>& meshes() { return m_meshes; }
        const std::vector<Mesh>& meshes() const { return m_meshes; }

    private:
        std::vector<Mesh> m_meshes;
    };
}

