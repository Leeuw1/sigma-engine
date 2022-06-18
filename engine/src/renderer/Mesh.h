#pragma once

#include "vulkan/Instance.h"
#include "vulkan/Buffer.h"
#include "vulkan/BufferLayout.h"

#include <string>

namespace sge
{
	class Mesh
	{
	public:
		Mesh(vulkan::Instance* vulkanInstance, const std::string& name);

		// This overload of contructor will likely be only used for testing
		Mesh(vulkan::Instance* vulkanInstance, const float* vertices, size_t verticesSize, const uint32_t* indices, size_t indicesSize);
		~Mesh();
		void Destroy(vulkan::Instance* vulkanInstance);
		void Serialize(vulkan::Instance* vulkanInstance);
	private:
		//std::string m_Name;
		vulkan::VertexBuffer* m_VertexBuffer;
		vulkan::IndexBuffer* m_IndexBuffer;

		friend class Renderer;
		friend class Scene;
	};
} // namespace sge