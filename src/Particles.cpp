#include "Particles.h"
#include "ECS.h"
#include "Renderer.h"
#include "Model.h"
#include "Assets.h"
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
    }

    void draw_particles()
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
                renderer::draw_model(assets::particle.get(), transform, Layer::World, glm::vec4(particle.color, 1.0f));
            }
        }
    }
}
