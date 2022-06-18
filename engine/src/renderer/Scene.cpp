#include "Scene.h"

#include <list>
#include <unordered_set>

namespace sge
{
	Scene::Scene()
	{
	}
	
	ecs::EntityID Scene::AddModel(vulkan::Instance* vulkanInstance, const std::string& meshName, const std::string& materialPath)
	{
		ecs::EntityID entity = m_Registry.NewEntityID();
		m_Registry.AddComponent<DrawableComponent>(entity, vulkanInstance,
			meshName, materialPath);
		
		return entity;
	}

	ecs::EntityID Scene::AddModel(vulkan::Instance* vulkanInstance, const float* vertices, size_t verticesSize, const uint32_t* indices, size_t indicesSize,
		const std::string& materialPath)
	{
		ecs::EntityID entity = m_Registry.NewEntityID();
		m_Registry.AddComponent<DrawableComponent>(entity, vulkanInstance, vertices, verticesSize,
			indices, indicesSize, materialPath);

		return entity;
	}

	void Scene::Destroy(vulkan::Instance* vulkanInstance)
	{
		m_Registry.ForEach<DrawableComponent>(
		[vulkanInstance](DrawableComponent* drawableComp)
		{
			drawableComp->Mesh.Destroy(vulkanInstance);
			drawableComp->Material.Destroy(vulkanInstance);
		});
	}

	void Scene::InitDescriptorSets(vulkan::Instance* vulkanInstance, vulkan::FrameGroup<vulkan::UniformBuffer*>& uniformBuffers)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		vulkanInstance->AddLayoutBindingUniformBuffer(bindings);

		// Record which descriptors have already been created
		std::unordered_set<void*> assets;

		m_Registry.ForEach<DrawableComponent>(
		[&](DrawableComponent* drawableComp)
		{
			if (drawableComp->Material.m_Albedo && !assets.contains(drawableComp->Material.m_Albedo))
			{
				vulkanInstance->AddLayoutBindingTexture(bindings);
				assets.insert(drawableComp->Material.m_Albedo);
			}

			if (drawableComp->Material.m_NormalMap && !assets.contains(drawableComp->Material.m_NormalMap))
			{
				vulkanInstance->AddLayoutBindingTexture(bindings);
				assets.insert(drawableComp->Material.m_NormalMap);
			}
		});

		vulkanInstance->AllocateDescriptorSets(bindings);

		for (uint32_t frameIndex = 0; frameIndex < vulkan::MAX_FRAMES_IN_FLIGHT; frameIndex++)
		{
			std::vector<VkWriteDescriptorSet> descriptorWrites;

			// Use list to prevent image and buffer info from being freed too early
			std::list<void*> infos;
			
			auto bufferInfo = vulkanInstance->GetBufferInfo(uniformBuffers[frameIndex]);
			infos.push_back(bufferInfo);
			vulkanInstance->AddDescriptorWrite(descriptorWrites, bufferInfo, frameIndex);

			m_Registry.ForEach<DrawableComponent>(
			[&](void* comp)
			{
				auto drawableComp = reinterpret_cast<DrawableComponent*>(comp);
				
				if (drawableComp->Material.m_Albedo)
				{
					auto albedoInfo = vulkanInstance->GetImageInfo(drawableComp->Material.m_Albedo);
					infos.push_back(albedoInfo);
					vulkanInstance->AddDescriptorWrite(descriptorWrites, albedoInfo, frameIndex);
				}

				if (drawableComp->Material.m_NormalMap)
				{
					auto normalMapInfo = vulkanInstance->GetImageInfo(drawableComp->Material.m_NormalMap);
					infos.push_back(normalMapInfo);
					vulkanInstance->AddDescriptorWrite(descriptorWrites, normalMapInfo, frameIndex);
				}
			});
			
			if (!descriptorWrites.empty())
				vkUpdateDescriptorSets(vulkanInstance->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

			for (auto info : infos)
				delete info;
		}
	}

	void Scene::InitPipelines(vulkan::Instance* vulkanInstance)
	{
		m_Registry.ForEach<DrawableComponent>(
		[vulkanInstance](DrawableComponent* drawableComp)
		{
			drawableComp->Material.m_PipelineIndex = vulkanInstance->CreatePipeline(
			drawableComp->Material.m_Shader,
			drawableComp->Mesh.m_VertexBuffer->GetLayout());
		});
	}
} // namespace sge