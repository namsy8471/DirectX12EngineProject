#pragma once

#include <vector>
#include <memory>
#include <string>

class GameObject; // Forward declaration
class Camera;

class Scene
{
public:
	Scene(std::string name = "New Scene") : m_Name(name) {}
	~Scene() = default;

	void Update(float deltaTime);

	GameObject* CreateGameObject(const std::string& name = "GameObject");
	GameObject* DestroyGameObject(GameObject* object);

	void SetMainCamera(Camera* camera);
	Camera* GetMainCamera() const;

	const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() 
		const { return m_GameObjects; }
private:

	std::string m_Name;
	std::vector<std::unique_ptr<GameObject>> m_GameObjects;
	
	Camera* m_MainCamera = nullptr;
};

