#version 450

#include "phong.glsl"

layout(location = 0) in vec3 out_Position;
layout(location = 1) in vec3 out_Normal;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstant
{
	// Phong parameters
	float ks; // Specular reflection constant
	float kd; // Diffuse reflection constant
	float ka; // Ambient reflection constant
	float a;  // Shininess constant
	
	vec3 color;
};

const vec3 pointLight = vec3(0.0f, -1.0f, 0.0f);
const vec3 viewer = vec3(0.0f, 0.0f, -1.0f); // Direction towards camera

void main()
{
	vec3 intensity = Phong(ks, kd, ka, a, pointLight, viewer, out_Position, out_Normal, color);
	fragColor = vec4(intensity, 1.0f);
	//fragColor = vec4(color, 1.0f);
}