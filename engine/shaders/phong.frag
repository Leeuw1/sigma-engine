#version 450

layout(location = 0) in vec3 out_Position;
layout(location = 1) in vec3 out_Normal;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstant
{
	float ks; // Specular reflection constant
	float kd; // Diffuse reflection constant
	float ka; // Ambient reflection constant
	float a;  // Shininess constant
	
	vec3 color;
};

const vec3 ambient = vec3(0.02f, 0.02f, 0.02f);
const vec3 specular = vec3(1.0f, 1.0f, 1.0f);

// Phong parameters
//const float ks = 1.0f;	// Specular reflection constant
//const float kd = 0.8f;	// Diffuse reflection constant
//const float ka = 0.5f;	// Ambient reflection constant
//const float a = 15.0f;	// Shininess constant

const vec3 pointLight = vec3(0.0f, -1.0f, 0.0f);
const vec3 viewer = vec3(0.0f, 0.0f, -1.0f); // Direction towards camera

void main()
{
	vec3 dirToLight = normalize(pointLight - out_Position);
	float diffuseValue = max(dot(dirToLight, out_Normal), 0.0f);
	vec3 reflectionDir = -reflect(dirToLight, out_Normal);
	vec3 diffuseColor = diffuseValue * color;

	float specularValue = ks * pow(dot(reflectionDir, viewer), a) * specular.x;
	specularValue = max(specularValue, 0.0f);
	vec3 intensity = ka * ambient + kd * diffuseColor + vec3(specularValue);
	fragColor = vec4(intensity, 1.0f);
}