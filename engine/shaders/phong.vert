#version 450

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;

layout(location = 0) out vec3 out_Position;
layout(location = 1) out vec3 out_Normal;

layout(binding = 0) uniform UniformBuffer
{
	mat4 Projection;
	mat4 Transform;
};

void main()
{
	mat4 normalTransform = transpose(inverse(Transform));
	out_Normal = normalize(vec4(normalTransform * vec4(v_Normal, 1.0f)).xyz);

	vec4 worldPos = Transform * vec4(v_Position, 1.0f);
	float xOffset = -2.0f + gl_InstanceIndex;
	worldPos.x += xOffset;
	worldPos.y += 0.5f;
	worldPos.z += 1.0f;
	out_Position = worldPos.xyz;
	gl_Position = Projection * worldPos;
}