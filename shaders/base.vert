#version 450 core

out vec3 WorldPos;
out vec3 VertexPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

int ChunkDim = 16;

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

flat out int instanceID;

void main()
{
	const vec3 positions[4] = vec3[]
    (
        vec3(0, 0, 0),
        vec3(+1, 0, 0),
        vec3(0, 0, +1),
        vec3(+1, 0, +1)
    );
    const vec2 coords[4] = vec2[]
    (
        vec2(0, 0),
        vec2(1, 0),
        vec2(0, 1),
        vec2(1, 1)
    );

	VertexPos = positions[gl_VertexID % 4];

    vec3 pos = vec3(faces[gl_InstanceID].pos);
    int face = faces[gl_InstanceID].face;

    if(face == 3)
    {
        VertexPos.y++;
    }
    if(face == 1)
    {
        VertexPos.xy = VertexPos.yx;
        VertexPos.x++;
    }
    if(face == 0)
    {
        VertexPos.xy = VertexPos.yx;
    }
    if(face == 5)
    {
        VertexPos.xyz = VertexPos.xzy;
        VertexPos.z++;
    }
    if(face == 4)
    {
        VertexPos.xyz = VertexPos.xzy;
    }

    VertexPos += pos - vec3(0, 16, 0);

	WorldPos = vec3(model * vec4(VertexPos, 1.0));

	gl_Position = projection * view * vec4(WorldPos, 1.0);
    instanceID = gl_InstanceID;
}