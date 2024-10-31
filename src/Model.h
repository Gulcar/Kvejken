#pragma once
#include <string_view>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace kvejken
{
    class Model
    {
    public:
        Model(std::string_view file_name);

    private:
        std::vector<glm::vec3> m_vertices;
        std::vector<glm::vec3> m_normals;
        std::vector<glm::vec2> m_texture_coords;
        std::vector<uint32_t> m_indices;
        std::vector<uint32_t> m_normal_indices;
        std::vector<uint32_t> m_texture_coords_indices;
    };
}
