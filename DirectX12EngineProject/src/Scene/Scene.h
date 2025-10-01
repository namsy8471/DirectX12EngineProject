#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

#include <DirectXMath.h>

using Entity = uint32_t;

struct Transform {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    DirectX::XMFLOAT3 scale;
};

struct MeshRenderer {
    uint32_t meshID;
    uint32_t materialID;
};


class Scene {
public:
    Entity CreateEntity() {
        Entity e = m_nextEntity++;
        m_entities.push_back(e);
        return e;
    }

    template<typename T, typename... Args>
    T& AddComponent(Entity e, Args&&... args) {
        auto& compArray = GetComponentArray<T>();
        compArray[e] = T(std::forward<Args>(args)...);
        return compArray[e];
    }

    template<typename T>
    T* GetComponent(Entity e) {
        auto& compArray = GetComponentArray<T>();
        auto it = compArray.find(e);
        return (it != compArray.end()) ? &it->second : nullptr;
    }

private:
	Entity              m_nextEntity = 0;        // Next available entity ID
	std::vector<Entity> m_entities;              // List of all entities

    template<typename T>
    std::unordered_map<Entity, T>& GetComponentArray() {
        static std::unordered_map<Entity, T> compArray;
        return compArray;
    }
};

