#include "Interactable.h"
#include "ECS.h"
#include "Components.h"
#include "Model.h"
#include "Assets.h"
#include "Player.h"
#include "Collision.h"
#include "Particles.h"

namespace kvejken
{
    namespace
    {
        std::unordered_map<WeaponType, WeaponInfo> m_weapon_infos;
        std::unordered_map<ItemType, ItemInfo> m_item_infos;
    }

    constexpr glm::vec4 INTERACT_TEXT_COLOR = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);

    void init_weapon_item_infos()
    {
        WeaponInfo& axe = m_weapon_infos[WeaponType::Axe];
        axe.range = 0.31f;
        axe.attack_time = 0.5f;
        axe.model = assets::axe.get();
        axe.model_scale = 0.5f;
        axe.model_offset = glm::vec3(0.0f, 0.0f, 0.0f);

        WeaponInfo& hammer = m_weapon_infos[WeaponType::Hammer];
        hammer.range = 0.37f;
        hammer.attack_time = 0.6f;
        hammer.model = assets::hammer.get();
        hammer.model_scale = 0.3f;
        hammer.model_offset = glm::vec3(0.0f, 0.3f, 0.0f);

        WeaponInfo& spiked_club = m_weapon_infos[WeaponType::SpikedClub];
        spiked_club.range = 0.26f;
        spiked_club.attack_time = 0.4f;
        spiked_club.model = assets::spiked_club.get();
        spiked_club.model_scale = 0.25f;
        spiked_club.model_offset = glm::vec3(0.0f, 0.2f, 0.0f);


        ItemInfo& key = m_item_infos[ItemType::Key];
        key.cost = (int)SpecialCost::Key;
        key.model = assets::key.get();
        key.model_scale = 0.3f;
        key.model_offset = glm::vec3(-0.4f, -0.4f, -1.1f);

        ItemInfo& torch = m_item_infos[ItemType::Torch];
        torch.cost = (int)SpecialCost::Torch;
        torch.model = assets::torch.get();
        torch.model_scale = 0.55f;
        torch.model_offset = glm::vec3(-0.55f, -0.8f, -1.1f);

        ItemInfo& lit_torch = m_item_infos[ItemType::LitTorch];
        lit_torch = torch;
        lit_torch.cost = 0;
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

    static void spawn_gate(glm::vec3 position, glm::quat rotation, glm::vec3 lever_pos, glm::quat lever_rot, int cost)
    {
        Gate gate;
        gate.anim_progress = 0.0f;
        gate.opened = false;
        gate.lever_pos = lever_pos;
        gate.lever_rot = lever_rot;

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
        transform.position = position + weapon.model_offset;
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
    }

    static void spawn_fireplace(glm::vec3 position)
    {
        Transform transform;
        transform.position = position;
        transform.rotation = glm::quat(1, 0, 0, 0);
        transform.scale = 1.0f;

        ParticleSpawner particles = {};
        particles.active = true;
        particles.spawn_rate = 120.0f;
        particles.min_size = 0.04f;
        particles.max_size = 0.08f;
        particles.min_time_alive = 0.5f;
        particles.max_time_alive = 0.8f;
        particles.origin = glm::vec3(0, 0, 0);
        particles.origin_radius = 0.2f;
        particles.min_velocity = 0.5f;
        particles.max_velocity = 0.75f;
        particles.velocity_offset = glm::vec3(0, 1, 0);
        particles.color_a = glm::vec3(1, 0.01f, 0);
        particles.color_b = glm::vec3(1, 0.03f, 0);
        particles.draw_layer = Layer::World;

        Interactable inter = {};
        inter.max_player_dist = 2.0f;
        inter.cost = (int)SpecialCost::Torch;

        Entity entity = ecs::create_entity();
        ecs::add_component(transform, entity);
        ecs::add_component(particles, entity);
        ecs::add_component(inter, entity);
        ecs::add_component(Fireplace{}, entity);
    }

    void spawn_interactables()
    {
        spawn_gate(glm::vec3(11.91f, 3.5f, 33.76f), glm::vec3(0, glm::radians(20.0f), 0), glm::vec3(15.03f, 4.2f, 32.38f), glm::vec3(0), 100);
        spawn_gate(glm::vec3(34.36f, 3.5f, 62.52f), glm::vec3(0, glm::radians(110.0f), 0), glm::vec3(32.46f, 4.2f, 61.53f), glm::vec3(0), 500);
        spawn_gate(glm::vec3(10.85f, 3.5f, 66.84f), glm::vec3(0, glm::radians(290.0f), 0), glm::vec3(12.85f, 4.2f, 67.94f), glm::vec3(0), 700);
        spawn_gate(glm::vec3(6.138f, -0.62f, 93.78f), glm::vec3(0, glm::radians(20.0f), 0), glm::vec3(6.4f, 0, 91.69f), glm::vec3(0, PI/2.0f, 0), (int)SpecialCost::Key);

        spawn_item(ItemType::Key, glm::vec3(43.0f, 4.0f, 54.42f), glm::vec3(0, 0, 0), 100);
        spawn_item(ItemType::Torch, glm::vec3(-3.67f, 4.0f, 71.58f), glm::vec3(0, 0, 0), 100);

        spawn_weapon(WeaponType::Axe, glm::vec3(4.85f, 4.0f, 18.38f), glm::vec3(0, PI/2.0f, 0), 0);
        spawn_weapon(WeaponType::SpikedClub, glm::vec3(11.81f, 8.0f, 60.44f), glm::vec3(0, 0, 0), 100);
        spawn_weapon(WeaponType::Hammer, glm::vec3(24.48f, -0.3f, 106.8f), glm::vec3(0, 0, 0), 100);

        spawn_fireplace(glm::vec3(-26.67f, 1.508f, 12.14f));

#ifdef KVEJKEN_TEST
        spawn_weapon(WeaponType::Axe, glm::vec3(0, 3, 0), glm::vec3(0, PI / 2.0f, 0), 100);
        spawn_weapon(WeaponType::SpikedClub, glm::vec3(1, 3, 0), glm::vec3(0, 0, 0), 100);
        spawn_weapon(WeaponType::Hammer, glm::vec3(2, 3, 0), glm::vec3(0, 0, 0), 100);
        spawn_item(ItemType::Torch, glm::vec3(3, 3, 0), glm::vec3(0, 0, 0), 100);
#endif
    }

    static void update_gates(Player& player, std::vector<Entity>& remove_interactable, float delta_time)
    {
        for (auto [gate, transform] : ecs::get_components<Gate, Transform>())
        {
            if (gate.anim_progress < 0.5f && gate.anim_progress + delta_time >= 0.5f)
                player.screen_shake += 1.0f;

            if (gate.opened)
            {
                gate.anim_progress += delta_time;
            }

            constexpr float GATE_OPEN_SPEED = 1.0f;
            constexpr float GATE_OPEN_TIME = 5.0f;
            if (gate.anim_progress > 0.5f && gate.anim_progress < GATE_OPEN_TIME)
            {
                transform.position.y += GATE_OPEN_SPEED * delta_time;
            }
        }

        for (auto [id, gate, interactable] : ecs::get_components_ids<Gate, Interactable>())
        {
            if (interactable.player_interacted)
            {
                gate.opened = true;
                remove_interactable.push_back(id);

                player.progress += 1;

                glm::quat rot = ecs::get_component<Transform>(id).rotation;
                player.forced_movement_time = 0.5f;
                player.forced_pos = gate.lever_pos + gate.lever_rot * rot * glm::vec3(0, 0, -1);
                player.forced_look_at = gate.lever_pos;
            }
            else if (interactable.player_close)
            {
                if (interactable.cost == (int)SpecialCost::Key)
                {
                    renderer::draw_text(u8"[E] odpri prehod s ključem", glm::vec2(1920 / 2, 800), 48, INTERACT_TEXT_COLOR, Align::Center);
                }
                else
                {
                    char text[64];
                    sprintf(text, u8"[E] odpri prehod za %d točk", interactable.cost);
                    renderer::draw_text(text, glm::vec2(1920 / 2, 800), 48, INTERACT_TEXT_COLOR, Align::Center);
                }
            }

            interactable.player_close = false;
            interactable.player_interacted = false;
        }
    }

    static void update_weapons(Player& player, Transform& player_transform)
    {
        static std::vector<std::tuple<WeaponType, glm::vec3>> new_spawn;
        new_spawn.clear();

        for (const auto [id, weapon, interactable] : ecs::get_components_ids<WeaponType, Interactable>())
        {
            if (interactable.player_interacted)
            {
                if (player.right_hand_item != WeaponType::None)
                {
                    new_spawn.push_back({ player.right_hand_item, player_transform.position + glm::vec3(0, -0.45f, 0) });
                }

                player.right_hand_item = weapon;
                player.right_hand_time_since_pickup = 0.0f;
                ecs::queue_destroy_entity(id);
            }
            else if (interactable.player_close)
            {
                if (interactable.cost == 0)
                {
                    renderer::draw_text(u8"[E] poberi orožje", glm::vec2(1920 / 2, 800), 48, INTERACT_TEXT_COLOR, Align::Center);
                }
                else
                {
                    char text[64];
                    sprintf(text, u8"[E] kupi orožje za %d točk", interactable.cost);
                    renderer::draw_text(text, glm::vec2(1920 / 2, 800), 48, INTERACT_TEXT_COLOR, Align::Center);
                }
            }

            interactable.player_close = false;
            interactable.player_interacted = false;
        }

        for (auto& [type, position] : new_spawn)
            spawn_weapon(type, position - get_weapon_info(type).model_offset, glm::vec3(0, 0, PI / 2.0f), 0);
    }

    static void update_items(Player& player, Transform& player_transform)
    {
        static std::vector<std::tuple<ItemType, glm::vec3>> new_spawn;
        new_spawn.clear();

        for (const auto [id, item, interactable] : ecs::get_components_ids<ItemType, Interactable>())
        {
            if (interactable.player_interacted)
            {
                if (player.left_hand_item != ItemType::None)
                {
                    if (player.left_hand_item == ItemType::LitTorch)
                        player.left_hand_item = ItemType::Torch;

                    new_spawn.push_back({ player.left_hand_item, player_transform.position + glm::vec3(0, -0.45f, 0) });
                }

                player.left_hand_item = item;
                player.left_hand_time_since_pickup = 0.0f;
                ecs::queue_destroy_entity(id);
            }
            else if (interactable.player_close)
            {
                const char* item_name;
                if (item == ItemType::Key) item_name = u8"ključ";
                else if (item == ItemType::Torch || item == ItemType::LitTorch) item_name = u8"baklo";
                else if (item == ItemType::Skull) item_name = u8"lobanjo";
                else ERROR_EXIT("no item_name found");

                if (interactable.cost == 0)
                {
                    char text[64];
                    sprintf(text, u8"[E] vzemi %s", item_name);
                    renderer::draw_text(text, glm::vec2(1920 / 2, 800), 48, INTERACT_TEXT_COLOR, Align::Center);
                }
                else
                {
                    char text[64];
                    sprintf(text, u8"[E] kupi %s za %d točk", item_name, interactable.cost);
                    renderer::draw_text(text, glm::vec2(1920 / 2, 800), 48, INTERACT_TEXT_COLOR, Align::Center);
                }
            }

            interactable.player_close = false;
            interactable.player_interacted = false;
        }

        for (auto& [type, position] : new_spawn)
            spawn_item(type, position, glm::vec3(0, 0, PI / 2.0f), 0);
    }

    static void update_fireplace(Player& player)
    {
        for (auto [fireplace, interactable] : ecs::get_components<Fireplace, Interactable>())
        {
            if (interactable.player_interacted)
            {
                player.left_hand_item = ItemType::LitTorch;
                player.left_hand_time_since_pickup = 0.0f;
            }
            else if (interactable.player_close && player.left_hand_item == ItemType::Torch)
            {
                renderer::draw_text(u8"[E] prižgi baklo", glm::vec2(1920 / 2, 800), 48, INTERACT_TEXT_COLOR, Align::Center);
            }

            interactable.player_close = false;
            interactable.player_interacted = false;
        }
    }

    void update_interactables(float delta_time, float game_time)
    {
        static std::vector<Entity> remove_interactable;
        remove_interactable.clear();

        auto [player, player_transform] = *ecs::get_components<Player, Transform>().begin();

        update_gates(player, remove_interactable, delta_time);
        update_weapons(player, player_transform);
        update_items(player, player_transform);
        update_fireplace(player);

        for (const auto& entity : remove_interactable)
        {
            ecs::remove_component<Interactable>(entity);
        }
    }

    void draw_levers()
    {
        auto& player = *ecs::get_components<Player>().begin();

        for (auto [id, gate, transform] : ecs::get_components_ids<Gate, Transform>())
        {
            if (!gate.opened)
            {
                auto& interactable = ecs::get_component<Interactable>(id);
                if (interactable.cost >= 0 && player.points < interactable.cost)
                    continue;
                if (interactable.cost < 0 && (player.left_hand_item == ItemType::None || get_item_info(player.left_hand_item).cost != interactable.cost))
                    continue;
            }

            int frame = (int)(gate.anim_progress / 0.025f);
            if (frame >= assets::lever_anim.size())
                frame = assets::lever_anim.size() - 1;

            glm::quat rot = gate.lever_rot * glm::angleAxis(PI / 2.0f, glm::vec3(0, 1, 0)) * transform.rotation;

            renderer::draw_model(&assets::lever_anim[frame], gate.lever_pos, rot, glm::vec3(0.35f));

            if (gate.opened && gate.anim_progress <= 0.5f)
                renderer::draw_model(&assets::lever_hand_anim[frame], gate.lever_pos, rot, glm::vec3(0.35f));
        }
    }
}
