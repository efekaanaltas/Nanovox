#include "shader.h"

Shader::Shader(const char* vertexPath, const char* fragPath, const char* geometryPath)
{
	std::string vertexCode;
	std::string fragCode;
	std::string geometryCode;
	std::ifstream vShaderFile(vertexPath, std::ios::in | std::ios::binary);
	std::ifstream fShaderFile(fragPath, std::ios::in | std::ios::binary);
	std::ifstream gShaderFile;
	if (geometryPath)
	{
		gShaderFile = std::ifstream(geometryPath, std::ios::in | std::ios::binary);
	}
	std::stringstream vShaderStream;
	std::stringstream fShaderStream;
	std::stringstream gShaderStream;

	if (vShaderFile)
	{
		vShaderStream << vShaderFile.rdbuf();
		vertexCode = vShaderStream.str();
		vShaderFile.close();
	}
	else
	{
		printf("Couldn't open vertex shader file.");
		return;
	}

	if (fShaderFile)
	{
		fShaderStream << fShaderFile.rdbuf();
		fragCode = fShaderStream.str();
		fShaderFile.close();
	}
	else
	{
		printf("Couldn't open fragment shader file.");
		return;
	}

	if (geometryPath)
	{
		if (gShaderFile)
		{
			gShaderStream << gShaderFile.rdbuf();
			geometryCode = gShaderStream.str();
			gShaderFile.close();
		}
		else
		{
			printf("Couldn't open geometry shader file.");
			return;
		}
	}

	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragCode.c_str();
	const char* gShaderCode = geometryCode.c_str();

	u32 vertex;
	u32 frag;
	u32 geometry;

	s32 success;
	char infoLog[512];

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, 0);
	glCompileShader(vertex);

	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(vertex, 512, 0, infoLog);
		printf("%s compilation failed: %s\n", vertexPath, infoLog);
	}

	frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fShaderCode, 0);
	glCompileShader(frag);

	glGetShaderiv(frag, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(frag, 512, 0, infoLog);
		printf("%s compilation failed: %s\n", fragPath, infoLog);
	}

	if (geometryPath)
	{
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &gShaderCode, 0);
		glCompileShader(geometry);

		glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			glGetShaderInfoLog(geometry, 512, 0, infoLog);
			printf("%s compilation failed: %s\n", geometryPath, infoLog);
		}
	}

	id = glCreateProgram();
	glAttachShader(id, vertex);
	glAttachShader(id, frag);
	if (geometryPath)
	{
		glAttachShader(id, geometry);
	}
	glLinkProgram(id);

	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(id, 512, 0, infoLog);
		printf("Linking %s and %s failed.\n", vertexPath, fragPath);
	}

	glDeleteShader(vertex);
	glDeleteShader(frag);
}

void Shader::Bind()
{
	glUseProgram(id);
}

u32 loc(std::string name, u32 id)
{
	return glGetUniformLocation(id, name.c_str());
}

void Shader::Set(std::string name, s32 value)
{
	glUniform1i(loc(name, id), value);
}

void Shader::Set(std::string name, f32 value)
{
	glUniform1f(loc(name, id), value);
}

void Shader::Set(std::string name, v3 value)
{
	glUniform3fv(loc(name, id), 1, &value[0]);
}

void Shader::Set(std::string name, f32 x, f32 y, f32 z)
{
	glUniform3f(loc(name, id), x, y, z);
}

void Shader::Set(std::string name, v4 value)
{
	glUniform4fv(loc(name, id), 1, &value[0]);
}

void Shader::Set(std::string name, mat3 value)
{
	glUniformMatrix3fv(loc(name, id), 1, GL_FALSE, &value[0][0]);
}

void Shader::Set(std::string name, mat4 value)
{
	glUniformMatrix4fv(loc(name, id), 1, GL_FALSE, &value[0][0]);
}