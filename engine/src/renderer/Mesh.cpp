#include "Mesh.h"
#include "FileUtil.h"

#include <fstream>

namespace sge
{
	// TODO: Only parse .obj files once, and store the geometry data
	//		 in a custom binary format after first use.

	/*
	* Custom binary format:
	* 
	* struct
	* {
	*	  struct Layout
	*	  {
	*		   uint32_t AttribCount;
	*		   enum AttributeType Attribs[AttribCount];
	*	  };
	* 
	*     size_t VerticesSize;
	*	  float Vertices[VerticesSize];
	*	  size_t IndicesSize;
	*	  uint32_t Indices[IndicesSize];
	* };
	*/

	void Mesh::Serialize(vulkan::Instance* vulkanInstance)
	{
		const std::string filepath = /*m_Name +*/  ".svb";
		std::ofstream file(filepath, std::ios::binary);

		SGE_ASSERTF(file.is_open(), "Could not open file '%s'", filepath.c_str());

		const auto* layout = m_VertexBuffer->GetLayout();
		for (auto& attribute : layout->GetAttributes())
			file << attribute.Type;

		size_t size = m_VertexBuffer->GetCount() * layout->GetStride();
		file << size;
		
		void* data;
		vkMapMemory(vulkanInstance->GetDevice(), m_VertexBuffer->GetDeviceMemory(), 0, size, 0, &data);
		file.write(static_cast<const char*>(data), size);
		vkUnmapMemory(vulkanInstance->GetDevice(), m_VertexBuffer->GetDeviceMemory());

		size = m_IndexBuffer->GetCount() * sizeof(uint32_t);
		file << size;
		
		vkMapMemory(vulkanInstance->GetDevice(), m_IndexBuffer->GetDeviceMemory(), 0, size, 0, &data);
		file.write(static_cast<const char*>(data), size);
	}

	Mesh::Mesh(vulkan::Instance* vulkanInstance, const std::string& name)
		: m_VertexBuffer(nullptr), m_IndexBuffer(nullptr)
	{
		std::string filepath = name + ".svb";
		std::ifstream file(filepath, std::ios::binary);
		
		// Check if binary exists
		if (file.is_open())
		{
			// Load from binary
			vulkan::BufferLayout layout;
			std::vector<float> vertices;
			std::vector<uint32_t> indices;
			
			// Layout
			uint32_t attribCount;
			file >> attribCount;
			layout.Resize(attribCount);
			std::vector<vulkan::AttributeType> attribs(attribCount);
			file.read((char*)attribs.data(), attribCount * sizeof(vulkan::Attribute));
			
			for (const auto attrib : attribs)
				layout.AddAttribute(attrib);

			// Vertices
			size_t size;
			file >> size;
			vertices.resize(size / sizeof(float));
			file.read((char*)vertices.data(), size);

			// Indices
			file >> size;
			indices.resize(size / sizeof(uint32_t));
			file.read((char*)vertices.data(), size);

			// Create buffers
			m_VertexBuffer = new vulkan::VertexBuffer(vulkanInstance->GetDevice(), vulkanInstance->GetPhysicalDevice(),
				vulkanInstance->GetCommandPool(), vulkanInstance->GetGraphicsQueue(), vertices.data(), vertices.size() * layout.GetStride(), layout);
			m_IndexBuffer = new vulkan::IndexBuffer(vulkanInstance->GetDevice(), vulkanInstance->GetPhysicalDevice(),
				vulkanInstance->GetCommandPool(), vulkanInstance->GetGraphicsQueue(), indices.data(), size);
		}
		else
		{
			// Otherwise parse .obj file
			memcpy(filepath.data() + name.size(), ".txt", 4);
			auto [vertices, indices] = file::LoadOBJFile(filepath, 6);
			file::CalculateNormals(vertices, indices);
			vulkan::BufferLayout vbLayout = { vulkan::_Vec3, vulkan::_Vec3 }; // Hard coded for now
			m_VertexBuffer = new vulkan::VertexBuffer(vulkanInstance->GetDevice(), vulkanInstance->GetPhysicalDevice(),
				vulkanInstance->GetCommandPool(), vulkanInstance->GetGraphicsQueue(), vertices.data(), vertices.size() * sizeof(float), vbLayout);
			m_IndexBuffer = new vulkan::IndexBuffer(vulkanInstance->GetDevice(), vulkanInstance->GetPhysicalDevice(),
				vulkanInstance->GetCommandPool(), vulkanInstance->GetGraphicsQueue(), indices.data(), indices.size() * sizeof(uint32_t));
		}
	}

	Mesh::Mesh(vulkan::Instance* vulkanInstance, const float* vertices, size_t verticesSize, const uint32_t* indices, size_t indicesSize)
		: m_VertexBuffer(nullptr), m_IndexBuffer(nullptr)
	{
		vulkan::BufferLayout vbLayout = { vulkan::_Vec3, vulkan::_Vec2 };
		m_VertexBuffer = new vulkan::VertexBuffer(vulkanInstance->GetDevice(), vulkanInstance->GetPhysicalDevice(), vulkanInstance->GetCommandPool(),
			vulkanInstance->GetGraphicsQueue(), vertices, verticesSize, vbLayout);
		m_IndexBuffer = new vulkan::IndexBuffer(vulkanInstance->GetDevice(), vulkanInstance->GetPhysicalDevice(), vulkanInstance->GetCommandPool(),
			vulkanInstance->GetGraphicsQueue(), indices, indicesSize);
	}

	Mesh::~Mesh()
	{
		delete m_VertexBuffer;
		delete m_IndexBuffer;
	}

	void Mesh::Destroy(vulkan::Instance* vulkanInstance)
	{
		m_VertexBuffer->Destroy(vulkanInstance->GetDevice());
		m_IndexBuffer->Destroy(vulkanInstance->GetDevice());
	}
} // namespace sge