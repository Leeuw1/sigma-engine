#pragma once

#include "ecs/Registry.h"

#include "Mesh.h"
#include "Material.h"

namespace sge
{
	// TODO: Make these non-pointer types to reduce memory redirections
	struct DrawableComponent
	{
		Mesh Mesh;
		Material Material;

		DrawableComponent(vulkan::Instance* vulkanInstance, const std::string& meshName, const std::string& materialPath)
			: Mesh(vulkanInstance, meshName), Material(vulkanInstance, materialPath)
		{
		}

		DrawableComponent(vulkan::Instance* vulkanInstance, const float* vertices, size_t verticesSize, const uint32_t* indices, size_t indicesSize,
			const std::string& materialPath)
			: Mesh(vulkanInstance, vertices, verticesSize, indices, indicesSize), Material(vulkanInstance, materialPath)
		{
		}
	};

	class Scene
	{
	public:
		Scene();
		ecs::EntityID AddModel(vulkan::Instance* vulkanInstance, const std::string& meshName, const std::string& materialPath);
		ecs::EntityID AddModel(vulkan::Instance* vulkanInstance, const float* vertices, size_t verticesSize,
			const uint32_t* indices, size_t indicesSize, const std::string& materialPath);
		void Destroy(vulkan::Instance* vulkanInstance);
		void InitDescriptorSets(vulkan::Instance* vulkanInstance, vulkan::FrameGroup<vulkan::UniformBuffer*>& uniformBuffers); // Uniform buffer as argument is temporary
		void InitPipelines(vulkan::Instance* vulkanInstance);
	private:
		ecs::Registry m_Registry;
		
		friend class Renderer;
	};
} // namespace sge