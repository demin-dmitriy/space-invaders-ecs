#pragma once

#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cassert>

struct Entity;

struct ComponentI
{
	virtual ~ComponentI() = default;
};

struct EntityManager final
{
	Entity* create_entity();
	void destroy_entity(Entity* entity);

	template <class ComponentType>
	std::vector<Entity*> filter();

private:
	friend struct Entity;

	struct Edge final
	{
		Entity* m_entity;
		std::unique_ptr<ComponentI> m_component;
	};

	std::vector<std::unique_ptr<Entity>> m_entities;
	std::unordered_map<std::type_index, std::vector<Edge>> m_components;
};

struct Entity final
{
	template <class Component>
	bool has_component() const
	{
		return has_component(typeid(Component));
	}

	bool has_component(const std::type_index index) const
	{
		return m_edge_index.find(index) != m_edge_index.end();
	}

	template <class Component>
	Component& get_component()
	{
		return static_cast<Component&>(get_component(typeid(Component)));
	}

	ComponentI& get_component(const std::type_index index)
	{
		assert(has_component(index));
		return *m_entity_manager->m_components[index][m_edge_index[index]].m_component;
	}

	template <class Component>
	Component* try_get_component()
	{
		return static_cast<Component*>(try_get_component(typeid(Component)));
	}

	ComponentI* try_get_component(const std::type_index index)
	{
		if (not has_component(index))
		{
			return nullptr;
		}

		return &get_component(index);
	}

	template <class Component, class ... Args>
	Component& add_component(Args&& ... args)
	{
		assert(not has_component<Component>());

		const std::type_index index = typeid(Component);
		std::vector<EntityManager::Edge>& edges = m_entity_manager->m_components[index];
		m_edge_index[index] = edges.size();

		edges.push_back
		(
			EntityManager::Edge
			{
				.m_entity = this,
				.m_component = std::make_unique<Component>(std::forward<Args>(args)...)
			}
		);

		return static_cast<Component&>(*edges.back().m_component);
	}

	template <class Component>
	void remove_component()
	{
		remove_component(typeid(Component));
	}

	void remove_component(const std::type_index index)
	{
		remove_component_edge(index);
		m_edge_index.erase(index);
	}

private:
	friend struct EntityManager;

	explicit Entity(EntityManager* entity_manager, size_t index):
		m_entity_manager(entity_manager),
		m_index(index)
	{
		assert(entity_manager != nullptr);
	}

	void remove_component_edge(const std::type_index index)
	{
		assert(has_component(index));
		std::vector<EntityManager::Edge>& edges = m_entity_manager->m_components[index];
		assert(edges.size() > 0);

		if (edges.back().m_entity != this)
		{
			const size_t i = m_edge_index[index];
			edges[i] = std::move(edges.back());
			edges[i].m_entity->m_edge_index[index] = i;
		}

		edges.pop_back();
	}

	EntityManager* m_entity_manager;
	size_t m_index;
	std::unordered_map<std::type_index, size_t> m_edge_index;
};

inline Entity* EntityManager::create_entity()
{
	m_entities.emplace_back(std::unique_ptr<Entity>(new Entity(this, m_entities.size())));
	return m_entities.back().get();
}

inline void EntityManager::destroy_entity(Entity* entity)
{
	assert(entity != nullptr);

	for (const auto i : entity->m_edge_index)
	{
		entity->remove_component_edge(i.first);
	}

	const size_t entity_index = entity->m_index;
	assert(entity_index < m_entities.size());
	m_entities[entity_index] = std::move(m_entities.back());
	m_entities[entity_index]->m_index = entity_index;
	m_entities.pop_back();
}

template <class ComponentType>
inline std::vector<Entity*> EntityManager::filter()
{
	const std::type_index index(typeid(ComponentType));
	std::vector<Entity*> entities;
	entities.reserve(m_components.size());

	for (const Edge& edge: m_components[index])
	{
		entities.push_back(edge.m_entity);
	}

	return entities;
}
