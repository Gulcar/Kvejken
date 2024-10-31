#include "Model.h"
#include "Utils.h"
#include <fstream>
#include <string>

namespace kvejken
{
    Model::Model(std::string_view file_name)
    {
        std::ifstream file(file_name.data());
        ASSERT(file.is_open() && file.good());

        std::string line;
        while (std::getline(file, line))
        {
            if (line.length() == 0 || line[0] == '#')
                continue;

            if (utils::starts_with(line, "v "))
            {
                float x, y, z;
                ASSERT(sscanf(line.c_str(), "v %f %f %f", &x, &y, &z) == 3);
                m_vertices.push_back({ x, y, z });
            }
            else if (utils::starts_with(line, "vn "))
            {
                float x, y, z;
                ASSERT(sscanf(line.c_str(), "vn %f %f %f", &x, &y, &z) == 3);
                m_normals.push_back({ x, y, z });
            }
            else if (utils::starts_with(line, "vt "))
            {
                float x, y, z;
                ASSERT(sscanf(line.c_str(), "vt %f %f", &x, &y) == 2);
                m_texture_coords.push_back({ x, y });
            }
            else if (utils::starts_with(line, "f "))
            {
                int av, avt, avn;
                int bv, bvt, bvn;
                int cv, cvt, cvn;
                ASSERT(sscanf(line.c_str(), "f %i/%i/%i %i/%i/%i %i/%i/%i",
                    &av, &avt, &avn, &bv, &bvt, &bvn, &cv, &cvt, &cvn) == 9);
                m_indices.push_back(av);
                m_indices.push_back(bv);
                m_indices.push_back(cv);
                m_texture_coords_indices.push_back(avt);
                m_texture_coords_indices.push_back(bvt);
                m_texture_coords_indices.push_back(cvt);
                m_normal_indices.push_back(avn);
                m_normal_indices.push_back(bvn);
                m_normal_indices.push_back(cvn);
            }
        }
    }
}
