#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Transform.h"

class IComponent;

class GameObject
{
public:
	GameObject(std::string name = "GameObject");
	~GameObject();

	Transform* GetTransform() { return &m_transform; }
	
	template<typename T, typename ...Args>
	T* AddComponent(Args&& ...args)
	{
		std::unique_ptr<T> newComponent = std::make_unique<T>(this, std::forward<Args>(args)...);
		T* newComponentPtr = newComponent.get();
		m_components.emplace_back(std::move(newComponent));
		return newComponentPtr;
	}

	template<typename T>
	T* GetComponent()
	{
		if constexpr (std::is_same_v <T, Transform>)
		{
			return static_cast<T*>(&m_transform);
		}

		for (auto& component : m_components)
		{
			if (T* casted = dynamic_cast<T*>(component.get()))
			{
				return casted;
			}
		}
		return nullptr;
	}

	void Update(float deltaTime);

private:
	std::string m_name;
	Transform m_transform;

	std::vector<std::unique_ptr<IComponent>> m_components;
};