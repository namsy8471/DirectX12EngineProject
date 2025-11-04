#pragma once
#include "IComponent.h"
#include <DirectXMath.h>

using namespace DirectX;

class GameObject; // Forward declaration

class Camera : public IComponent
{
public:
	Camera(GameObject* owner);
	virtual ~Camera() = default;

	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetProjectionMatrix() const;

	void SetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);

private:

	float m_FOV = 60.0f;
	float m_AspectRatio = 16.0f / 9.0f;
	float m_NearPlane = 0.1f;
	float m_FarPlane = 1000.0f;

	XMFLOAT4X4 m_ViewMatrix;
	XMFLOAT4X4 m_ProjectionMatrix;
};

