#pragma once
#include "Model.h"
#include <memory>
#include <vector>

namespace kvejken::assets
{
    inline std::unique_ptr<Model> terrain;

    inline std::vector<Model> eel_anim;

    inline std::unique_ptr<Model> axe;

    inline std::unique_ptr<Model> test_cube;
    inline std::unique_ptr<Model> test_rock;
    inline std::unique_ptr<Model> test_multiple;

    inline void load()
    {
        terrain = std::make_unique<Model>("assets/environment/terrain.obj", false);
        
        for (int i = 1; i <= 12; i++)
            eel_anim.emplace_back("assets/enemies/eel" + std::to_string(i) + ".obj");

        axe = std::make_unique<Model>("assets/weapons/axe.obj");

        test_cube = std::make_unique<Model>("assets/test/test_cube.obj");
        test_rock = std::make_unique<Model>("assets/test/test_rock.obj");
        test_multiple = std::make_unique<Model>("assets/test/test_multiple.obj");
    }

    inline void unload()
    {
        terrain.reset();

        eel_anim.clear();
        
        axe.reset();

        test_cube.reset();
        test_rock.reset();
        test_multiple.reset();
    }
}
