#pragma once
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace kvejken
{
    struct Transform
    {
        glm::vec3 position;
        glm::quat rotation;
        float scale;
    };

    struct Player
    {
        int health;
        bool local;
        float velocity_y;
        glm::vec2 move_velocity;
        float jump_allowed_time;
    };
}

