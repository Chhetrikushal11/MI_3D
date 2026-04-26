#include "core/Camera.h"





namespace mi_3d
{
	Camera::Camera(float distance, float azimuth, float elevation)
		:_mCenter{ 0.0f, 0.0f, 0.0f},
		_mDistance{ distance },
		_mAzimuth{azimuth},
		_mElevation(elevation),
		_mFOV(45.0f),
		_mNearPlane(0.1f),
		_mFarPlane(100.0f)
	{
	}



	glm::vec3 Camera::CalculatePosition() const
	{
		float azimutRad = glm::radians(_mAzimuth);
		float elevationRad = glm::radians(_mElevation);

		float x = _mDistance * glm::cos(elevationRad) * glm::sin(azimutRad);
		float y = _mDistance * glm::sin(elevationRad);
		float z = _mDistance * glm::cos(elevationRad) * glm::cos(azimutRad);

		return _mCenter + glm::vec3(x, y, z);

	}

	glm::mat4 Camera::GetViewMatrix() const
	{
		// first get the position of the camera
		glm::vec3 position = CalculatePosition();
		return glm::lookAt(position,_mCenter, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
	{
		return glm::perspective(glm::radians(_mFOV), aspectRatio,_mNearPlane, _mFarPlane);
	}

	void Camera::Rotate(float deltaAzimuth, float deltaElevation)
	{
		// this manipulate the where camera is focusing.
		_mAzimuth += deltaAzimuth;
		_mElevation += deltaElevation;

		if (_mElevation > 89.0f) _mElevation = 89.0f;
		if (_mElevation < -89.0f) _mElevation = -89.0f;
	}

	void Camera::Zoom(float deltaDistance)
	{
		// this manipulate how far and near the object is from camera
		_mDistance += deltaDistance;
		if (_mDistance > 50.0f) _mDistance =50.0f;
		if (_mDistance < 0.5f) _mDistance = 0.5f;
	}

	void Camera::SetDistance(float distance)
	{
		_mDistance = distance;
		if (_mDistance > 50.0f) _mDistance = 50.0f;
		if (_mDistance < 0.5f) _mDistance = 0.5f;
	}

	void Camera::SetAzimuth(float azimuth)
	{
		_mAzimuth = azimuth;
	}

	void Camera::SetElevation(float elevation)
	{
		_mElevation = elevation;
		if (_mElevation > 89.0f) _mElevation = 89.0f;
		if (_mElevation < -89.0f) _mElevation = -89.0f;
	}

	void Camera::SetDefault()
	{
		_mDistance = 5.0f;
		_mAzimuth = 0.0f;
		_mElevation = 20.0f;
		_mCenter = glm::vec3(0.0f);
	}

}