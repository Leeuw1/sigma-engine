cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(sigma-engine)

set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
set(ENGINE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(VULKAN_SDK_DIR E:/VulkanSDK/1.3.211.0)

add_library(${PROJECT_NAME} STATIC
	${ENGINE_SRC_DIR}/Application.cpp
	${ENGINE_SRC_DIR}/Window.cpp
	${ENGINE_SRC_DIR}/Log.cpp
	${ENGINE_SRC_DIR}/Layer.cpp
	${ENGINE_SRC_DIR}/ImGuiLayer.cpp
	${ENGINE_SRC_DIR}/FileUtil.cpp
	${ENGINE_SRC_DIR}/ecs/Registry.cpp
	${ENGINE_SRC_DIR}/renderer/Renderer.cpp
	${ENGINE_SRC_DIR}/renderer/Scene.cpp
	${ENGINE_SRC_DIR}/renderer/Mesh.cpp
	${ENGINE_SRC_DIR}/renderer/Material.cpp
	${ENGINE_SRC_DIR}/vulkan/Instance.cpp
	${ENGINE_SRC_DIR}/vulkan/Util.cpp
	${ENGINE_SRC_DIR}/vulkan/Pipeline.cpp
	${ENGINE_SRC_DIR}/vulkan/Swapchain.cpp
	${ENGINE_SRC_DIR}/vulkan/Buffer.cpp
	${ENGINE_SRC_DIR}/vulkan/BufferLayout.cpp
	${ENGINE_SRC_DIR}/vulkan/Shader.cpp
	${ENGINE_SRC_DIR}/vulkan/Texture.cpp
	${VENDOR_DIR}/stb_image/stb_image.cpp
)

# Custom build step to compile shaders and move dll
add_custom_command(TARGET ${PROJECT_NAME}
	POST_BUILD
	COMMAND cmd /c ${CMAKE_CURRENT_SOURCE_DIR}/compile-shaders.bat ${VULKAN_SDK_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/shaders texture
	COMMAND cmd /c ${CMAKE_CURRENT_SOURCE_DIR}/compile-shaders.bat ${VULKAN_SDK_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/shaders phong
)

# Subdirectories
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/imgui)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE DEBUG
	#SGE_SHADER_DIR=${CMAKE_CURRENT_SOURCE_DIR}/shaders
)

target_include_directories(${PROJECT_NAME}
	PUBLIC ${ENGINE_SRC_DIR}
	PUBLIC ${VULKAN_SDK_DIR}/Include
	PUBLIC ${VENDOR_DIR}
	PUBLIC ${VENDOR_DIR}/glfw/include
	PUBLIC ${VENDOR_DIR}/glm
	PUBLIC ${VENDOR_DIR}/imgui
	PUBLIC ${VENDOR_DIR}/imgui/imgui
)

target_link_directories(${PROJECT_NAME}
	PUBLIC ${VULKAN_SDK_DIR}/Lib
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC glfw
	PUBLIC vulkan-1
	PUBLIC glm
	PUBLIC imgui
)