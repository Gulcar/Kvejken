#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <stdint.h>
#include <map>
#include <string>

namespace kvejken
{
    class Shader
    {
    public:
        Shader();
        Shader(const char* vertex_src_file, const char* fragment_src_file);

        Shader(const Shader& other) = delete;
        Shader& operator=(const Shader& other) = delete;
        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;

        ~Shader();

        void set_uniform(const char* name, float value);
        void set_uniform(const char* name, int value);
        void set_uniform(const char* name, glm::vec3 value);
        void set_uniform(const char* name, const glm::mat4& value);

        void set_uniform(const char* name, const float* values, size_t count);
        void set_uniform(const char* name, const int* values, size_t count);
        void set_uniform(const char* name, const glm::vec3* values, size_t count);
        void set_uniform(const char* name, const glm::mat4* values, size_t count);

        uint32_t id() { return m_id; }

    private:
        int get_loc(const char* name);

        uint32_t m_id;
        std::map<std::string, int> m_locations;
    };
}
