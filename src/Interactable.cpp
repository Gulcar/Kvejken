#include "Interactable.h"
#include "ECS.h"
#include "Components.h"
#include "Model.h"
#include "Assets.h"
#include "Player.h"
#include "Collision.h"

namespace kvejken
{
    namespace
    {
        std::unordered_map<WeaponType, WeaponInfo> m_weapon_infos;
        std::unordered_map<ItemType, ItemInfo> m_item_infos;
    }

    void init_weapon_item_infos()
    {
        WeaponInfo& axe = m_weapon_infos[WeaponType::Axe];
        axe.range = 2.0f;
        axe.attack_time = 0.5f;
        axe.model = assets::axe.get();
        axe.model_scale = 0.3f;
        axe.model_offset = glm::vec3(0.3f, -0.5f, -0.5f);

        WeaponInfo& hammer = m_weapon_infos[WeaponType::Hammer];
        hammer.range = 2.4f;
        hammer.attack_time = 0.6f;
        hammer.model = assets::hammer.get();
        hammer.model_scale = 0.2f;
        hammer.model_offset = glm::vec3(0.35f, -0.4f, -0.5f);

        WeaponInfo& spiked_club = m_weapon_infos[WeaponType::SpikedClub];
        spiked_club.range = 1.66f;
        spiked_club.attack_time = 0.4f;
        spiked_club.model = assets::spiked_club.get();
        spiked_club.model_scale = 0.15f;
        spiked_club.model_offset = glm::vec3(0.3f, -0.35f, -0.5f);


        ItemInfo& key = m_item_infos[ItemType::Key];
        key.cost = COST_KEY;
        key.model = assets::key.get();
        key.model_scale = 0.3f;
        key.model_offset = glm::vec3(-0.4f, -0.4f, -1.1f);

        ItemInfo& torch = m_item_infos[ItemType::Torch];
        torch.cost = 0;
        torch.model = assets::torch.get();
        torch.model_scale = 0.55f;
        torch.model_offset = glm::vec3(-0.55f, -0.8f, -1.1f);
    }

    const WeaponInfo& get_weapon_info(WeaponType type)
    {
        auto it = m_weapon_infos.find(type);
        ASSERT(it != m_weapon_infos.end());
        return it->second;
    }

    const ItemInfo& get_item_info(ItemType type)
    {
        auto it = m_item_infos.find(type);
        ASSERT(it != m_item_infos.end());
        return it->second;
    }

    static void spawn_gate(glm::vec3 position, glm::quat rotation, int cost)
    {
        Gate gate;
        gate.anim_progress = 0.0f;
        gate.opened = false;

        Interactable inter = {};
        inter.max_player_dist = 2.5f;
        inter.cost = cost;

        Transform transform;
        transform.position = position;
        transform.rotation = rotation;
        transform.scale = 0.13f;

        Model* model = assets::gate.get();

        collision::RectCollider collider;
        collider.center_offset = glm::vec3(0, 2, 0);
        collider.width = 4.0f;
        collider.height = 5.8f;

        Entity e = ecs::create_entity();
        ecs::add_component(gate, e);
        ecs::add_component(inter, e);
        ecs::add_component(transform, e);
        ecs::add_component(model, e);
        ecs::add_component(collider, e);
    }

    static void spawn_weapon(WeaponType weapon_tag, glm::vec3 position, glm::quat rotation, int cost)
    {
        const WeaponInfo& weapon = get_weapon_info(weapon_tag);

        Interactable inter = {};
        inter.max_player_dist = 1.5f;
        inter.cost = cost;

        Transform transform;
        transform.position = position;
        transform.rotation = rotation;
        transform.scale = weapon.model_scale;

        Entity e = ecs::create_entity();
        ecs::add_component(inter, e);
        ecs::add_component(weapon.model, e);
        ecs::add_component(transform, e);
        ecs::add_component(weapon_tag, e);
    }

    static void spawn_item(ItemType item_tag, glm::vec3 position, glm::quat rotation, int cost)
    {
        const ItemInfo& item = get_item_info(item_tag);

        Interactable inter = {};
        inter.max_player_dist = 1.5f;
        inter.cost = cost;

        Transform transform;
        transform.position = position;
        transform.rotation = rotation;
        transform.scale = item.model_scale;

        Entity e = ecs::create_entity();
        ecs::add_component(inter, e);
        ecs::add_component(item.model, e);
        ecs::add_component(transform, e);
        ecs::add_component(item_tag, e);

        if (item_tag == ItemType::Torch)
        {
            PointLight light;
            light.offset = glm::vec3(0, 0.8f, 0);
            light.color = glm::vec3(1.0f, 0.5f, 0.5f);
            light.strength = 10.0f;
            ecs::add_component(light, e);
        }
    }

