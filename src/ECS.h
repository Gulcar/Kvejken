#pragma once
#include "Utils.h"
#include <vector>

namespace kvejken
{
    using Entity = uint32_t;

    class IComponentPool {};

    template<typename T>
    class ComponentPool : IComponentPool
    {
    public:
        ComponentPool() {}
        ComponentPool(const ComponentPool& other) = delete;
        ComponentPool& operator=(const ComponentPool& other) = delete;

        void add_component(const T& comp, Entity entity)
        {
            uint32_t index = m_components.size();
            m_components.push_back(comp);
            m_index_to_entity.push_back(entity);
            auto result = m_entity_to_index.insert({ entity, index });
            ASSERT(result.second); // assert da se ni obstajal
        }

        T& get_component(Entity entity)
        {
            auto it = m_entity_to_index.find(entity);
            ASSERT(it != m_entity_to_index.end());
            return m_components[it->second];
        }

        void remove_component(Entity entity)
        {
            auto it = m_entity_to_index.find(entity);
            ASSERT(it != m_entity_to_index.end());
            uint32_t index = it->second;

            Entity last = m_index_to_entity[m_components.size() - 1];
            m_entity_to_index[last] = index;
            m_entity_to_index.erase(entity);

            m_components[index] = m_components.back();
            m_components.pop_back();
            m_index_to_entity.pop_back();
        }

        typename std::vector<T>::iterator begin() { return m_components.begin(); }
        typename std::vector<T>::iterator end() { return m_components.end(); }

    private:
        std::unordered_map<Entity, uint32_t> m_entity_to_index;
        std::vector<Entity> m_index_to_entity;
        std::vector<T> m_components;
    };

    namespace ecs
    {
        std::vector<IComponentPool*>& component_pools();

        inline uint32_t new_component_id()
        {
            static uint32_t id = 0;
            return id++;
        }

        template<typename T>
        inline uint32_t component_id()
        {
            static uint32_t comp_id = new_component_id();
            return comp_id;
        }

        inline Entity create_entity()
        {
            static uint32_t entity_id = 1;
            return entity_id++;
        }

        template<typename T>
        inline T& get_component(Entity entity)
        {
            uint32_t comp_id = component_id<T>();
            ASSERT(comp_id < component_pools().size());
            auto pool = (ComponentPool<T>*)(component_pools()[comp_id]);
            return pool->get_component(entity);
        }

        template<typename T>
        inline void add_component(const T& comp, Entity entity)
        {
            uint32_t comp_id = component_id<T>();
            ASSERT(comp_id <= component_pools().size());

            if (comp_id == component_pools().size())
            {
                auto pool = new ComponentPool<T>();
                component_pools().push_back((IComponentPool*)pool);
                pool->add_component(comp, entity);
            }
            else
            {
                auto pool = (ComponentPool<T>*)(component_pools()[comp_id]);
                pool->add_component(comp, entity);
            }
        }

        template<typename T>
        inline ComponentPool<T>& get_components()
        {
            uint32_t comp_id = component_id<T>();
            ASSERT(comp_id < component_pools().size());
            return *(ComponentPool<T>*)(component_pools()[comp_id]);
        }
    }
}
