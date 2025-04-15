#include "Model.h"
#include "Utils.h"
#include <fstream>
#include <string>
#include <map>
#include <glad/glad.h>

namespace kvejken
{
    constexpr size_t MAX_VERTICES_TO_BATCH = 1000;

    Mesh::Mesh(const std::vector<Vertex>& vertices, Texture texture, bool gen_vertex_buffer)
    {
        m_vertex_count = vertices.size();
        m_vertices = vertices;
        m_diffuse_texture = texture;

        if (gen_vertex_buffer)
            prepare_vertex_buffer();
    }

    Mesh::Mesh(std::vector<Vertex>&& vertices, Texture texture, bool gen_vertex_buffer)
    {
        m_vertex_count = vertices.size();
        m_vertices = std::move(vertices);
        m_diffuse_texture = texture;

        if (gen_vertex_buffer)
            prepare_vertex_buffer();
    }

    Mesh::Mesh(Mesh&& other) noexcept
    {
        m_vertices = std::move(other.m_vertices);
        m_diffuse_texture = other.m_diffuse_texture;
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_vertex_count = other.m_vertex_count;

        other.m_vao = -1;
        other.m_vbo = -1;
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        m_vertices = std::move(other.m_vertices);
        m_diffuse_texture = other.m_diffuse_texture;
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_vertex_count = other.m_vertex_count;

        other.m_vao = -1;
        other.m_vbo = -1;
        return *this;
    }

    Mesh::~Mesh()
    {
        if (m_vao != (uint32_t)(-1))
        {
            glDeleteVertexArrays(1, &m_vao);
            glDeleteBuffers(1, &m_vbo);
            m_vao = -1;
            m_vbo = -1;
        }
    }

    void Mesh::prepare_vertex_buffer()
    {
        ASSERT(m_vao == (uint32_t)(-1));

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, texture_coords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, color));

        // free memory
        m_vertices = std::vector<Vertex>();
    }


    static std::map<std::string, Texture> load_materials(const std::string& directory, const std::string& file_path)
    {
        std::ifstream file(directory + file_path);
        if (!file.is_open() || !file.good())
            file = std::ifstream("../../" + directory + file_path);
        ASSERT(file.is_open() && file.good());

        std::map<std::string, Texture> materials;
        std::string current_material;

        std::string line;
        while (std::getline(file, line))
        {
            if (utils::starts_with(line, "newmtl "))
            {
                current_material = line.substr(7, -1);

                if (current_material == "collision_only")
                    materials[current_material] = TEXTURE_COLLISION_ONLY;
            }
            else if (utils::starts_with(line, "map_Kd "))
            {
                std::string file_path = directory + line.substr(7, -1);
                materials[current_material] = renderer::load_texture(file_path.c_str());
            }
        }

        return materials;
    }

    Model::Model(const std::string& file_path, bool allow_vbo)
    {
        std::ifstream file(file_path);
        if (!file.is_open() || !file.good())
            file = std::ifstream("../../" + file_path);
        ASSERT(file.is_open() && file.good());

        std::string directory = "./";
        size_t last_slash_pos = file_path.find_last_of("/\\");
        if (last_slash_pos != std::string::npos)
            directory = file_path.substr(0, last_slash_pos + 1);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texture_coords;

        std::vector<uint32_t> position_indices;
        std::vector<uint32_t> normal_indices;
        std::vector<uint32_t> texture_coords_indices;

        std::map<std::string, Texture> materials;
        std::string current_material;

        std::vector<Vertex> vertices;

        std::string line;
        while (std::getline(file, line))
        {
            if (line.length() == 0 || line[0] == '#')
                continue;

            if (utils::starts_with(line, "mtllib "))
            {
                auto m = load_materials(directory, line.substr(7, -1));
                materials.merge(m);
            }
            else if (utils::starts_with(line, "usemtl "))
            {
                std::string new_material = line.substr(7, -1);
                if (current_material.length() > 0 && current_material != new_material)
                {
                    bool gen_vertex_buffer = vertices.size() > MAX_VERTICES_TO_BATCH && allow_vbo;
                    m_meshes.emplace_back(std::move(vertices), materials[current_material], gen_vertex_buffer);
                    vertices.clear();
                }
                current_material = new_material;
            }
            else if (utils::starts_with(line, "v "))
            {
                float x, y, z;
                ASSERT(sscanf(line.c_str(), "v %f %f %f", &x, &y, &z) == 3);
                positions.push_back({ x, y, z });
            }
            else if (utils::starts_with(line, "vn "))
            {
                float x, y, z;
                ASSERT(sscanf(line.c_str(), "vn %f %f %f", &x, &y, &z) == 3);
                normals.push_back({ x, y, z });
            }
            else if (utils::starts_with(line, "vt "))
            {
                float x, y, z;
                ASSERT(sscanf(line.c_str(), "vt %f %f", &x, &y) == 2);
                texture_coords.push_back({ x, y });
            }
            else if (utils::starts_with(line, "f "))
            {
                int av, avt, avn;
                int bv, bvt, bvn;
                int cv, cvt, cvn;
                ASSERT(sscanf(line.c_str(), "f %i/%i/%i %i/%i/%i %i/%i/%i",
                    &av, &avt, &avn, &bv, &bvt, &bvn, &cv, &cvt, &cvn) == 9);

                glm::vec2 tex_coords_a = texture_coords[avt - 1];
                glm::vec2 tex_coords_b = texture_coords[bvt - 1];
                glm::vec2 tex_coords_c = texture_coords[cvt - 1];
                utils::wrap_texture_coords(&tex_coords_a, &tex_coords_b, &tex_coords_c);

                vertices.push_back(Vertex{
                    positions[av - 1],
                    normals[avn - 1],
                    utils::pack_texture_coords(tex_coords_a),
                    0xffffffff,
                });
                vertices.push_back(Vertex{
                    positions[bv - 1],
                    normals[bvn - 1],
                    utils::pack_texture_coords(tex_coords_b),
                    0xffffffff,
                });
                vertices.push_back(Vertex{
                    positions[cv - 1],
                    normals[cvn - 1],
                    utils::pack_texture_coords(tex_coords_c),
                    0xffffffff,
                });
            }
        }

        bool gen_vertex_buffer = vertices.size() > MAX_VERTICES_TO_BATCH && allow_vbo;
        m_meshes.emplace_back(std::move(vertices), materials[current_material], gen_vertex_buffer);
    }
}

