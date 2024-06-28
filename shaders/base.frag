#version 450 core

in vec3 WorldPos;
in vec3 VertexPos;

out vec4 color;

flat in int instanceID;

uniform vec3 fogColor;

uniform float near = 0.1;
uniform float far = 100.0;

struct CubeFace
{
    ivec3 pos;
    int face;
	int material;
	vec3 a;
};

layout(std430, binding = 2) readonly buffer voxels
{
	CubeFace faces[];
};

vec3 NormalFromFaceDirection(int faceDir)
{
	if(faceDir == 0) return vec3(-1, 0, 0);
	if(faceDir == 1) return vec3(+1, 0, 0);
	if(faceDir == 2) return vec3(0, -1, 0);
	if(faceDir == 3) return vec3(0, +1, 0);
	if(faceDir == 4) return vec3(0, 0, -1);
	if(faceDir == 5) return vec3(0, 0, +1);
}

float LinearDepth(float d, float near, float far)
{
    float zn = 2.0 * d - 1.0;
    return 2.0 * near * far / (far + near - zn * (far - near));
}

void main()
{
	vec3 lightDir = vec3(0.5, -0.7, 0.5);
	lightDir = normalize(lightDir);

	int face = faces[instanceID].face;
	vec3 normal = NormalFromFaceDirection(face);
	float lighting = max(dot(lightDir, normal), 0.0);

	vec3 materials[8] = vec3[]
	(
		vec3(0.1, 1.0, 0.1),
		vec3(0.8, 0.9, 0.2),
		vec3(0.1, 0.8, 0.7),
		vec3(0.1, 0.2, 0.7),
		vec3(0.9, 0.2, 0.7),
		vec3(0.9, 0.2, 0.1),
		vec3(1, 1, 1),
		vec3(0, 0, 0)
	);

	vec3 col = materials[0];//materials[faces[instanceID].material];

	vec3 ambient = col/3.0;

	vec3 shaded = col*lighting+ambient;

	float z = gl_FragCoord.z;

	float linearDepth = LinearDepth(z, 0.1, 100.0);
	float linearDepth01 = linearDepth / far;
	vec3 fogged = mix(shaded, fogColor, clamp(linearDepth01, 0, 1));

	color = vec4(fogged, 1.0);
}