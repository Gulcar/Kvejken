#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string_view>
#include <stdint.h>
#include <fstream>
#include <cstring>
#include <random>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/common.hpp>

#ifdef WIN32
#define DEBUG_BREAK() __debugbreak()
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif
#include <signal.h>

#define ERROR_EXIT(...) { fprintf(stderr, "ERORR: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "!\n"); DEBUG_BREAK(); exit(1); }
#define ASSERT(x) { if (!(x)) { fprintf(stderr, "ERROR assert failed: " __FILE__ " line %d!\n", __LINE__); DEBUG_BREAK(); exit(1); } }

constexpr float PI = 3.14159265358979323846f;

namespace kvejken::utils
{
    inline bool starts_with(std::string_view str, std::string_view check)
    {
        if (str.length() < check.length())
            return false;
        return strncmp(str.data(), check.data(), check.size()) == 0;
    }

    inline bool ends_with(std::string_view str, std::string_view check)
    {
        if (str.length() < check.length())
            return false;
        const char* str_end = str.data() + (str.length() - check.length());
        return strncmp(str_end, check.data(), check.size()) == 0;
    }

    inline std::string read_file_to_string(const char* file_path)
    {
        std::ifstream file(file_path);
        if (!file.is_open() || !file.good())
            file = std::ifstream("../../" + std::string(file_path));
        ASSERT(file.is_open() && file.good());

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        std::string str(size, '\0');

        file.seekg(0);
        file.read(&str[0], size);
        file.close();
        return str;
    }

    // GL_INT_2_10_10_10_REV
    inline uint32_t pack_vec3_to_32b(glm::vec3 v)
    {
        const uint32_t xs = v.x < 0;
        const uint32_t ys = v.y < 0;
        const uint32_t zs = v.z < 0;
        uint32_t vi =
            zs << 29 | ((uint32_t)(v.z * 511 + (zs << 9)) & 511) << 20 |
            ys << 19 | ((uint32_t)(v.y * 511 + (ys << 9)) & 511) << 10 |
            xs << 9 | ((uint32_t)(v.x * 511 + (xs << 9)) & 511);
        return vi;
    }

    // normalized for range [-1, 2]
    inline glm::u16vec2 pack_texture_coords(glm::vec2 v)
    {
        float ux = ((v.x + 1.0f) / 3.0f) * ((1 << 16) - 1);
        float uy = ((v.y + 1.0f) / 3.0f) * ((1 << 16) - 1);
        return glm::u16vec2((uint16_t)ux, (uint16_t)uy);
    }

    inline void wrap_texture_coords(glm::vec2* a, glm::vec2* b, glm::vec2* c)
    {
        if (a->x >= 2.0f || b->x >= 2.0f || c->x >= 2.0f)
        {
            a->x -= 1.0f;
            b->x -= 1.0f;
            c->x -= 1.0f;
        }
        else if (a->x <= -1.0f || b->x <= -1.0f || c->x <= -1.0f)
        {
            a->x += 1.0f;
            b->x += 1.0f;
            c->x += 1.0f;
        }
        if (a->y >= 2.0f || b->y >= 2.0f || c->y >= 2.0f)
        {
            a->y -= 1.0f;
            b->y -= 1.0f;
            c->y -= 1.0f;
        }
        else if (a->y <= -1.0f || b->y <= -1.0f || c->y <= -1.0f)
        {
            a->y += 1.0f;
            b->y += 1.0f;
            c->y += 1.0f;
        }
    }

    inline int rand(int min, int max)
    {
        static std::mt19937 generator;
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(generator);
    }

    inline float randf(float min, float max)
    {
        static std::mt19937 generator;
        std::uniform_real_distribution<float> distribution(min, max);
        return distribution(generator);
    }
}
