#version 450

#include "phong.glsl"

layout(location = 0) in vec3 out_Position;
layout(location = 1) in vec2 out_TexCoord;
layout(location = 2) in mat4 out_NormalTransform;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D textureSampler;
layout(binding = 2) uniform sampler2D normalSampler;

layout(push_constant) uniform PushConstant
{
	// Phong parameters
	float ks; // Specular reflection constant
	float kd; // Diffuse reflection constant
	float ka; // Ambient reflection constant
	float a;  // Shininess constant
	
	vec3 color;
};

const vec3 pointLight = { 0.0f, 0.0f, -2.0f };
const vec3 viewer = { 0.0f, 0.0f, 0.0f };

void main()
{
	vec3 normal = -texture(normalSampler, out_TexCoord).xyz;
	normal = normalize(vec4(out_NormalTransform * vec4(normal, 1.0f)).xyz);
	vec3 surfaceColor = texture(textureSampler, out_TexCoord).xyz;
	vec3 intensity = Phong(ks, kd, ka, a, pointLight, viewer, out_Position, normal, surfaceColor);

	fragColor = vec4(intensity, 1.0f);
}