    void spawn_interactables()
    {
        spawn_gate(glm::vec3(11.91f, 3.5f, 33.76f), glm::vec3(0, glm::radians(20.0f), 0), 100);
        spawn_gate(glm::vec3(34.36f, 3.5f, 62.52f), glm::vec3(0, glm::radians(110.0f), 0), 500);
        spawn_gate(glm::vec3(10.85f, 3.5f, 66.84f), glm::vec3(0, glm::radians(110.0f), 0), 700);
        spawn_gate(glm::vec3(6.138f, -0.62f, 93.78f), glm::vec3(0, glm::radians(20.0f), 0), COST_KEY);

        spawn_item(ItemType::Key, glm::vec3(43.0f, 4.0f, 54.42f), glm::vec3(0, 0, 0), 100);
        spawn_item(ItemType::Torch, glm::vec3(-3.67f, 4.0f, 71.58f), glm::vec3(0, 0, 0), 100);

        spawn_weapon(WeaponType::Axe, glm::vec3(4.85f, 4.0f, 18.38f), glm::vec3(0, PI/2.0f, 0), 100);
        spawn_weapon(WeaponType::SpikedClub, glm::vec3(11.81f, 8.0f, 60.44f), glm::vec3(0, 0, 0), 100);
        spawn_weapon(WeaponType::Hammer, glm::vec3(24.48f, -0.3f, 106.8f), glm::vec3(0, 0, 0), 100);
    }

    static void update_gates(std::vector<Entity>& remove_interactable, float delta_time)
    {
        for (auto [gate, transform] : ecs::get_components<Gate, Transform>())
        {
            constexpr float GATE_OPEN_SPEED = 1.0f;
            constexpr float GATE_OPEN_TIME = 4.5f;
            if (gate.opened && gate.anim_progress < GATE_OPEN_TIME)
            {
                gate.anim_progress += delta_time;
                transform.position.y += GATE_OPEN_SPEED * delta_time;
            }
        }

        for (auto [id, gate, interactable] : ecs::get_components_ids<Gate, Interactable>())
        {
            if (interactable.player_interacted)
            {
                gate.opened = true;
                remove_interactable.push_back(id);
                ecs::get_components<Player>().begin()->progress += 1;
            }
            else if (interactable.player_close)
            {
                // TODO: napisi na zaslon: odpri prehod za x tock [E]
            }

            interactable.player_close = false;
            interactable.player_interacted = false;
        }
    }

    static void update_weapons()
    {
        std::vector<std::tuple<WeaponType, glm::vec3>> new_spawn;
        new_spawn.clear();

        for (const auto [id, weapon, interactable] : ecs::get_components_ids<WeaponType, Interactable>())
        {
            if (interactable.player_interacted)
            {
                auto [player, player_transform] = *ecs::get_components<Player, Transform>().begin();
                if (player.right_hand_item != WeaponType::None)
                {
                    new_spawn.push_back({ player.right_hand_item, player_transform.position + glm::vec3(0, -0.45f, 0) });
                }

                player.right_hand_item = weapon;
                ecs::queue_destroy_entity(id);
            }
            else if (interactable.player_close)
            {
                // TODO: napisi na zaslon: kupi predmet za x tock [E] razen ce zastonj ker sem ga ze kupil in dropal
            }

            interactable.player_close = false;
            interactable.player_interacted = false;
        }

        for (auto& [type, position] : new_spawn)
            spawn_weapon(type, position, glm::vec3(0, 0, PI / 2.0f), 0);
    }

    static void update_items()
    {
        std::vector<std::tuple<ItemType, glm::vec3>> new_spawn;
        new_spawn.clear();

        for (const auto [id, item, interactable] : ecs::get_components_ids<ItemType, Interactable>())
        {
            if (interactable.player_interacted)
            {
                auto [player, player_transform] = *ecs::get_components<Player, Transform>().begin();
                if (player.left_hand_item != ItemType::None)
                {
                    new_spawn.push_back({ player.left_hand_item, player_transform.position + glm::vec3(0, -0.45f, 0) });
                }

                player.left_hand_item = item;
                ecs::queue_destroy_entity(id);
            }
            else if (interactable.player_close)
            {
                // TODO: napisi na zaslon: kupi predmet za x tock [E] razen ce zastonj ker sem ga ze kupil in dropal
            }

            interactable.player_close = false;
            interactable.player_interacted = false;
        }

        for (auto& [type, position] : new_spawn)
            spawn_item(type, position, glm::vec3(0, 0, PI / 2.0f), 0);
    }

    void update_interactables(float delta_time, float game_time)
    {
        static std::vector<Entity> remove_interactable;
        remove_interactable.clear();

        update_gates(remove_interactable, delta_time);
        update_weapons();
        update_items();

        for (const auto& entity : remove_interactable)
        {
            ecs::remove_component<Interactable>(entity);
        }
    }
}
