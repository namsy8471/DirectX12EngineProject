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

	Transform* GetTransform() { return &m_Transform; }
	
	template<typename T, typename ...Args>
	T* AddComponent(Args&& ...args)
	{
		std::unique_ptr<T> newComponent = std::make_unique<T>(this, std::forward<Args>(args)...);
		T* newComponentPtr = newComponent.get();
		m_Components.emplace_back(std::move(newComponent));
		return newComponentPtr;
	}

	template<typename T>
	T* GetComponent()
	{
		for (auto& component : m_Components)
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
	std::string m_Name;
	Transform m_Transform;

	std::vector<std::unique_ptr<IComponent>> m_Components;
};