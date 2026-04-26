#include "renderer/shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace mi_3d
{
	Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
	{
		// __________________ READING THE FILE________________________
		std::string vertexSource = ReadFile(vertexPath);
		std::string fragmentSource = ReadFile(fragmentPath);

		if (vertexSource.empty() || fragmentSource.empty())
		{
			std::cerr << "Shader Error: Failed to read shader files" << std::endl;
			return;
		}
		// __________________ COMPILING THE FILE________________________
		GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
		GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

		if (vertexShader == 0 || fragmentShader == 0)
		{
			std::cerr << "Shader Error: Failed to compile shaders" << std::endl;
			return;
		}
		// __________________ LINKING  THE FILE________________________
		_mProgramID = LinkProgram(vertexShader, fragmentShader);
	}
	Shader::~Shader()
	{
		glDeleteProgram(_mProgramID);
	}
	void Shader::UseProgramID() const
	{
		glUseProgram(_mProgramID);
	}

	void Shader::SetUniformMat4(const std::string& name, const glm::mat4& matrix) const
	{
		GLint location = glGetUniformLocation(_mProgramID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
	}

	void Shader::SetUniformFloat(const std::string& name, float value) const
	{
		GLint location = glGetUniformLocation(_mProgramID, name.c_str());
		glUniform1f(location, value);
	}

	void Shader::SetUniformInt(const std::string& name, int value) const
	{
		GLint location = glGetUniformLocation(_mProgramID, name.c_str());
		glUniform1i(location, value);
	}

	std::string Shader::ReadFile(const std::string& filepath)const
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			std::cerr << "Shader Error: Could not open file:" << filepath << std::endl;
			return "";
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	GLuint Shader::CompileShader(GLenum type, const std::string& source) const
	{

		GLuint shader = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			std::cerr << "Shader Compilation Error ("
				<< (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
				<< "):\n" << infoLog << std::endl;
			glDeleteShader(shader);
			return 0;
		}

		return shader;

	}

	GLuint Shader::LinkProgram(GLuint vertexShader, GLuint fragmentShader) const
	{
		GLuint program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			char infoLog[512];
			glGetProgramInfoLog(program, 512, nullptr, infoLog);
			std::cerr << "Shader Link Error:\n" << infoLog << std::endl;
			glDeleteProgram(program);
			return 0;
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return program;
	}

} // end of mi_3D