:: First arg is Vulkan SDK directory, second arg is shaders directory, third arg is shader name
%1/Bin/glslc.exe %2/%3.vert -o %2/%3.vert.spv
%1/Bin/glslc.exe %2/%3.frag -o %2/%3.frag.spv
pause