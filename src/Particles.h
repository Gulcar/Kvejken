﻿#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include "Renderer.h"

namespace kvejken
{
    struct Particle
    {
        float size;
        float time_alive;
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 color;
    };

    struct ParticleExplosion
    {
        std::vector<Particle> particles;
        float time;
        float max_time;
        Layer draw_layer;
    };

    struct ParticleExplosionParameters
    {
        int min_count, max_count;
        float min_size, max_size;
        float min_time_alive, max_time_alive;
        glm::vec3 origin;
        float min_velocity, max_velocity;
        glm::vec3 velocity_offset;
        glm::vec3 color_a, color_b;
        Layer draw_layer;
    };

    struct ParticleSpawner
    {
        std::vector<Particle> particles;
        bool active;
        float spawn_rate;
        float time;
        int next_index;

        float min_size, max_size;
        float min_time_alive, max_time_alive;
        glm::vec3 origin;
        float origin_radius;
        float min_velocity, max_velocity;
        glm::vec3 velocity_offset;
        glm::vec3 color_a, color_b;
        Layer draw_layer;
    };

    void spawn_particle_explosion(const ParticleExplosionParameters& params);

    void update_particles(float delta_time, float game_time);

    void draw_particles(float game_time);
}

