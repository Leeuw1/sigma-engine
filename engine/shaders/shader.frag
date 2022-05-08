#version 450

layout(location = 0) in vec3 out_Color;
layout(location = 0) out vec4 fragColor;

void main()
{
	fragColor = vec4(out_Color, 1.0f);
}