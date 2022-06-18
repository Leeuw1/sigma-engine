#pragma once

#include <array>

namespace sge::vulkan
{
	constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

	template<typename T>
	using FrameGroup = std::array<T, MAX_FRAMES_IN_FLIGHT>;
} // namespace sge::vulkan