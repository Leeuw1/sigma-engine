#version 450

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;

layout(location = 0) out vec3 out_Color;

layout(push_constant) uniform PushConstant
{
	mat4 Projection;
}
push;

layout(binding = 0) uniform UniformBuffer
{
	mat4 Transforms[4];
}
uBuffer;

const vec3 dirToLight = normalize(vec3(0.5f, 1.0f, -1.0f));

const vec3 ambient = vec3(0.02f, 0.02f, 0.02f);
const vec3 color = vec3(1.0f, 0.03f, 0.03f);

void main()
{
	mat4 normalTransform = transpose(inverse(uBuffer.Transforms[0]));
	vec3 normal = vec4(normalTransform * vec4(v_Normal, 1.0f)).xyz;

	float diffuse = max(dot(dirToLight, normalize(normal)), 0.0f);

	out_Color = ambient + diffuse * color;
	vec4 worldPos = uBuffer.Transforms[0] * vec4(v_Position, 1.0f);
	float xOffset = -2.0f + gl_InstanceIndex;
	worldPos.x += xOffset;
	worldPos.y += 0.5f;
	worldPos.z += 1.0f;
	gl_Position = push.Projection * worldPos;
}