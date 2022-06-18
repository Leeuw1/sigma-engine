#pragma once

#include "base.h"

#include <cstdlib>
#include <functional>

#define SGE_COMP_ARGS(className) sizeof(className), #className
#define SGE_STRINGIFY(x) #x

namespace sge::ecs
{
	using Byte			= uint8_t;
	using EntityID		= uint32_t;
	using ComponentID	= uint32_t;

	struct ComponentInfo
	{
		EntityID Entity;
		ComponentID ID;
	};

	class Registry
	{
	public:
		template<typename ComponentClass>
		using ForEachFunc = std::function<void(ComponentClass*)>;
		
		Registry();
		~Registry();
		EntityID NewEntityID();

		template<typename ComponentClass, typename... Args>
		void AddComponent(EntityID entity, Args&&... args);
		
		template<typename ComponentClass>
		void ForEach(const ForEachFunc<ComponentClass>& func);

	private:
		static ComponentID HashString(const std::string& string);
	private:
		EntityID m_AvailableEntityID;
		size_t m_Size;
		Byte* m_Components;
	};

	template<typename ComponentClass, typename... Args>
	void Registry::AddComponent(EntityID entity, Args&&... args)
	{
		const std::string className = typeid(ComponentClass).name();
		SGE_WARNF("Component class name: '%s'.", className.c_str());

		size_t oldSize = m_Size;
		m_Size += sizeof(ComponentInfo) + sizeof(ComponentClass);
		void* newComponents = realloc(m_Components, m_Size);
		SGE_ASSERTM(newComponents, "Reallocation of registry components failed.");
		m_Components = static_cast<Byte*>(newComponents);

		ComponentInfo info = {
			.Entity	= entity,
			.ID		= HashString(className),
		};

		memcpy(m_Components + oldSize, &info, sizeof(ComponentInfo));

		auto location = (ComponentClass*)(m_Components + oldSize + sizeof(ComponentInfo));
		auto ptr = std::construct_at(location, std::forward<Args>(args)...);
	}

	template<typename ComponentClass>
	void Registry::ForEach(const ForEachFunc<ComponentClass>& func)
	{
		const std::string className = typeid(ComponentClass).name();
		ComponentID classID = HashString(className);

		for (size_t i = 0; i < m_Size;)
		{
			ComponentInfo* info = (ComponentInfo*)(m_Components + i);
			i += sizeof(ComponentInfo);
			if (info->ID == classID)
				func(reinterpret_cast<ComponentClass*>(m_Components + i));
			i += sizeof(ComponentClass);
		}
	}
} // namespace sge::ecs