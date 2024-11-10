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
        uint32_t normal;
        glm::u16vec2 texture_coords;
    };

    class Mesh
    {
    public:
        Mesh(const std::vector<Vertex>& vertices, Texture texture);
        Mesh(std::vector<Vertex>&& vertices, Texture texture);

        const std::vector<Vertex>& vertices() const { return m_vertices; }
        const Texture& diffuse_texture() const { return m_diffuse_texture; }

    private:
        std::vector<Vertex> m_vertices;
        Texture m_diffuse_texture = {};
    };

    class Model
    {
    public:
        Model(const std::string& file_path);

        const std::vector<Mesh>& meshes() const { return m_meshes; }

    private:
        std::vector<Mesh> m_meshes;
    };
}
