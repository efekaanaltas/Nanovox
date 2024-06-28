#pragma once

#include "glad/glad.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "types.h"

struct Shader
{
	u32 id;

	Shader(const char* vertexPath, const char* fragPath, const char* geometryPath = nullptr);

	void Bind();

	void Set(std::string name, s32 value);
	void Set(std::string name, f32 value);
	void Set(std::string name, v3 value);
	void Set(std::string name, f32 x, f32 y, f32 z);
	void Set(std::string name, v4 value);
	void Set(std::string name, mat3 value);
	void Set(std::string name, mat4 value);
};