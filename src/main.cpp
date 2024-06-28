#include "main.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "shader.h"

#define DB_PERLIN_IMPL
#include "perlin.h"

u32 ScreenWidth = 1920;
u32 ScreenHeight = 1080;

u32 ChunkDim = 16;
u32 ChunkDim2 = ChunkDim * ChunkDim;
u32 ChunkDim3 = ChunkDim * ChunkDim * ChunkDim;

v3 camPos = v3(0, 0, 3);
v3 camFront = v3(0, 0, -1);
v3 camUp = v3(0, 1, 0);

f32 lastX = 400;
f32 lastY = 300;

f32 yaw = +50;
f32 pitch = 0;

u32 frameCount = 0;
f32 dt = 0.0f;
f32 lastFrameTime = 0.0f;

b32 firstMouse = true;
f32 fov = 90.0f;

u32 materialCount = 6;

std::random_device rd;
std::mt19937 rng(rd());
std::uniform_real_distribution<> random(0, materialCount);

void GLFWFramebufferSizeCallback(GLFWwindow* window, s32 width, s32 height)
{
	glViewport(0, 0, width, height);
}

void GLFWScrollCallback(GLFWwindow* window, f64 xOffset, f64 yOffset)
{
	fov -= (f32)yOffset;
	if (fov < 1)    fov = 1;
	if (fov > 120)  fov = 120;
}

void GLFWMouseCallback(GLFWwindow* window, f64 x, f64 y)
{
	if (firstMouse)
	{
		lastX = (f32)x;
		lastY = (f32)y;
		firstMouse = false;
	}

	f32 xOffset = (f32)x - lastX;
	f32 yOffset = lastY - (f32)y;
	lastX = (f32)x;
	lastY = (f32)y;

	f32 sensitivity = 0.1f;
	xOffset *= sensitivity;
	yOffset *= sensitivity;

	yaw += xOffset;
	pitch += yOffset;

#if 1
	if (pitch > 89.0f)
	{
		pitch = 89.0f;
	}
	if (pitch < -89.0f)
	{
		pitch = -89.0f;
	}
#endif

	v3 camDir;
	camDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	camDir.y = sin(glm::radians(pitch));
	camDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	camFront = glm::normalize(camDir);
}

void ProcessInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	float camSpeed = 2.5f * dt;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		camSpeed = 10.0f * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		camSpeed = 500.0f * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		camPos += camSpeed * camFront;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		camPos -= camSpeed * camFront;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		camPos -= glm::normalize(glm::cross(camFront, camUp)) * camSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		camPos += glm::normalize(glm::cross(camFront, camUp)) * camSpeed;
	}
}

struct CubeFace
{
	i3 pos;
	u32 face;
	u32 material;
	v3 a;
};

u32 ChunkArrayIndex(u32 x, u32 y, u32 z)
{
	return x + y*ChunkDim + z*ChunkDim2;
}

u32 ChunkArrayIndex(i3 pos)
{
	return ChunkArrayIndex(pos.x, pos.y, pos.z);
}

enum FaceDirection : u32
{
	Left, Right, Bottom, Top, Back, Front
};

v3 FaceDirectionToNormal(FaceDirection faceDirection)
{
	switch (faceDirection)
	{
	case Left:	  return { -1,0,0 };
	case Right:	  return { +1,0,0 };
	case Bottom:  return { 0,-1,0 };
	case Top:	  return { 0,+1,0 };
	case Back:	  return { 0,0,-1 };
	case Front:	  return { 0,0,+1 };
	};
}

