#pragma once
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace mi_3d
{
	class Camera; // forward declaring Camera to use the camera member function
	class UIManager
	{
	public:
		UIManager(GLFWwindow* window);
		~UIManager();

		// delete copy constructor and operator
		UIManager(const UIManager&) = delete;
		UIManager& operator=(const UIManager&) = delete;

		void BeginFrame();
		void RenderDebugPanel(Camera& camera);
		void EndFrame();


	};
}