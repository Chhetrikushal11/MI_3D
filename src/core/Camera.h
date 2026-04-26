#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // that BUILD matrices.

namespace mi_3d
{
	class Camera
	{
	public:
		Camera(float distance = 5.0f, float azimuth = 0.0f, float elevation = 20.0f);
		// ____________________ FOR VIEW________________________
		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix(float aspectRatio) const;
		// _________________ FOR TRANSFORMATION ________________
		void Rotate(float deltaAzimuth, float deltaElevation);
		void Zoom(float deltaDistance);

		// ----------------- GETTER FUNCTION --------------------------
		float GetDistance() const { return _mDistance; }
		float GetAzimuth() const { return _mAzimuth; }
		float GetElevation() const { return _mElevation; }

		// --------------- SETTER FUNCTION ---------------------------------
		void SetDistance(float distance);
		void SetAzimuth(float azimuth);
		void SetElevation(float elevation);
		void SetDefault();
	private:
		glm::vec3 _mCenter;
		float _mDistance;
		float _mAzimuth;
		float _mElevation;
		float _mFOV;
		float _mNearPlane;
		float _mFarPlane;

		glm::vec3 CalculatePosition() const; // for calculating current position of camera.	
	};
} // end mi_3d
