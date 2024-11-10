#include "Model.h"
#include "Utils.h"
#include <fstream>
#include <string>
#include <map>

namespace kvejken
{
    Mesh::Mesh(const std::vector<Vertex>& vertices, Texture texture)
    {
        m_vertices = vertices;
        m_diffuse_texture = texture;
    }

    Mesh::Mesh(std::vector<Vertex>&& vertices, Texture texture)
    {
        m_vertices = std::move(vertices);
        m_diffuse_texture = texture;
    }

    static std::map<std::string, Texture> load_materials(const std::string& directory, const std::string& file_path)
    {
        std::ifstream file(directory + file_path);
        ASSERT(file.is_open() && file.good());

        std::map<std::string, Texture> materials;
        std::string current_material;

        std::string line;
        while (std::getline(file, line))
        {
            if (utils::starts_with(line, "newmtl "))
            {
                current_material = line.substr(7, -1);
            }
            else if (utils::starts_with(line, "map_Kd "))
            {
                std::string file_path = directory + line.substr(7, -1);
                materials[current_material] = renderer::load_texture(file_path.c_str());
            }
        }

        return materials;
    }

    Model::Model(const std::string& file_path)
    {
        std::ifstream file(file_path);
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
                if (current_material.length() > 0)
                {
                    m_meshes.emplace_back(std::move(vertices), materials[current_material]);
                    vertices.clear();
                }
                current_material = line.substr(7, -1);
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

                vertices.push_back(Vertex{
                    positions[av - 1],
                    utils::pack_normals(normals[avn - 1]),
                    utils::pack_texture_coords(texture_coords[avt - 1]),
                });
                vertices.push_back(Vertex{
                    positions[bv - 1],
                    utils::pack_normals(normals[bvn - 1]),
                    utils::pack_texture_coords(texture_coords[bvt - 1]),
                });
                vertices.push_back(Vertex{
                    positions[cv - 1],
                    utils::pack_normals(normals[cvn - 1]),
                    utils::pack_texture_coords(texture_coords[cvt - 1]),
                });
            }
        }

        m_meshes.emplace_back(std::move(vertices), materials[current_material]);
    }
}
