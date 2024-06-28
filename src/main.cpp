#include "main.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "shader.h"

#define DB_PERLIN_IMPL
#include "perlin.h"

#include <thread>
#include <mutex>
#include <chrono>

u32 ScreenWidth = 1920/2;
u32 ScreenHeight = 1080/2;

u32 ChunkDim = 32;
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

std::mutex chunkPushMutex;

struct CubeFace
{
	i4 data; // XYZ pos, W: ----Mat-Face
};

struct Chunk
{
	i3 chunkPos;
	std::vector<CubeFace> faces;
};

u32 materialCount = 6;

std::vector<Chunk> chunks;

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

u32 ChunkArrayIndex(u32 x, u32 y, u32 z)
{
	return x + y*ChunkDim + z*ChunkDim2;
}

u32 SampleVoxel(i3 pos, i3 chunkPos)
{
	i3 samplePos = pos + chunkPos * (s32)ChunkDim;

	f32 frequency = 0.02f;
	f32 noise = db::perlin(samplePos.x * frequency, samplePos.y * frequency, samplePos.z * frequency);
	f32 vertical = -((pos.y / (float)ChunkDim) - 0.5f);
	vertical *= vertical * vertical;
	return (0.1f * noise + vertical < 0.0) ? 1 : 0;
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
					cubeFace.data = i4(i3(x,y,z) + (s32)ChunkDim*chunkPos, 0);
					u32 directions[6] = {};

					for (u32 i = 0; i < 6; i++)
					{
						FaceDirection direction = (FaceDirection)i;
						i3 neighborPos = i3(x, y, z) + (i3)FaceDirectionToNormal(direction);
						if (neighborPos.x < 0 || neighborPos.y < 0 || neighborPos.z < 0
						||  neighborPos.x >= ChunkDim || neighborPos.y >= ChunkDim || neighborPos.z >= ChunkDim)
						{
							directions[i] = SampleVoxel(neighborPos, chunkPos);
						}
						else
						{
							directions[i] = voxels[ChunkArrayIndex( neighborPos )];
						}

						if (!directions[i])
						{
							cubeFace.data.w = 0;
							cubeFace.data.w |= i;
							
							f32 noise = db::perlin(x * 0.4684f, y * 0.684f, z * 0.4684f);
							u32 material = floor((noise+1)*0.5f * materialCount);
							cubeFace.data.w |= (material << 3);
							faces.push_back(cubeFace);
						}
					}
				}
			}
		}
	}

	return faces;
}

u32* GenerateChunkFilledness(i3 chunkPos)
{
	u32* voxels = (u32*)malloc(ChunkDim3 * sizeof(u32));

	for (u32 z = 0; z < ChunkDim; z++)
	for (u32 y = 0; y < ChunkDim; y++)
	for (u32 x = 0; x < ChunkDim; x++)
	{
		voxels[ChunkArrayIndex(x, y, z)] = SampleVoxel({ x,y,z }, chunkPos);
	}

	return voxels;
}

b32 TrySpawnChunks(s32 kernelSize)
{
	v3 camPosXZ = v3(camPos.x, 0, camPos.z);
	i3 camChunkPos = camPosXZ / (f32)ChunkDim;

	b32 generatedAChunk = false;
	for (s32 offsetZ = -kernelSize; offsetZ <= kernelSize; offsetZ++)
	{
		for (s32 offsetX = -kernelSize; offsetX <= kernelSize; offsetX++)
		{
			b32 hasChunkHere = false;
			for (u32 i = 0; i < chunks.size(); i++)
			{
				if (chunks[i].chunkPos == camChunkPos + i3(offsetX, 0, offsetZ))
				{
					hasChunkHere = true;
					break;
				}
			}

			i3 newChunkPos = camChunkPos + i3(offsetX, 0, offsetZ);

			v3 chunkCenter = v3(newChunkPos * (s32)ChunkDim) + v3(ChunkDim/2);
			v3 chunkCenterDirFromCam = glm::normalize(chunkCenter - camPos);
			b32 probablyInView = glm::dot(camFront, chunkCenterDirFromCam) > 0;
			b32 tooClose = glm::distance((v3)camChunkPos, (v3)newChunkPos) < 20;
			b32 shouldRender = probablyInView | tooClose;

			if (!hasChunkHere && shouldRender)
			{
				u32* chunkFilledness = GenerateChunkFilledness(newChunkPos);
				Chunk chunk;
				chunk.chunkPos = newChunkPos;
				if(chunkPushMutex.try_lock())
				{
					chunk.faces = ConstructChunk(chunkFilledness, newChunkPos);
					chunks.push_back(chunk);
					chunkPushMutex.unlock();
				}
				generatedAChunk = true;
			}
		}
	}

	if (!generatedAChunk && kernelSize < 20)
	{
		TrySpawnChunks(kernelSize + 4);
	}
	
	return true;
}

void ChunkGeneratorThread(GLFWwindow* window)
{
	while (!glfwWindowShouldClose(window))
	{
		using namespace std::chrono_literals;
		TrySpawnChunks(1);
		std::this_thread::sleep_for(100ms);
	}
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
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
	glEnable(GL_CULL_FACE);

	u32 vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLFWMouseCallback(window, ScreenWidth/2, ScreenHeight/2); // Initialize camera facing and mouse deltas

	Shader shader("shaders/base.vert", "shaders/base.frag");

	shader.Bind();

	f32 near = 0.1f;
	f32 far = 500.0f;

	mat4 projection = glm::perspective(glm::radians(90.0f), (float)ScreenWidth / (float)ScreenHeight, near, far);
	shader.Set("projection", projection);
	shader.Set("model", mat4(1.0f));
	shader.Set("near", near);
	shader.Set("far", far);

	glViewport(0, 0, ScreenWidth, ScreenHeight);
	v3 skyColor = { 0.082f, 0.721f, 0.901f };
	glClearColor(skyColor.x, skyColor.y, skyColor.z, 1);
	shader.Set("fogColor", skyColor);

	u32 ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

	u32 ChunkCountAcrossOneAxis = 10;

	std::thread chunkThreads[19];
	for (u32 i = 0; i < 19; i++)
	{
		chunkThreads[i] = std::thread(ChunkGeneratorThread, window);
	}

	while (!glfwWindowShouldClose(window))
	{
		f32 time = (f32)glfwGetTime();
		dt = time - lastFrameTime;
		lastFrameTime = time;
		frameCount++;

		//if (frameCount % (1<<5) == 0)
		//{
		//	printf("Frametime: %.3fms, %.1f FPS, Chunk count: %d\n", dt * 1000.0f, 1/(dt), chunks.size());
		//}

		ProcessInput(window);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
		shader.Set("view", view);
			
		for (u32 i = 0; i < chunks.size(); i++)
		{
			v3 worldPos = chunks[i].chunkPos * (s32)ChunkDim;
			v3 camPosXZ = { camPos.x, 0, camPos.z };
			f32 distance = glm::distance(worldPos, camPosXZ);
			if (distance < 500)
			{
				std::vector<CubeFace> faces = chunks[i].faces;
				glBufferData(GL_SHADER_STORAGE_BUFFER, faces.size() * sizeof(CubeFace), (void*)faces.data(), GL_DYNAMIC_DRAW);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);

				glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, faces.size());
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	for (u32 i = 0; i < 19; i++)
	{
		chunkThreads[i].join();
	}
	glfwTerminate();
	return 0;
}