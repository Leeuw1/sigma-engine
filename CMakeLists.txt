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
	${ENGINE_SRC_DIR}/vulkan/Instance.cpp
)

# Subdirectories
add_subdirectory(vendor/glfw)

target_compile_definitions(${PROJECT_NAME}
	PUBLIC DEBUG
)

target_include_directories(${PROJECT_NAME}
	PUBLIC ${ENGINE_SRC_DIR}
	PUBLIC ${VULKAN_SDK_DIR}/Include
	PUBLIC ${VENDOR_DIR}/glfw/include
)

target_link_directories(${PROJECT_NAME}
	PUBLIC ${VULKAN_SDK_DIR}/Lib
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC glfw
	PUBLIC vulkan-1
)