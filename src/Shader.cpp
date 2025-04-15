#include "Shader.h"
#include <glad/glad.h>
#include "Utils.h"

namespace kvejken
{
    static uint32_t load_shader(const char* source, GLenum type)
    {
        uint32_t shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, 0);
        glCompileShader(shader);
        return shader;
    }

    Shader::Shader()
    {
        m_id = (uint32_t)(-1);
    }

    Shader::Shader(const char* vertex_src_file, const char* fragment_src_file)
    {
        std::string vertex_src = utils::read_file_to_string(vertex_src_file);
        std::string fragment_src = utils::read_file_to_string(fragment_src_file);

        uint32_t vertex_shader = load_shader(vertex_src.c_str(), GL_VERTEX_SHADER);
        uint32_t fragment_shader = load_shader(fragment_src.c_str(), GL_FRAGMENT_SHADER);

        uint32_t program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        int link_status;
        glGetProgramiv(program, GL_LINK_STATUS, &link_status);
        if (!link_status)
        {
            int log_length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
            char* log = new char[log_length + 1];
            glGetProgramInfoLog(program, log_length + 1, 0, log);
            ERROR_EXIT("failed to link shader program:\n%s", log);
        }

        // da jih bo deletalo ko bo deletan program
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        m_id = program;
    }

    Shader::Shader(Shader&& other) noexcept
    {
        m_id = other.m_id;
        m_locations = std::move(other.m_locations);
        other.m_id = (uint32_t)(-1);
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        m_id = other.m_id;
        m_locations = std::move(other.m_locations);
        other.m_id = (uint32_t)(-1);
        return *this;
    }

    Shader::~Shader()
    {
        if (m_id != (uint32_t)(-1))
        {
            glDeleteProgram(m_id);
        }
    }

    void Shader::set_uniform(const char* name, float value)
    {
        glUniform1f(get_loc(name), value);
    }
    void Shader::set_uniform(const char* name, int value)
    {
        glUniform1i(get_loc(name), value);
    }
    void Shader::set_uniform(const char* name, glm::vec3 value)
    {
        glUniform3f(get_loc(name), value.x, value.y, value.z);
    }
    void Shader::set_uniform(const char* name, const glm::mat4& value)
    {
        glUniformMatrix4fv(get_loc(name), 1, GL_FALSE, &value[0][0]);
    }

    void Shader::set_uniform(const char* name, const float* values, size_t count)
    {
        glUniform1fv(get_loc(name), count, values);
    }
    void Shader::set_uniform(const char* name, const int* values, size_t count)
    {
        glUniform1iv(get_loc(name), count, values);
    }
    void Shader::set_uniform(const char* name, const glm::vec3* values, size_t count)
    {
        glUniform3fv(get_loc(name), count, (float*)values);
    }
    void Shader::set_uniform(const char* name, const glm::mat4* values, size_t count)
    {
        glUniformMatrix4fv(get_loc(name), count, GL_FALSE, (float*)values);
    }

    int Shader::get_loc(const char* name)
    {
        auto it = m_locations.find(name);
        if (it != m_locations.end())
            return it->second;

        int loc = glGetUniformLocation(m_id, name);
        ASSERT(loc != -1);
        m_locations[name] = loc;
        return loc;
    }
}

