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

    class Model
    {
    public:
        Model(const std::string& file_path);

        const std::vector<Vertex>& vertices() const { return m_vertices; }
        const Texture& diffuse_texture() const { return m_diffuse_texture; }

    private:
        void load_material(const std::string& directory, const std::string& file_path);

        std::vector<Vertex> m_vertices;
        Texture m_diffuse_texture = {};
    };
}
