#pragma once
#include <string_view>
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
        Model(std::string_view file_name);

        const std::vector<Vertex>& vertices() const { return m_vertices; }

    private:
        std::vector<Vertex> m_vertices;
    };
}
