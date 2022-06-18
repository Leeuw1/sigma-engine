#pragma once

#include "vulkan/Instance.h"
#include "vulkan/Shader.h"
#include "vulkan/Texture.h"

namespace sge
{
	class Material
	{
	public:
		Material(vulkan::Instance* vulkanInstance, const std::string& filepath);
		void Destroy(vulkan::Instance* vulkanInstance);
	private:
		int m_PipelineIndex;
		vulkan::Shader* m_Shader;
		vulkan::Texture* m_Albedo;
		vulkan::Texture* m_NormalMap;

		friend class Renderer;
		friend class Scene;
	};
} // namespace sge