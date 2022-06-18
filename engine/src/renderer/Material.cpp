#include "Material.h"

#include <fstream>

namespace sge
{
	// TODO: Test if material exists already before loading it

	Material::Material(vulkan::Instance* vulkanInstance, const std::string& filepath)
		: m_PipelineIndex(-1), m_Shader(nullptr), m_Albedo(nullptr), m_NormalMap(nullptr)
	{
		std::ifstream file(filepath);
		SGE_ASSERTF(file.is_open(), "Could not open file '%s'.", filepath.c_str());
		std::string vertPath;
		std::getline(file, vertPath);
		std::string fragPath;
		std::getline(file, fragPath);

		m_Shader = new vulkan::Shader(vulkanInstance->GetDevice(), vulkanInstance->GetDescriptorPool(), vertPath, fragPath);

		// Check if material has albedo or normal map
		if (!file.eof())
		{
			std::string albedoPath;
			std::getline(file, albedoPath);
			m_Albedo = new vulkan::Texture(vulkanInstance->GetDevice(), vulkanInstance->GetPhysicalDevice(),
				vulkanInstance->GetCommandPool(), vulkanInstance->GetGraphicsQueue(), albedoPath);
		}

		if (!file.eof())
		{
			std::string normalMapPath;
			std::getline(file, normalMapPath);
			m_NormalMap = new vulkan::Texture(vulkanInstance->GetDevice(), vulkanInstance->GetPhysicalDevice(),
				vulkanInstance->GetCommandPool(), vulkanInstance->GetGraphicsQueue(), normalMapPath);
		}
	}

	void Material::Destroy(vulkan::Instance* vulkanInstance)
	{
		m_Shader->Destroy(vulkanInstance->GetDevice());
		delete m_Shader;

		if (m_Albedo)
		{
			m_Albedo->Destroy(vulkanInstance->GetDevice());
			delete m_Albedo;
		}

		if (m_NormalMap)
		{
			m_NormalMap->Destroy(vulkanInstance->GetDevice());
			delete m_NormalMap;
		}
	}
} // namespace sge