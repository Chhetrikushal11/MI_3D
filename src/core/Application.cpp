#include "core/Application.h"
#include "renderer/shader.h"
#include "core/Camera.h"
#include "core/Window.h"
#include "ui/UIManager.h"
#include "medical/DicomParser.h"
#include "medical/VolumeData.h"
#include "imgui.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

// --- Global Callbacks ---

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	static double lastX = 400.0, lastY = 300.0;
	static bool firstMouse = true;

	if (firstMouse) {
		lastX = xpos; lastY = ypos;
		firstMouse = false;
	}

	double deltaX = xpos - lastX;
	double deltaY = ypos - lastY;
	lastX = xpos; lastY = ypos;

	if (ImGui::GetIO().WantCaptureMouse) return;

	auto* app = static_cast<mi_3d::Application*>(glfwGetWindowUserPointer(window));
	if (!app) return;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		mi_3d::RenderMode mode = app->GetCurrentRenderMode();
		
		// In 3D or QuadView (bottom right), rotate the camera
		if (mode == mi_3d::RenderMode::Volume3D) {
			app->GetCamera().Rotate(static_cast<float>(deltaX) * 0.3f, static_cast<float>(deltaY) * 0.3f);
		}
		else if (mode == mi_3d::RenderMode::QuadView) {
			int winW, winH;
			glfwGetWindowSize(window, &winW, &winH);
			int drawW = winW - 250;
			if (xpos > (drawW / 2) && ypos > (winH / 2)) {
				app->GetCamera().Rotate(static_cast<float>(deltaX) * 0.3f, static_cast<float>(deltaY) * 0.3f);
			} else {
				app->UpdateSliceZ(static_cast<float>(deltaY) * 0.005f);
			}
		}
		else {
			app->UpdateSliceZ(static_cast<float>(deltaY) * 0.005f);
		}
	}
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (ImGui::GetIO().WantCaptureMouse) return;
	auto* app = static_cast<mi_3d::Application*>(glfwGetWindowUserPointer(window));
	if (!app) return;

	mi_3d::RenderMode mode = app->GetCurrentRenderMode();

	if (mode == mi_3d::RenderMode::Volume3D || mode == mi_3d::RenderMode::QuadView)
		app->GetCamera().Zoom(static_cast<float>(yoffset) * 0.5f);
	else
		app->UpdateSliceZ(static_cast<float>(yoffset) * 0.01f);
}

// callback for updating the size of the screen
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	auto* app = static_cast<mi_3d::Application*>(glfwGetWindowUserPointer(window));
	if (app) app->GetWindow().UpdateSize(width, height);
}

// to add the shortcut for using keyboard
void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS) return; // only respond on key press, not release
	
	if (ImGui::GetIO().WantCaptureKeyboard) return; // ImGui has focus

	auto* app = static_cast<mi_3d::Application*> (glfwGetWindowUserPointer(window));
	if (!app) return;

	switch (key)
	{
		case GLFW_KEY_1: app->SetRenderMode(mi_3d::RenderMode::Slice2D); break;
		case GLFW_KEY_2: app->SetRenderMode(mi_3d::RenderMode::Volume3D); break;
		case GLFW_KEY_3: app->SetRenderMode(mi_3d::RenderMode::QuadView); break;
		case GLFW_KEY_R: app->GetCamera().SetDefault();

		/*case GLFW_KEY_A: app->GetCamera().SetAzimuth(5.0f);*/
	}
}
namespace mi_3d
{
	Application::Application()
		: _mSliceZ(0.5f), _mWindowCenter(40.0f), _mWindowWidth(400.0f),
		  _mCurrentViewIndex(0), _mCurrentMode{ RenderMode::Slice2D }, _mStepSize(0.01f),
		  _mClipMin(0.0f), _mClipMax(1.0f)
	{}

	Application::~Application() { CleanupGPU(); }

