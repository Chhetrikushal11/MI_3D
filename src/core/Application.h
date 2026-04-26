#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <memory>
#include "core/Camera.h"

namespace mi_3d
{
	class Window;
	class UIManager;
	class VolumeData;
	class Shader;

	enum class RenderMode {
		Slice2D,
		Volume3D,
		QuadView
	};

	class Application
	{
	public:
		Application();
		~Application();

		// Disable Copying
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		bool Initialize(int width, int height, const std::string& title);
		void Run();

		void SetupVolumeTexture();
		void SetupSliceQuad();
		void SetupFBO(GLuint& fbo, GLuint& colorTex, GLuint& depthRB, int width, int height);
		void SetupTransferFunction();

		// Getters
		RenderMode GetCurrentRenderMode() const { return _mCurrentMode; }
		Camera& GetCamera() const { return *_mAppCamera; }
		Window& GetWindow() const { return *_mAppWindow;  }
		float GetSliceZ() const { return _mSliceZ; }

		// Logic
		void UpdateSliceZ(float delta);

		// seting up render mode
		void SetRenderMode(RenderMode mode);

	private:
		// Smart Pointers
		std::unique_ptr<Window> _mAppWindow;
		std::unique_ptr<Camera> _mAppCamera;
		std::unique_ptr<UIManager> _mAppUI;
		std::unique_ptr<VolumeData> _mVolume;

		// Shaders
		std::unique_ptr<Shader> _mAppShader;
		std::unique_ptr<Shader> _mSliceShader;
		std::unique_ptr<Shader> _mRaycastShader;
		std::unique_ptr<Shader> _mDebugShader;
		std::unique_ptr<Shader> _mVolumeShader;

		// OpenGL Handles
		GLuint _mVAO = 0, _mVBO = 0, _mEBO = 0;
		GLuint _mEntryFBO = 0, _mEntryTexture = 0, _mEntryDepth = 0;
		GLuint _mExitFBO = 0, _mExitTexture = 0, _mExitDepth = 0;
		GLuint _mVolumeTexture = 0;
		GLuint _mTransferTexture = 0;
		GLuint _mSliceVAO = 0, _mSliceVBO = 0;

		// Medical Imaging State
		float _mSliceZ = 0.5f;
		float _mWindowCenter = 40.0f;
		float _mWindowWidth = 400.0f;
		float _mStepSize = 0.005f;
		float _mClipMin = 0.0f;
		float _mClipMax = 1.0f;

		int _mCurrentViewIndex = 0;
		RenderMode _mCurrentMode = RenderMode::Slice2D;

		// Internal Helpers
		void SetupCube();
		void SetupCallbacks();
		void CleanupGPU();

		void RenderSingleSlice(int viewMode, int x, int y, int width, int height);
		void RenderVolumeSide(const glm::mat4& vp, int x, int y, int width, int height);
	};
}