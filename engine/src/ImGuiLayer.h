#pragma once

#include "Layer.h"
#include "vulkan/Instance.h"

namespace sge
{
	constexpr size_t TEXT_ENTRY_SIZE = 1024;

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer(vulkan::Instance* vulkanInstance);
		virtual ~ImGuiLayer() override;

		virtual void OnUpdate() override;
		virtual bool OnEvent(Event& event) override { return false; }

		static void CheckVkResult(VkResult result);
	private:
		void NewFrame();
		void DrawFrame();
		void ClearTextEntry();
	private:
		vulkan::Instance* m_VulkanInstance;
		char m_TextEntryBuffer[TEXT_ENTRY_SIZE];
	};

} // namespace sge