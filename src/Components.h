#pragma once
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace kvejken
{
    struct Transform
    {
        glm::vec3 position;
        float scale;
        glm::quat rotation;
    };

    struct Player
    {
        int health;
        bool local;
    };
}