	bool Application::Initialize(int width, int height, const std::string& title)
	{
		_mAppWindow = std::make_unique<Window>(width, height, title);
		_mAppCamera = std::make_unique<Camera>();

		DicomParser parser;
		std::cout << "Starting parse...." << std::endl;
		/*auto slices = parser.ParseDirectory("data/test_ct");*/
		// for testing 
		auto slices = parser.ParseDirectory("data/spinal_myeloma_real_ct");
		std::cout << "Parse done. Slices: " << slices.size() << std::endl;
		if (!slices.empty())
		{
			_mRescaleSlope = slices[0].rescaleSlope;
			_mRescaleIntercept = slices[0].rescaleIntercept;
		}

		_mVolume = std::make_unique<VolumeData>();
		std::cout << "Building volume..." << std::endl;
		_mVolume->Build(slices);
		std::cout << "Volume built." << std::endl;

		std::cout << "Uploading to GPU..." << std::endl;
		std::cout << "GPU upload done." << std::endl;
		// --------------------- DEBUG — check real data range --------------------------------
		int16_t globalMin = _mVolume->GetVoxels()[0];
		int16_t globalMax = _mVolume->GetVoxels()[0];
		for (auto v : _mVolume->GetVoxels()) {
			if (v < globalMin) globalMin = v;
			if (v > globalMax) globalMax = v;
		}
		std::cout << "Data range: " << globalMin << " to " << globalMax << std::endl;
		std::cout << "Rescale slope: " << slices[0].rescaleSlope << std::endl;
		std::cout << "Rescale intercept: " << slices[0].rescaleIntercept << std::endl;
		// -----------------------------------------------------------------------
		SetupVolumeTexture();
		SetupTransferFunction();
		SetupCallbacks();
		SetupSliceQuad();
		SetupCube();

		SetupFBO(_mEntryFBO, _mEntryTexture, _mEntryDepth, width, height);
		SetupFBO(_mExitFBO, _mExitTexture, _mExitDepth, width, height);

		_mSliceShader = std::make_unique<Shader>("shaders/slice.vert", "shaders/slice.frag");
		_mRaycastShader = std::make_unique<Shader>("shaders/raycast_entry.vert", "shaders/raycast_entry.frag");
		_mVolumeShader = std::make_unique<Shader>("shaders/slice.vert", "shaders/volume.frag");

		_mAppUI = std::make_unique<UIManager>(_mAppWindow->GetHandle());

		return true;
	}

