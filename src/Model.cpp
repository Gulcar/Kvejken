#include "Model.h"
#include "Utils.h"
#include <fstream>
#include <string>

namespace kvejken
{
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

        std::string line;
        while (std::getline(file, line))
        {
            if (line.length() == 0 || line[0] == '#')
                continue;

            if (utils::starts_with(line, "v "))
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

                position_indices.push_back(av);
                position_indices.push_back(bv);
                position_indices.push_back(cv);

                texture_coords_indices.push_back(avt);
                texture_coords_indices.push_back(bvt);
                texture_coords_indices.push_back(cvt);

                normal_indices.push_back(avn);
                normal_indices.push_back(bvn);
                normal_indices.push_back(cvn);
            }
            else if (utils::starts_with(line, "mtllib "))
            {
                load_material(directory, line.substr(7, -1));
            }
        }

        m_vertices.reserve(position_indices.size());

        for (int i = 0; i < position_indices.size(); i++)
        {
            m_vertices.push_back(Vertex{
                positions[position_indices[i] - 1],
                utils::pack_normals(normals[normal_indices[i] - 1]),
                utils::pack_texture_coords(texture_coords[texture_coords_indices[i] - 1]),
            });
        }
    }

    void Model::load_material(const std::string& directory, const std::string& file_path)
    {
        std::ifstream file(directory + file_path);
        ASSERT(file.is_open() && file.good());

        std::string line;
        while (std::getline(file, line))
        {
            if (utils::starts_with(line, "map_Kd "))
            {
                std::string file_path = directory + line.substr(7, -1);
                m_diffuse_texture = renderer::load_texture(file_path.c_str());
            }
        }
    }
}
