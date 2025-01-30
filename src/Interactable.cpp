#include "Interactable.h"
#include "ECS.h"
#include "Components.h"
#include "Model.h"
#include "Assets.h"

namespace kvejken
{
    static void spawn_gate(glm::vec3 position, glm::quat rotation, int cost)
    {
        Gate gate;
        gate.anim_progress = 0.0f;
        gate.opened = false;

        Interactable inter;
        inter.max_player_dist = 5.0f;
        inter.cost = cost;
        inter.enabled = true;
        inter.player_close = false;
        inter.player_interacted = false;

        Transform transform;
        transform.position = position;
        transform.rotation = rotation;
        transform.scale = 0.13f;

        Model* model = assets::gate.get();

        Entity e = ecs::create_entity();
        ecs::add_component(gate, e);
        ecs::add_component(inter, e);
        ecs::add_component(transform, e);
        ecs::add_component(model, e);
    }

    void spawn_gates()
    {
        spawn_gate(glm::vec3(11.91f, 3.22f, 33.76f), glm::vec3(0, glm::radians(20.0f), 0), 100);
        spawn_gate(glm::vec3(34.36f, 3.22f, 62.52f), glm::vec3(0, glm::radians(110.0f), 0), 100);
        spawn_gate(glm::vec3(10.85f, 3.22f, 66.84f), glm::vec3(0, glm::radians(110.0f), 0), 100);
        spawn_gate(glm::vec3(6.138f, -0.9f, 93.78f), glm::vec3(0, glm::radians(20.0f), 0), 100);
    }

    void update_interactables(float delta_time, float game_time)
    {
        for (auto [gate, interactable, transform] : ecs::get_components<Gate, Interactable, Transform>())
        {
            if (gate.opened)
            {
                gate.anim_progress += delta_time;

                constexpr float GATE_OPEN_SPEED = 1.0f;
                constexpr float GATE_OPEN_TIME = 2.0f;
                if (gate.anim_progress > GATE_OPEN_TIME)
                    gate.anim_progress = GATE_OPEN_TIME;
                transform.position.y += GATE_OPEN_SPEED * delta_time;
            }
            else if (interactable.player_interacted)
            {
                gate.opened = true;
                interactable.enabled = false;
            }
            else if (interactable.player_close)
            {
                // TODO: napisi na zaslon: odpri prehod za x tock [E]
            }

            interactable.player_close = false;
            interactable.player_interacted = false;
        }
    }
}
