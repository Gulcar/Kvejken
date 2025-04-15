#include "Particles.h"
#include "ECS.h"
#include "Renderer.h"
#include "Model.h"
#include "Assets.h"
#include "Components.h"
#include <glm/gtc/random.hpp>
#include <memory>

namespace kvejken
{
    void spawn_particle_explosion(const ParticleExplosionParameters& params)
    {
        Entity e = ecs::create_entity();
        ecs::add_component(ParticleExplosion{}, e);

        ParticleExplosion& pe = ecs::get_component<ParticleExplosion>(e);
        pe.time = 0.0f;
        pe.max_time = params.max_time_alive;
        pe.draw_layer = params.draw_layer;

        int count = utils::rand(params.min_count, params.max_count);
        pe.particles.resize(count);

        for (int i = 0; i < count; i++)
        {
            pe.particles[i].size = utils::randf(params.min_size, params.max_size);
            pe.particles[i].time_alive = utils::randf(params.min_time_alive, params.max_time_alive);
            pe.particles[i].position = params.origin;
            pe.particles[i].velocity = glm::sphericalRand(utils::randf(params.min_velocity, params.max_velocity)) + params.velocity_offset;
            pe.particles[i].color = glm::mix(params.color_a, params.color_b, utils::randf(0.0f, 1.0f));
        }
    }

    void update_particles(float delta_time, float game_time)
    {
        for (auto [id, particle_explosion] : ecs::get_components_ids<ParticleExplosion>())
        {
            particle_explosion.time += delta_time;

            if (particle_explosion.time > particle_explosion.max_time)
            {
                ecs::queue_destroy_entity(id);
                continue;
            }

            for (auto& particle : particle_explosion.particles)
            {
                particle.position += particle.velocity * delta_time;
            }
        }

        for (auto& [spawner, transform] : ecs::get_components<ParticleSpawner, Transform>())
        {
            if (spawner.active)
                spawner.time += delta_time;

            if (spawner.time >= 1.0f / spawner.spawn_rate)
            {
                spawner.time = 0.0f;

                Particle p;
                p.size = utils::randf(spawner.min_size, spawner.max_size);
                p.time_alive = game_time + utils::randf(spawner.min_time_alive, spawner.max_time_alive);
                p.position = spawner.origin + transform.position;
                if (spawner.origin_radius > 0.0f) p.position += glm::ballRand(spawner.origin_radius);
                p.velocity = glm::sphericalRand(utils::randf(spawner.min_velocity, spawner.max_velocity)) + spawner.velocity_offset;
                p.color = glm::mix(spawner.color_a, spawner.color_b, utils::randf(0.0f, 1.0f));

                if (spawner.next_index < spawner.particles.size())
                {
                    spawner.particles[spawner.next_index] = p;
                    spawner.next_index += 1;
                }
                else if (spawner.particles.size() > 0 && spawner.particles[0].time_alive < game_time)
                {
                    spawner.particles[0] = p;
                    spawner.next_index = 1;
                }
                else
                {
                    spawner.particles.push_back(p);
                    spawner.next_index += spawner.particles.size();
                }
            }

            for (auto& particle : spawner.particles)
            {
                particle.position += particle.velocity * delta_time;
            }
        }
    }

    void draw_particles(float game_time)
    {
        for (auto& particle_explosion : ecs::get_components<ParticleExplosion>())
        {
            for (auto& particle : particle_explosion.particles)
            {
                float t = particle_explosion.time / particle.time_alive;
                if (t > 1.0f)
                    continue;
                t = t * t;

                glm::mat4 transform(1.0f);
                transform = glm::translate(transform, particle.position);
                transform = glm::scale(transform, glm::vec3(particle.size * (1.0f - t)));
                renderer::draw_model(assets::particle.get(), transform, particle_explosion.draw_layer, glm::vec4(particle.color, 1.0f));
            }
        }

        for (auto& particle_spawner : ecs::get_components<ParticleSpawner>())
        {
            for (auto& particle : particle_spawner.particles)
            {
                if (particle.time_alive < game_time)
                    continue;
                float t = 1.0f - (particle.time_alive - game_time) / particle_spawner.max_time_alive;
                t = t * t;

                glm::mat4 transform(1.0f);
                transform = glm::translate(transform, particle.position);
                transform = glm::scale(transform, glm::vec3(particle.size * (1.0f - t)));
                renderer::draw_model(assets::particle.get(), transform, particle_spawner.draw_layer, glm::vec4(particle.color, 1.0f));
            }
        }
    }
}