	void Application::Run()
	{
		const char* viewModes[] = { "Axial", "Sagittal", "Coronal" };
		float sidebarWidth = 250.0f;

		while (!_mAppWindow->ShouldClose())
		{
			int winW = _mAppWindow->GetWidth();
			int winH = _mAppWindow->GetHeight();
			int drawW = winW - static_cast<int>(sidebarWidth);

			glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			_mAppUI->BeginFrame();

			// --- UI Sidebar ---
			ImGui::SetNextWindowPos(ImVec2((float)drawW, 0));
			ImGui::SetNextWindowSize(ImVec2(sidebarWidth, (float)winH));
			ImGui::Begin("mI-3d Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
				ImGui::Text("Viewport Mode");
				if (ImGui::RadioButton("2D Slice", _mCurrentMode == RenderMode::Slice2D)) _mCurrentMode = RenderMode::Slice2D;
				ImGui::SameLine();
				if (ImGui::RadioButton("Volume 3D", _mCurrentMode == RenderMode::Volume3D)) _mCurrentMode = RenderMode::Volume3D;
				ImGui::SameLine();
				if (ImGui::RadioButton("Quad View", _mCurrentMode == RenderMode::QuadView)) _mCurrentMode = RenderMode::QuadView;
				if (ImGui::Button("Reset")) _mAppCamera->SetDefault();
				ImGui::SameLine();
				if (ImGui::Button("Reset Slice")) _mSliceZ = 0.5f;
				ImGui::Separator();

				if (_mCurrentMode == RenderMode::Slice2D) {
					ImGui::Combo("Orientation", &_mCurrentViewIndex, viewModes, 3);
					ImGui::SliderFloat("Slice Z", &_mSliceZ, 0.0f, 1.0f);
				} 
				else if (_mCurrentMode == RenderMode::Volume3D || _mCurrentMode == RenderMode::QuadView) {
					ImGui::SliderFloat("Step Size", &_mStepSize, 0.001f, 0.05f);
					ImGui::SliderFloat("Slice/Crosshair", &_mSliceZ, 0.0f, 1.0f);
					if (ImGui::CollapsingHeader("Thresholding")) {
						ImGui::SliderFloat("Min HU", &_mClipMin, 0.0f, _mClipMax);
						ImGui::SliderFloat("Max HU", &_mClipMax, _mClipMin, 1.0f);
					}


				}
			ImGui::End();

			// --- Rendering Logic ---
			if (_mCurrentMode == RenderMode::Slice2D)
			{
				RenderSingleSlice(_mCurrentViewIndex, 0, 0, drawW, winH);
			}
			else if (_mCurrentMode == RenderMode::Volume3D)
			{
				glm::mat4 vp = _mAppCamera->GetProjectionMatrix((float)drawW / winH) * _mAppCamera->GetViewMatrix();
				RenderVolumeSide(vp, 0, 0, drawW, winH);
			}
			else if (_mCurrentMode == RenderMode::QuadView)
			{
				int hW = drawW / 2;
				int hH = winH / 2;
				glm::mat4 vp = _mAppCamera->GetProjectionMatrix((float)hW / hH) * _mAppCamera->GetViewMatrix();

				RenderSingleSlice(0, 0, hH, hW, hH);   // Top Left: Axial
				RenderSingleSlice(1, hW, hH, hW, hH);  // Top Right: Sagittal
				RenderSingleSlice(2, 0, 0, hW, hH);    // Bottom Left: Coronal
				RenderVolumeSide(vp, hW, 0, hW, hH);   // Bottom Right: 3D
			}

			_mAppUI->EndFrame();
			_mAppWindow->SwapBuffers();
			_mAppWindow->PollEvents();
		}
	}

	void Application::RenderSingleSlice(int viewMode, int x, int y, int width, int height)
	{
		glViewport(x, y, width, height);
		glDisable(GL_DEPTH_TEST);
		_mSliceShader->UseProgramID();
		_mSliceShader->SetUniformFloat("uRescaleSlope", _mRescaleSlope);
		_mSliceShader->SetUniformFloat("uRescaleIntercept", _mRescaleIntercept);
		_mSliceShader->SetUniformInt("uViewMode", viewMode);
		_mSliceShader->SetUniformFloat("uSliceZ", _mSliceZ);
		_mSliceShader->SetUniformFloat("uWindowCenter", _mWindowCenter);
		_mSliceShader->SetUniformFloat("uWindowWidth", _mWindowWidth);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, _mVolumeTexture);
		_mSliceShader->SetUniformInt("uVolume", 0);

		glBindVertexArray(_mSliceVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
	}

	void Application::RenderVolumeSide(const glm::mat4& vp, int x, int y, int width, int height)
	{
		// Pass 1: Entry Points
		glBindFramebuffer(GL_FRAMEBUFFER, _mEntryFBO);
		glViewport(0, 0, _mAppWindow->GetWidth(), _mAppWindow->GetHeight()); 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_CULL_FACE); glCullFace(GL_BACK);
		_mRaycastShader->UseProgramID();
		_mRaycastShader->SetUniformMat4("uViewProjection", vp);

		glBindVertexArray(_mVAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		// Pass 2: Exit Points
		glBindFramebuffer(GL_FRAMEBUFFER, _mExitFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glDisable(GL_CULL_FACE);

		// Pass 3: Raycasting to Screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(x, y, width, height); 
		_mVolumeShader->UseProgramID();

		_mVolumeShader->SetUniformFloat("uRescaleSlope", _mRescaleSlope);
		_mVolumeShader->SetUniformFloat("uRescaleIntercept", _mRescaleIntercept);

		glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, _mEntryTexture);
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, _mExitTexture);
		glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, _mVolumeTexture);
		glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_1D, _mTransferTexture);

		_mVolumeShader->SetUniformInt("uEntryTexture", 0);
		_mVolumeShader->SetUniformInt("uExitTexture", 1);
		_mVolumeShader->SetUniformInt("uVolume", 2);
		_mVolumeShader->SetUniformInt("uTransferFunc", 3);
		_mVolumeShader->SetUniformFloat("uStepSize", _mStepSize);
		_mVolumeShader->SetUniformFloat("uClipMin", _mClipMin);
		_mVolumeShader->SetUniformFloat("uClipMax", _mClipMax);

		glDisable(GL_DEPTH_TEST);
		glBindVertexArray(_mSliceVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
	}
	void Application::SetupVolumeTexture()
	{
		glGenTextures(1, &_mVolumeTexture);
		glBindTexture(GL_TEXTURE_3D, _mVolumeTexture);
		glTexImage3D(
			GL_TEXTURE_3D,
			0,
			GL_R16_SNORM,
			_mVolume->GetWidth(),
			_mVolume->GetHeight(),
			_mVolume->GetDepth(),
			0, GL_RED,
			GL_SHORT,
			_mVolume->GetVoxels().data());

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	void Application::SetupSliceQuad()
	{
		float quadVertices[] =
		{
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f,

			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f,
			-1.0f,  1.0f,  0.0f, 1.0f
		};

		glGenVertexArrays(1, &_mSliceVAO);
		glGenBuffers(1, &_mSliceVBO);

		glBindVertexArray(_mSliceVAO);
		glBindBuffer(GL_ARRAY_BUFFER, _mSliceVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);


		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
	}

	void Application::SetupFBO(GLuint& fbo, GLuint& colorTex, GLuint& depthRB, int width, int height)
	{

			// Step 1 & 2: Generate and Bind the Framebuffer
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			// Step 3: Create the Color Texture
			glGenTextures(1, &colorTex);
			glBindTexture(GL_TEXTURE_2D, colorTex);

			// Using GL_RGBA16F to match your comment - high precision for coordinates!
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

			// Step 4: Filtering and Wrapping
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			/* Why we need GL_TEXTURE_WRAP_S/T:
			   If a ray is calculated at the very edge (coordinate 0.0 or 1.0),
			   OpenGL might try to "repeat" the texture or blend with the opposite side.
			   GL_CLAMP_TO_EDGE forces the GPU to never grab data from outside the cube.
			*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			// Step 5: Attachment
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

			// Step 6: Depth Buffer (Required so the GPU knows which face is in front)
			glGenRenderbuffers(1, &depthRB);
			glBindRenderbuffer(GL_RENDERBUFFER, depthRB);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRB);

			// Final Check: status needs to be fetched from OpenGL
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				std::cerr << "ERROR: Framebuffer is not complete! Status: " << status << std::endl;
			}

			// Cleanup: Unbind to return to the default screen
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}

	void Application::SetupTransferFunction()
	{
		float transferData[256 * 4]; // 256 entries, each has R, G, B, A
		
		for (int i = 0; i < 256; i++)
		{
			float t = static_cast<float>(i) / 255.0f;
			float hu = t * 4096.0f - 1024.0f; // maps to -1024 to + 3071

			// now decide color based on HU value
			float r, g, b, a;

			if (hu < -500.0f)             // air and lungs
			{
				r = 0.0f; g = 0.0f; b = 0.0f; a = 0.0f; // transparent
			}

			else if (hu < 100.0f)             // soft tissue
			{
				r = 0.8f; g = 0.0f; b = 0.0f; a = 0.05f; // semi-transparent red
			}

			else if (hu < 700.0f)             // dense tissue
			{
				r = 0.9f; g = 0.7f; b = 0.5f; a = 0.05f; // semi-transparent tan
			}

			else                            // bone
			{
				r = 1.0f; g = 1.0f; b = 1.0f; a = 0.9f; //	opaque white
			}

			transferData[i * 4 + 0] = r;
			transferData[i * 4 + 1] = g;
			transferData[i * 4 + 2] = b;
			transferData[i * 4 + 3] = a;

		}

		glGenTextures(1, &_mTransferTexture);
		glBindTexture(GL_TEXTURE_1D, _mTransferTexture);
		glTexImage1D(
			GL_TEXTURE_1D,
			0,
			GL_RGBA,
			256,
			0,
			GL_RGBA,
			GL_FLOAT,
			transferData);

		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	}

	void Application::UpdateSliceZ(float delta)
	{
		_mSliceZ += delta;

		// constrain it between 0 and 1 so we dont go outside the volume
		if (_mSliceZ < 0.0f) _mSliceZ = 0.0f;
		if (_mSliceZ > 1.0f) _mSliceZ = 1.0f;
	}

	void Application::SetRenderMode(RenderMode mode)
	{
		_mCurrentMode = mode;
	}

	void Application::SetupCube()
	{
		float vertices[] = {
			// FRONT face (red) — z (positive)
			-0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,  // 0: bottom-left
			 0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,  // 1: bottom-right
			 0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,  // 2: top-right
			-0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,  // 3: top-left

			// BACK face (green) — z (negative)
			-0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,  // 4
			 0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,  // 5
			 0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,  // 6
			-0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,  // 7

			// TOP face (blue) — y (positive)
			-0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,  // 8
			 0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,  // 9
			 0.5f,  0.5f, -0.5f,   0.0f, 0.0f, 1.0f,  // 10
			-0.5f,  0.5f, -0.5f,   0.0f, 0.0f, 1.0f,  // 11

			// BOTTOM face (yellow) — y (positive)
			-0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,  // 12
			 0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,  // 13
			 0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,  // 14
			-0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,  // 15

			// RIGHT face (magenta) — x (positive)
			 0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,  // 16
			 0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,  // 17
			 0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 1.0f,  // 18
			 0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 1.0f,  // 19

			 // LEFT face (cyan) — x (negative)
			 -0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 1.0f,  // 20
			 -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 1.0f,  // 21
			 -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,  // 22
			 -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f   // 23
		};


		unsigned int indices[] =
		{
			// front 
			0, 1, 2, 2, 3, 0,
			// back 
			5, 4, 7, 7, 6, 5,
			// top 
			8, 9, 10, 10, 11, 8,
			// bottom
			 13, 12, 15, 15, 14, 13,
			 // right
			  16, 17, 18, 18, 19, 16,
			  // left
			   21, 20, 23, 23, 22, 21
		};

		// to load vertices on  the GPU we need VAO and VBO

		glGenVertexArrays(1, &_mVAO);
		glGenBuffers(1, &_mVBO);


		glBindVertexArray(_mVAO);
		glBindBuffer(GL_ARRAY_BUFFER, _mVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glGenBuffers(1, &_mEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		std::cout << "VAO/VBO setup complete. VAO ID: " << _mVAO << " VBO ID: " << _mVBO << std::endl;
	}

	void Application::SetupCallbacks()
	{
		glfwSetWindowUserPointer(_mAppWindow->GetHandle(), this);
		glfwSetCursorPosCallback(_mAppWindow->GetHandle(), mouseCallback);
		glfwSetScrollCallback(_mAppWindow->GetHandle(), scrollCallback);
		glfwSetFramebufferSizeCallback(_mAppWindow->GetHandle(), framebufferSizeCallback);
		glfwSetKeyCallback(_mAppWindow->GetHandle(), KeyboardCallback);
	}

	void Application::CleanupGPU()
	{
		glDeleteVertexArrays(1, &_mVAO);
		glDeleteBuffers(1, &_mVBO);
		glDeleteBuffers(1, &_mEBO);
		glDeleteTextures(1, &_mVolumeTexture);
		glDeleteVertexArrays(1, &_mSliceVAO);
		glDeleteBuffers(1, &_mSliceVBO);

		glDeleteFramebuffers(1, &_mEntryFBO);
		glDeleteTextures(1, &_mEntryTexture);
		glDeleteRenderbuffers(1, &_mEntryDepth);
		glDeleteFramebuffers(1, &_mExitFBO);
		glDeleteTextures(1, &_mExitTexture);
		glDeleteRenderbuffers(1, &_mExitDepth);
	}
}