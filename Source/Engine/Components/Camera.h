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

	void Init() override;

	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetProjectionMatrix() const;

	void SetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);

private:

	float m_fov = 60.0f;
	float m_aspectRatio = 16.0f / 9.0f;
	float m_nearPlane = 0.1f;
	float m_farPlane = 1000.0f;

	XMFLOAT4X4 m_viewMatrix;
	XMFLOAT4X4 m_projectionMatrix;
};

