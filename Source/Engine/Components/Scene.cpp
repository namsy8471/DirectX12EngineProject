#include "Scene.h"

#include "GameObject.h"
#include "IComponent.h"
#include "Camera.h"

void Scene::Update(float deltaTime)
{
	for (const auto& gameObject : m_GameObjects)
	{
		gameObject->Update(deltaTime);
	}
}

GameObject* Scene::CreateGameObject(const std::string& name)
{
	auto newObject = std::make_unique<GameObject>(name);
	GameObject* ptr = newObject.get();
	m_GameObjects.emplace_back(std::move(newObject));
	
	return ptr;
}

GameObject* Scene::DestroyGameObject(GameObject* object)
{
	Camera* camera = object->GetComponent<Camera>();
	if (camera && camera == m_MainCamera)
	{
		m_MainCamera = nullptr;
	}

	// Erase Remove Idiom
	auto it = std::remove_if(m_GameObjects.begin(), m_GameObjects.end(),
		[object](const std::unique_ptr<GameObject>& objPtr)
		{
			return objPtr.get() == object;
		}
	);

	if (it != m_GameObjects.end())
	{
		m_GameObjects.erase(it, m_GameObjects.end());
		return object;
	}
}

void Scene::SetMainCamera(Camera* camera)
{
	m_MainCamera = camera;
}

Camera* Scene::GetMainCamera() const
{
	return m_MainCamera;
}
