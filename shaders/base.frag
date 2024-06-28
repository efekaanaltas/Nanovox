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
	ivec4 data;
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

	CubeFace cubeFace = faces[instanceID];

	int face = cubeFace.data.w & 0x7;
	int material = (cubeFace.data.w >> 3);
	vec3 normal = NormalFromFaceDirection(face);
	float lighting = max(dot(lightDir, normal), 0.0);

	vec3 materials[6] = vec3[]
	(
		vec3(5/255.0, 146/255.0, 18/255.0),
		vec3(6/255.0, 208/255.0, 1/255.0),
		vec3(155/255.0, 236/255.0, 0/255.0),
		vec3(200/255.0, 200/255.0, 200/255.0),
		vec3(0.0, 0.9, 1.0),
		vec3(0.7, 0.7, 0.7)
	);

	vec3 mainGreen = materials[5];
	vec3 col = materials[material];
	col = mix(col, mainGreen, 0.8);

	vec3 seaColor = vec3(0.2, 0.491, 0.601) + vec3(0.2);

	//col = mix(col, seaColor, WorldPos.yyy/16.0);

	if(material == 3 || material == 4)
	{
		col = materials[material];
	}

	vec3 ambient = col/3.0;
	vec3 shaded = col*lighting+ambient;
	float z = gl_FragCoord.z;
	float linearDepth = LinearDepth(z, near, far);
	float linearDepth01 = linearDepth / far;
	vec3 fogged = mix(shaded, fogColor, clamp(linearDepth01*7, 0, 1));

	color = vec4(fogged, 1.0);
}