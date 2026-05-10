#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace mi_3d
{
	class Shader
	{
	public:
		// constructor with the parameters
		Shader(const std::string& vertexPath, const std::string& fragementPath);
		~Shader();

		void UseProgramID() const;
		GLuint GetID() const { return _mProgramID; }
		// we will not make the copy of the constructor
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;

		void SetUniformMat4(const std::string& name, const glm::mat4& matrix) const;

		// for slice shader
		void SetUniformFloat(const std::string& name, float value) const;

		void SetUniformInt(const std::string& name, int value) const;
	private:
		// for ticketing purpose
		GLuint _mProgramID = 0; // default value is zero;

		std::string ReadFile(const std::string& filepath) const;
		GLuint CompileShader(GLenum type, const std::string& source) const;
		GLuint LinkProgram(GLuint vertexShader, GLuint fragementShader) const;
	};
}