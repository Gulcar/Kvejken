#pragma once
#include "Utils.h"
#include <vector>
#include <bitset>
#include <unordered_map>

namespace kvejken
{
    using Entity = uint32_t;

    class IComponentPool
    {
    public:
        virtual bool has_component(Entity entity) const = 0;
        virtual void remove_component(Entity entity) = 0;

        virtual size_t size() const = 0;
        virtual const char* component_name() const = 0;
    };

    template<typename T>
    class ComponentPool : public IComponentPool
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

        T* try_get_component(Entity entity)
        {
            auto it = m_entity_to_index.find(entity);
            if (it == m_entity_to_index.end())
                return nullptr;
            return &m_components[it->second];
        }

        bool has_component(Entity entity) const override
        {
            return m_entity_to_index.count(entity) > 0;
        }

        void remove_component(Entity entity) override
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

        Entity entity_at(size_t index) { return m_index_to_entity[index]; }
        T& component_at(size_t index) { return m_components[index]; }
        size_t size() const override { return m_components.size(); }

        const char* component_name() const override { return typeid(T).name(); }

    private:
        std::unordered_map<Entity, uint32_t> m_entity_to_index;
        std::vector<Entity> m_index_to_entity;
        std::vector<T> m_components;
    };

    template<typename T1>
    class ComponentPoolCollectionIds1
    {
    public:
        ComponentPoolCollectionIds1(ComponentPool<T1>* p1)
        {
            m_p1 = p1;
        }

        class Iterator
        {
        public:
            using value_type = std::pair<Entity, T1&>;

            Iterator(ComponentPool<T1>* p1, size_t i)
            {
                m_p1 = p1;
                m_i = i;
            }

            value_type operator*()
            {
                return { m_p1->entity_at(m_i), m_p1->component_at(m_i) };
            }

            Iterator& operator++()
            {
                if (m_i < m_p1->size())
                    m_i++;
                return *this;
            }

            bool operator==(const Iterator& b)
            {
                return m_p1 == b.m_p1
                    && m_i == b.m_i;
            }

            bool operator!=(const Iterator& b)
            {
                return !(*this == b);
            }

        private:
            ComponentPool<T1>* m_p1;
            size_t m_i;
        };

        Iterator begin() { return Iterator(m_p1, 0); }
        Iterator end() { return Iterator(m_p1, m_p1->size()); }

    private:
        ComponentPool<T1>* m_p1;
    };

    template<typename T1, typename T2>
    class ComponentPoolCollection2
    {
    public:
        ComponentPoolCollection2(ComponentPool<T1>* p1, ComponentPool<T2>* p2)
        {
            m_p1 = p1;
            m_p2 = p2;
        }

        class Iterator
        {
        public:
            using value_type = std::pair<T1&, T2&>;

            Iterator(ComponentPool<T1>* p1, ComponentPool<T2>* p2, size_t i)
            {
                m_p1 = p1;
                m_p2 = p2;
                m_i = i;

                while (m_i < m_p1->size() && m_p2->try_get_component(m_p1->entity_at(m_i)) == nullptr)
                {
                    m_i++;
                }
            }

            value_type operator*()
            {
                Entity e = m_p1->entity_at(m_i);
                return { m_p1->component_at(m_i), m_p2->get_component(e) };
            }

            Iterator& operator++()
            {
                T2* t;
                do {
                    m_i++;
                    if (m_i == m_p1->size())
                        return *this;
                    Entity e = m_p1->entity_at(m_i);
                    t = m_p2->try_get_component(e);
                } while (t == nullptr);

                return *this;
            }

            bool operator==(const Iterator& b)
            {
                return m_p1 == b.m_p1
                    && m_p2 == b.m_p2
                    && m_i == b.m_i;
            }

            bool operator!=(const Iterator& b)
            {
                return !(*this == b);
            }

        private:
            ComponentPool<T1>* m_p1;
            ComponentPool<T2>* m_p2;
            size_t m_i;
        };

        Iterator begin() { return Iterator(m_p1, m_p2, 0); }
        Iterator end() { return Iterator(m_p1, m_p2, m_p1->size()); }

    private:
        ComponentPool<T1>* m_p1;
        ComponentPool<T2>* m_p2;
    };

    template<typename T1, typename T2>
    class ComponentPoolCollectionIds2
    {
    public:
        ComponentPoolCollectionIds2(ComponentPool<T1>* p1, ComponentPool<T2>* p2)
        {
            m_p1 = p1;
            m_p2 = p2;
        }

        class Iterator
        {
        public:
            using value_type = std::tuple<Entity, T1&, T2&>;

            Iterator(ComponentPool<T1>* p1, ComponentPool<T2>* p2, size_t i)
            {
                m_p1 = p1;
                m_p2 = p2;
                m_i = i;

                while (m_i < m_p1->size() && m_p2->try_get_component(m_p1->entity_at(m_i)) == nullptr)
                {
                    m_i++;
                }
            }

            value_type operator*()
            {
                Entity e = m_p1->entity_at(m_i);
                return { e, m_p1->component_at(m_i), m_p2->get_component(e) };
            }

            Iterator& operator++()
            {
                T2* t;
                do {
                    m_i++;
                    if (m_i == m_p1->size())
                        return *this;
                    Entity e = m_p1->entity_at(m_i);
                    t = m_p2->try_get_component(e);
                } while (t == nullptr);

                return *this;
            }

            bool operator==(const Iterator& b)
            {
                return m_p1 == b.m_p1
                    && m_p2 == b.m_p2
                    && m_i == b.m_i;
            }

            bool operator!=(const Iterator& b)
            {
                return !(*this == b);
            }

        private:
            ComponentPool<T1>* m_p1;
            ComponentPool<T2>* m_p2;
            size_t m_i;
        };

        Iterator begin() { return Iterator(m_p1, m_p2, 0); }
        Iterator end() { return Iterator(m_p1, m_p2, m_p1->size()); }

    private:
        ComponentPool<T1>* m_p1;
        ComponentPool<T2>* m_p2;
    };

    template<typename T1, typename T2, typename T3>
    class ComponentPoolCollection3
    {
    public:
        ComponentPoolCollection3(ComponentPool<T1>* p1, ComponentPool<T2>* p2, ComponentPool<T3>* p3)
        {
            m_p1 = p1;
            m_p2 = p2;
            m_p3 = p3;
        }

        class Iterator
        {
        public:
            using value_type = std::tuple<T1&, T2&, T3&>;

            Iterator(ComponentPool<T1>* p1, ComponentPool<T2>* p2, ComponentPool<T3>* p3, size_t i)
            {
                m_p1 = p1;
                m_p2 = p2;
                m_p3 = p3;
                m_i = i;

                while (m_i < m_p1->size() && (
                    m_p2->try_get_component(m_p1->entity_at(m_i)) == nullptr ||
                    m_p3->try_get_component(m_p1->entity_at(m_i)) == nullptr))
                {
                    m_i++;
                }
            }

            value_type operator*()
            {
                Entity e = m_p1->entity_at(m_i);
                return { m_p1->component_at(m_i), m_p2->get_component(e), m_p3->get_component(e) };
            }

            Iterator& operator++()
            {
                Entity e;
                do {
                    m_i++;
                    if (m_i == m_p1->size())
                        return *this;
                    e = m_p1->entity_at(m_i);
                } while (m_p2->try_get_component(e) == nullptr || m_p3->try_get_component(e) == nullptr);

                return *this;
            }

            bool operator==(const Iterator& b)
            {
                return m_p1 == b.m_p1
                    && m_p2 == b.m_p2
                    && m_p3 == b.m_p3
                    && m_i == b.m_i;
            }

            bool operator!=(const Iterator& b)
            {
                return !(*this == b);
            }

        private:
            ComponentPool<T1>* m_p1;
            ComponentPool<T2>* m_p2;
            ComponentPool<T3>* m_p3;
            size_t m_i;
        };

        Iterator begin() { return Iterator(m_p1, m_p2, m_p3, 0); }
        Iterator end() { return Iterator(m_p1, m_p2, m_p3, m_p1->size()); }

    private:
        ComponentPool<T1>* m_p1;
        ComponentPool<T2>* m_p2;
        ComponentPool<T3>* m_p3;
    };

    template<typename T1, typename T2, typename T3>
    class ComponentPoolCollectionIds3
    {
    public:
        ComponentPoolCollectionIds3(ComponentPool<T1>* p1, ComponentPool<T2>* p2, ComponentPool<T3>* p3)
        {
            m_p1 = p1;
            m_p2 = p2;
            m_p3 = p3;
        }

        class Iterator
        {
        public:
            using value_type = std::tuple<Entity, T1&, T2&, T3&>;

            Iterator(ComponentPool<T1>* p1, ComponentPool<T2>* p2, ComponentPool<T3>* p3, size_t i)
            {
                m_p1 = p1;
                m_p2 = p2;
                m_p3 = p3;
                m_i = i;

                while (m_i < m_p1->size() && (
                    m_p2->try_get_component(m_p1->entity_at(m_i)) == nullptr ||
                    m_p3->try_get_component(m_p1->entity_at(m_i)) == nullptr))
                {
                    m_i++;
                }
            }

            value_type operator*()
            {
                Entity e = m_p1->entity_at(m_i);
                return { e, m_p1->component_at(m_i), m_p2->get_component(e), m_p3->get_component(e) };
            }

            Iterator& operator++()
            {
                Entity e;
                do {
                    m_i++;
                    if (m_i == m_p1->size())
                        return *this;
                    e = m_p1->entity_at(m_i);
                } while (m_p2->try_get_component(e) == nullptr || m_p3->try_get_component(e) == nullptr);

                return *this;
            }

            bool operator==(const Iterator& b)
            {
                return m_p1 == b.m_p1
                    && m_p2 == b.m_p2
                    && m_p3 == b.m_p3
                    && m_i == b.m_i;
            }

            bool operator!=(const Iterator& b)
            {
                return !(*this == b);
            }

        private:
            ComponentPool<T1>* m_p1;
            ComponentPool<T2>* m_p2;
            ComponentPool<T3>* m_p3;
            size_t m_i;
        };

        Iterator begin() { return Iterator(m_p1, m_p2, m_p3, 0); }
        Iterator end() { return Iterator(m_p1, m_p2, m_p3, m_p1->size()); }

    private:
        ComponentPool<T1>* m_p1;
        ComponentPool<T2>* m_p2;
        ComponentPool<T3>* m_p3;
    };

    namespace ecs
    {
        std::vector<IComponentPool*>& component_pools();
        std::unordered_map<Entity, std::bitset<32>>& signatures();
        std::vector<Entity>& to_destroy();

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

        inline void destroy_entity(Entity entity)
        {
            auto it = signatures().find(entity);
            ASSERT(it != signatures().end());
            std::bitset<32> signature = it->second;
            signatures().erase(it);

            for (int i = 0; i < 32 && signature.any(); i++)
            {
                if (signature[0] == true)
                {
                    uint32_t comp_id = i;
                    auto pool = component_pools()[comp_id];
                    pool->remove_component(entity);

                    signature >>= 1;
                }
            }
        }

        inline void queue_destroy_entity(Entity entity)
        {
            to_destroy().push_back(entity);
        }

        inline void destroy_queued_entities()
        {
            for (const auto& entity : to_destroy())
            {
                destroy_entity(entity);
            }
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

            signatures()[entity][comp_id] = true;
        }

        template<typename T>
        inline void remove_component(Entity entity)
        {
            uint32_t comp_id = component_id<T>();
            ASSERT(comp_id < component_pools().size());

            auto pool = (ComponentPool<T>*)(component_pools()[comp_id]);
            pool->remove_component(entity);

            signatures()[entity][comp_id] = false;
        }

        template<typename T>
        inline ComponentPool<T>& get_components()
        {
            uint32_t comp_id = component_id<T>();
            ASSERT(comp_id < component_pools().size());
            return *(ComponentPool<T>*)(component_pools()[comp_id]);
        }

        template<typename T1, typename T2>
        inline ComponentPoolCollection2<T1, T2> get_components()
        {
            uint32_t comp_id1 = component_id<T1>();
            ASSERT(comp_id1 < component_pools().size());
            uint32_t comp_id2 = component_id<T2>();
            ASSERT(comp_id2 < component_pools().size());

            auto p1 = (ComponentPool<T1>*)(component_pools()[comp_id1]);
            auto p2 = (ComponentPool<T2>*)(component_pools()[comp_id2]);
            return ComponentPoolCollection2(p1, p2);
        }

        template<typename T1, typename T2, typename T3>
        inline ComponentPoolCollection3<T1, T2, T3> get_components()
        {
            uint32_t comp_id1 = component_id<T1>();
            ASSERT(comp_id1 < component_pools().size());
            uint32_t comp_id2 = component_id<T2>();
            ASSERT(comp_id2 < component_pools().size());
            uint32_t comp_id3 = component_id<T3>();
            ASSERT(comp_id3 < component_pools().size());

            auto p1 = (ComponentPool<T1>*)(component_pools()[comp_id1]);
            auto p2 = (ComponentPool<T2>*)(component_pools()[comp_id2]);
            auto p3 = (ComponentPool<T3>*)(component_pools()[comp_id3]);
            return ComponentPoolCollection3(p1, p2, p3);
        }

        template<typename T>
        inline ComponentPoolCollectionIds1<T>& get_components_ids()
        {
            uint32_t comp_id = component_id<T>();
            ASSERT(comp_id < component_pools().size());

            auto p1 = *(ComponentPool<T>*)(component_pools()[comp_id]);
            return ComponentPoolCollectionIds1(p1);
        }

        template<typename T1, typename T2>
        inline ComponentPoolCollectionIds2<T1, T2> get_components_ids()
        {
            uint32_t comp_id1 = component_id<T1>();
            ASSERT(comp_id1 < component_pools().size());
            uint32_t comp_id2 = component_id<T2>();
            ASSERT(comp_id2 < component_pools().size());

            auto p1 = (ComponentPool<T1>*)(component_pools()[comp_id1]);
            auto p2 = (ComponentPool<T2>*)(component_pools()[comp_id2]);
            return ComponentPoolCollectionIds2(p1, p2);
        }

        template<typename T1, typename T2, typename T3>
        inline ComponentPoolCollectionIds3<T1, T2, T3> get_components_ids()
        {
            uint32_t comp_id1 = component_id<T1>();
            ASSERT(comp_id1 < component_pools().size());
            uint32_t comp_id2 = component_id<T2>();
            ASSERT(comp_id2 < component_pools().size());
            uint32_t comp_id3 = component_id<T3>();
            ASSERT(comp_id3 < component_pools().size());

            auto p1 = (ComponentPool<T1>*)(component_pools()[comp_id1]);
            auto p2 = (ComponentPool<T2>*)(component_pools()[comp_id2]);
            auto p3 = (ComponentPool<T3>*)(component_pools()[comp_id3]);
            return ComponentPoolCollectionIds3(p1, p2, p3);
        }
    }

    // chatgpt mi je dal idejo kako bi z vecimi threadi hkrati updatal komponente
    // in sicer bi grupiral sisteme glede na to katere komponente potrebujejo
    // (to bi ugotovil z bit maskami) in potem vse neodvisne skupine izvedel na svojem threadu
    // mimogrede koncno razumem kaj dela std::bind
}
