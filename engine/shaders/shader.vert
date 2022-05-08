#version 450

layout (location = 0) out vec3 out_Color;

const vec2 positions[3] = {
	vec2(0.0f, -0.5f),
	vec2(0.5f, 0.5f),
	vec2(-0.5f, 0.5f)
};

const vec3 colors[3] = {
	vec3(1.0f, 0.0f, 0.0f),
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f, 0.0f, 1.0f)
};

void main()
{
	out_Color = colors[gl_VertexIndex];
	gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
}