#include "ui/UIManager.h"
#include "core/Camera.h"

namespace mi_3d
{
	UIManager::UIManager(GLFWwindow* window)
	{
		// FIRST CHECK:	Version
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}
	UIManager::~UIManager()
	{
		// Destroy ImplOpengGL3_Shutdown()
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void UIManager::BeginFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


	}

	void UIManager::RenderDebugPanel(Camera& camera)
	{
		ImGui::Begin("Debug");
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);


		float dist = camera.GetDistance();
		float az = camera.GetAzimuth();
		float el = camera.GetElevation();

		if (ImGui::SliderFloat("Distance", &dist, 0.5f, 50.0f))
			camera.SetDistance(dist);
		if (ImGui::SliderFloat("Azimuth", &az, -180.0f, 180.0f))
			camera.SetAzimuth(az);
		if (ImGui::SliderFloat("Elevation", &el, -89.0f, 89.0f))
			camera.SetElevation(el);
		if (ImGui::Button("Reset Camera"))
			camera.SetDefault();
		ImGui::End();
	}

	void UIManager::EndFrame()
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

}