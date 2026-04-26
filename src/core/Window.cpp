#include "core/Window.h"

mi_3d::Window::Window(int width, int height, const std::string& title)
	:_mWidth{width},
_mHeight{height}
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;

	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// creating the temprorary window
	_mWindow = glfwCreateWindow(_mWidth, _mHeight, title.c_str(), nullptr, nullptr);
	if (!_mWindow)
	{
		std::cerr << "Failed to create a GLFW window" << std::endl;
		glfwTerminate();

	}
	glfwMakeContextCurrent(_mWindow);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();

	}

	std::cout << "GLAD loaded successfully!" << std::endl;
	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
}

mi_3d::Window::~Window()
{
	glfwDestroyWindow(_mWindow);
	glfwTerminate();
}

bool mi_3d::Window::ShouldClose() const
{
	return glfwWindowShouldClose(_mWindow);
}

void mi_3d::Window::SwapBuffers()
{
	glfwSwapBuffers(_mWindow);
}

void mi_3d::Window::PollEvents()
{
	glfwPollEvents();
}

GLFWwindow* mi_3d::Window::GetHandle() const
{
	return _mWindow;
}

float mi_3d::Window::GetAspectRatio() const
{
	return static_cast<float>(_mWidth) / static_cast<float>(_mHeight);
	
}

void mi_3d::Window::UpdateSize(int width, int height)
{
	_mWidth = width;
	_mHeight = height;
}
