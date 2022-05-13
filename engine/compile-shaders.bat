:: First arg is Vulkan SDK directory, second arg is shaders directory
%1/Bin/glslc.exe %2/shader.vert -o %2/shader.vert.spv
%1/Bin/glslc.exe %2/shader.frag -o %2/shader.frag.spv
pause