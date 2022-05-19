project(imgui)

add_library(imgui
    ${VENDOR_DIR}/imgui/imgui.cpp
    ${VENDOR_DIR}/imgui/imgui_demo.cpp
    ${VENDOR_DIR}/imgui/imgui_draw.cpp
    ${VENDOR_DIR}/imgui/imgui_tables.cpp
    ${VENDOR_DIR}/imgui/imgui_widgets.cpp

    ${VENDOR_DIR}/imgui/backends/imgui_impl_glfw.cpp>
    ${VENDOR_DIR}/imgui/backends/imgui_impl_vulkan.cpp>
)

target_include_directories(imgui
    PRIVATE ${VENDOR_DIR}/imgui
    PRIVATE ${VENDOR_DIR}/glfw/include
    PRIVATE ${VULKAN_SDK_DIR}/Include
)
