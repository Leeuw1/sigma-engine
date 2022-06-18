// GLSL Header File

// Phong parameters:
// float ks; Specular reflection constant
// float kd; Diffuse reflection constant
// float ka; Ambient reflection constant
// float a;  Shininess constant

const vec3 ambient = vec3(0.02f, 0.02f, 0.02f);
const vec3 specular = vec3(1.0f, 1.0f, 1.0f);

vec3 Phong(float ks, float kd, float ka, float a, vec3 pointLight, vec3 viewer, vec3 position, vec3 normal, vec3 color)
{
	vec3 dirToLight = normalize(pointLight - position);
	float diffuseValue = max(dot(dirToLight, normal), 0.0f);
	vec3 reflectionDir = -reflect(dirToLight, normal);
	vec3 diffuseColor = diffuseValue * color;

	float specularValue = ks * pow(dot(reflectionDir, viewer), a) * specular.x;
	specularValue = max(specularValue, 0.0f);
	return ka * ambient + kd * diffuseColor + vec3(specularValue);
}