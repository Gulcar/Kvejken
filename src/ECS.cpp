#include "ECS.h"
#include <vector>
#include <iostream>

namespace kvejken
{
    std::vector<IComponentPool*> m_component_pools;

    std::vector<IComponentPool*>& component_pools()
    {
        return m_component_pools;
    }
}
