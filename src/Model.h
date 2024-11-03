#pragma once
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
        glm::vec2 texture_coords;
    };

    class Model
    {
    public:
        Model(const std::string& file_path);

        const std::vector<Vertex>& vertices() const { return m_vertices; }
        const std::string& diffuse_texture_file() const { return m_diffuse_texture_file; }

    private:
        void load_material(const std::string& directory, const std::string& file_path);

        std::vector<Vertex> m_vertices;
        std::string m_diffuse_texture_file;
    };
}
