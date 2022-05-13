cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(sigma-engine)

set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(ENGINE_SRC_DIR ${CMAKE_SOURCE_DIR}/engine/src)
set(VULKAN_SDK_DIR E:/VulkanSDK/1.3.211.0)

add_executable(${PROJECT_NAME}
	${ENGINE_SRC_DIR}/main.cpp
	${ENGINE_SRC_DIR}/Application.cpp
	${ENGINE_SRC_DIR}/Window.cpp
	${ENGINE_SRC_DIR}/Layer.cpp
	${ENGINE_SRC_DIR}/vulkan/Instance.cpp
	${ENGINE_SRC_DIR}/vulkan/Util.cpp
	${ENGINE_SRC_DIR}/vulkan/Pipeline.cpp
	${ENGINE_SRC_DIR}/vulkan/Swapchain.cpp
	${ENGINE_SRC_DIR}/vulkan/Buffer.cpp
)

# Custom build step to compile shaders
add_custom_command(TARGET ${PROJECT_NAME}
	POST_BUILD
	COMMAND cmd /c ${CMAKE_SOURCE_DIR}/engine/compile-shaders.bat ${VULKAN_SDK_DIR} ${CMAKE_SOURCE_DIR}/engine/shaders
)

# Subdirectories
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)

target_compile_definitions(${PROJECT_NAME}
	PUBLIC DEBUG
)

target_include_directories(${PROJECT_NAME}
	PUBLIC ${ENGINE_SRC_DIR}
	PUBLIC ${VULKAN_SDK_DIR}/Include
	PUBLIC ${VENDOR_DIR}/glfw/include
	PUBLIC ${VENDOR_DIR}/glm
)

target_link_directories(${PROJECT_NAME}
	PUBLIC ${VULKAN_SDK_DIR}/Lib
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC glfw
	PUBLIC vulkan-1
	PUBLIC glm
)