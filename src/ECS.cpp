#include "ECS.h"
#include <vector>
#include <iostream>
#include <bitset>
#include <unordered_map>
#include <unordered_set>

namespace kvejken::ecs
{
    std::vector<IComponentPool*> m_component_pools;
    std::unordered_map<Entity, std::bitset<32>> m_signatures;
    std::unordered_set<Entity> m_to_destroy;

    std::vector<IComponentPool*>& component_pools()
    {
        return m_component_pools;
    }

    std::unordered_map<Entity, std::bitset<32>>& signatures()
    {
        return m_signatures;
    }

    std::unordered_set<Entity>& to_destroy()
    {
        return m_to_destroy;
    }
}
