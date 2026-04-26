#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace mi_3d
{
	class Window
	{
	public:
		Window(int width, int height, const  std::string& title);
		~Window();

			// delete copy constructor and operator
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		bool ShouldClose() const;
		void SwapBuffers();
		void PollEvents();
		GLFWwindow* GetHandle() const;
		float GetAspectRatio() const;

		// for window dimension
		unsigned int GetWidth() const { return _mWidth; }
		unsigned int GetHeight() const { return _mHeight; }

		// to resize the winow 
		void UpdateSize(int width, int height);

		int _mWidth;
		int _mHeight;
		
		GLFWwindow* _mWindow;
	};
}