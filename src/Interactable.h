#pragma once

namespace kvejken
{
    struct Interactable
    {
        float max_player_dist;
        int cost;

        bool player_close;
        bool player_interacted;
    };

    struct Gate
    {
        bool opened;
        float anim_progress;
    };

    void spawn_gates();
    void update_interactables(float delta_time, float game_time);
}
