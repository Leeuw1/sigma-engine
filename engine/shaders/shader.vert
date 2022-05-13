#version 450

layout(location = 0) in vec2 v_Position;
layout(location = 1) in vec3 v_Color;
layout(location = 0) out vec3 out_Color;

layout(push_constant) uniform PushConstant
{
	mat4 projection;
}
push;

layout(binding = 0) uniform UniformBuffer
{
	mat4 transforms[4];
}
uBuffer;

void main()
{
	out_Color = v_Color;
	float instanceIndex = float(gl_InstanceIndex);
	gl_Position = uBuffer.transforms[gl_InstanceIndex] * vec4(v_Position.x + instanceIndex / 4.0f, v_Position.y, 0.0f, 1.0f);
}