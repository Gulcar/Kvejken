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

        glm::vec2 move_velocity;
        float velocity_y;
        float jump_allowed_time;

        float look_yaw;
        float look_pitch;
    };
}

