#version 450

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec2 v_TexCoord;

layout(location = 0) out vec3 out_Position;
layout(location = 1) out vec2 out_TexCoord;
layout(location = 2) out mat4 out_NormalTransform;

layout(binding = 0) uniform UniformBuffer
{
	mat4 Model;
	mat4 View;
	mat4 Projection;
};

void main()
{
	out_Position = v_Position;
	out_TexCoord = v_TexCoord;

	vec4 worldPos = Model * vec4(v_Position, 1.0f);
	worldPos.z += 4.0f;
	out_Position = worldPos.xyz;
	out_NormalTransform = transpose(inverse(Model));

	gl_Position = Projection * View * worldPos;
}