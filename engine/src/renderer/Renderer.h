#pragma once

#include "vulkan/Instance.h"
#include "Scene.h"
#include "Mesh.h"
#include "Material.h"

namespace sge
{
	class Renderer
	{
	public:
		Renderer(vulkan::Instance* vulkanInstance);
		~Renderer() {}

		uint32_t BeginFrame();
		void EndFrame(uint32_t imageIndex);

		void DrawScene(Scene& scene);
		void DrawMesh(const Mesh& mesh, const Material& material, uint32_t instanceCount);
	private:
		vulkan::Instance* m_VulkanInstance;
	};
} // namespace sge