std::vector<CubeFace> ConstructChunk(u32* voxels, i3 chunkPos = i3(0))
{
	std::vector<CubeFace> faces = std::vector<CubeFace>();

	for (u32 z = 0; z < ChunkDim; z++)
	{
		for (u32 y = 0; y < ChunkDim; y++)
		{
			for (u32 x = 0; x < ChunkDim; x++)
			{
				if (voxels[ChunkArrayIndex(x, y, z)] != 0)
				{
					CubeFace cubeFace;
					cubeFace.pos = i3(x,y,z) + (s32)ChunkDim*chunkPos;
					//cubeFace.material = 0;//random(rng);
					u32 directions[6] = {};

					for (u32 i = 0; i < 6; i++)
					{
						FaceDirection direction = (FaceDirection)i;
						i3 neighborPos = i3(x, y, z) + (i3)FaceDirectionToNormal(direction);
						if (neighborPos.x < 0 || neighborPos.y < 0 || neighborPos.z < 0
						||  neighborPos.x >= ChunkDim || neighborPos.y >= ChunkDim || neighborPos.z >= ChunkDim)
						{
							directions[i] = 1; // 0: Put face at boundaries, 1: Don't
						}
						else
						{
							directions[i] = voxels[ChunkArrayIndex( neighborPos )];
						}

						if (!directions[i])
						{
							cubeFace.face = i;
							faces.push_back(cubeFace);
						}
					}
				}
			}
		}
	}

	return faces;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWwindow* window = glfwCreateWindow(ScreenWidth, ScreenHeight, "Nanovox", 0, 0);
	if (!window)
	{
		printf("Failed to create a window.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, GLFWFramebufferSizeCallback);
	glfwSetCursorPosCallback(window, GLFWMouseCallback);
	glfwSetScrollCallback(window, GLFWScrollCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to load GL functions.\n");
		return -1;
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glEnable(GL_DEPTH_TEST);

	u32 vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLFWMouseCallback(window, ScreenWidth/2, ScreenHeight/2); // Initialize camera facing and mouse deltas

	u32 ChunkCountAcrossOneAxis = 20;

	u32* voxels = (u32*)malloc(ChunkCountAcrossOneAxis * ChunkCountAcrossOneAxis * ChunkDim3 * sizeof(u32));

	for (u32 chunkZ = 0; chunkZ < ChunkCountAcrossOneAxis; chunkZ++)
	{
		for (u32 chunkX = 0; chunkX < ChunkCountAcrossOneAxis; chunkX++)
		{
			for (u32 z = 0; z < ChunkDim; z++)
			{
				for (u32 y = 0; y < ChunkDim; y++)
				{
					for (u32 x = 0; x < ChunkDim; x++)
					{
						s32 xPos = x + chunkX*ChunkDim;
						s32 yPos = y;
						s32 zPos = z + chunkZ*ChunkDim;

						static f32 displacement = 0.0f;
						displacement += 0.000001f * dt;
						f32 frequency = 0.02f;
						f32 noise = db::perlin(xPos * frequency + displacement, yPos * frequency + displacement, zPos * frequency);
						f32 vertical = -((y / (float)ChunkDim) - 0.5f);
						vertical *= vertical * vertical;
						voxels[ChunkArrayIndex(x, y, z) + chunkX * ChunkDim3 + chunkZ * ChunkDim3 * ChunkCountAcrossOneAxis] = (0.1f * noise + vertical < 0.0) ? 1 : 0;
					}
				}
			}
		}
	}

	std::vector<CubeFace> faces;
	for (u32 chunkIndex = 0; chunkIndex < ChunkCountAcrossOneAxis*ChunkCountAcrossOneAxis; chunkIndex++)
	{
		std::vector<CubeFace> chunkFaces = ConstructChunk(&voxels[0 + chunkIndex * ChunkDim3], i3(chunkIndex%ChunkCountAcrossOneAxis, 0, chunkIndex/ChunkCountAcrossOneAxis));
		faces.insert(faces.end(), chunkFaces.begin(), chunkFaces.end());
	}

	u32 dirCounts[6] = {};
	for (u32 i = 0; i < faces.size(); i++)
	{
		CubeFace cubeFace = faces[i];
		dirCounts[cubeFace.face]++;
	}

	for (u32 i = 0; i < 6; i++)
	{
		printf("Face %d count: %d\n", i, dirCounts[i]);
	}

	u32 ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, faces.size() * sizeof(CubeFace), (void*)faces.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);

	Shader shader("shaders/base.vert", "shaders/base.frag");

	shader.Bind();

	f32 near = 0.1f;
	f32 far = 100.0f;

	mat4 projection = glm::perspective(glm::radians(90.0f), (float)ScreenWidth / (float)ScreenHeight, near, far);
	shader.Set("projection", projection);
	shader.Set("model", mat4(1.0f));
	shader.Set("near", near);
	shader.Set("far", far);

	glViewport(0, 0, ScreenWidth, ScreenHeight);
	v3 skyColor = { 0.082f, 0.721f, 0.901f };
	glClearColor(skyColor.x, skyColor.y, skyColor.z, 1);
	shader.Set("fogColor", skyColor);

	while (!glfwWindowShouldClose(window))
	{
		f32 time = (f32)glfwGetTime();
		dt = time - lastFrameTime;
		lastFrameTime = time;
		frameCount++;

		if (frameCount % (1<<5) == 0)
		{
			printf("Frametime: %.3fms, %.1f FPS\n", dt * 1000.0f, 1/(dt));
		}

		ProcessInput(window);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
		shader.Set("view", view);
			
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, faces.size());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}