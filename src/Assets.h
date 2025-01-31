#pragma once
#include "Model.h"
#include <memory>
#include <vector>

namespace kvejken::assets
{
    inline std::unique_ptr<Model> terrain;
    inline std::unique_ptr<Model> gate;

    inline std::vector<Model> eel_anim;

    inline std::unique_ptr<Model> axe;
    inline std::unique_ptr<Model> hammer;
    inline std::unique_ptr<Model> spiked_club;

    inline std::unique_ptr<Model> key;

    inline std::unique_ptr<Model> test_cube;
    inline std::unique_ptr<Model> test_rock;
    inline std::unique_ptr<Model> test_multiple;

    inline void load()
    {
        terrain = std::make_unique<Model>("assets/environment/terrain.obj", false);
        gate = std::make_unique<Model>("assets/environment/gate.obj");
        
        for (int i = 1; i <= 12; i++)
            eel_anim.emplace_back("assets/enemies/eel" + std::to_string(i) + ".obj");

        axe = std::make_unique<Model>("assets/weapons/axe.obj");
        hammer = std::make_unique<Model>("assets/weapons/hammer.obj");
        spiked_club = std::make_unique<Model>("assets/weapons/spiked_club.obj");

        key = std::make_unique<Model>("assets/items/key.obj");

        test_cube = std::make_unique<Model>("assets/test/test_cube.obj");
        test_rock = std::make_unique<Model>("assets/test/test_rock.obj");
        test_multiple = std::make_unique<Model>("assets/test/test_multiple.obj");
    }

    inline void unload()
    {
        terrain.reset();
        gate.reset();

        eel_anim.clear();
        
        axe.reset();
        hammer.reset();
        spiked_club.reset();

        key.reset();

        test_cube.reset();
        test_rock.reset();
        test_multiple.reset();
    }
}
