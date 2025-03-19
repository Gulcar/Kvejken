#pragma once
#include <glm/vec3.hpp>
#include "Model.h"

namespace kvejken
{
    struct Interactable
    {
        float max_player_dist;
        int cost; // tocke ali SpecialCost

        bool player_close;
        bool player_interacted;
    };

    enum class SpecialCost
    {
        Key = -999,
        Torch,
        Skull,
    };

    struct Gate
    {
        bool opened;
        float anim_progress;
    };

    enum class WeaponType : uint8_t
    {
        None,
        Axe,
        Hammer,
        SpikedClub,
        Fireball,
        Crossbow,
    };

    enum class ItemType : uint8_t
    {
        None,
        Key,
        Torch,
        LitTorch,
        Skull,
    };

    struct WeaponInfo
    {
        float range;
        float attack_time;

        Model* model;
        float model_scale;
        glm::vec3 model_offset;
    };

    struct ItemInfo
    {
        int cost; // ce se lahko uporabi za odpiranje vrat npr je SpecialCost

        Model* model;
        float model_scale;
        glm::vec3 model_offset;
    };

    struct Fireplace {};

    void init_weapon_item_infos();
    const WeaponInfo& get_weapon_info(WeaponType type);
    const ItemInfo& get_item_info(ItemType type);

    void spawn_interactables();
    void update_interactables(float delta_time, float game_time);
}
