// ComponentManager.h
#pragma once

#include "ComponentArray.h"
#include <memory>
#include <unordered_map>
#include <typeindex>      // 템플릿 타입을 맵의 키로 사용하기 위해

/*
 * [ComponentManager]
 * 모든 'ComponentArray<T>' 인스턴스들을 소유하고 관리합니다.
 * DIP를 활용하여, 자신은 'IComponentArray' 인터페이스만 알고 있습니다.
 */
class ComponentManager
{
public:
    template<typename T, typename... Args>
    T& AddComponent(Entity entity, Args&&... args)
    {
        // 1. 'Transform' 타입의 ComponentArray를 가져옵니다.
        //    (없으면 내부에서 새로 생성합니다.)
        ComponentArray<T>* pArray = GetComponentArray<T>();

        // 2. 실제 컴포넌트 추가는 ComponentArray의 '희소 집합' 로직에 맡깁니다.
        return pArray->AddComponent(entity, T(std::forward<Args>(args)...));
    }

    template<typename T>
    void RemoveComponent(Entity entity)
    {
        GetComponentArray<T>()->RemoveComponent(entity);
    }

    template<typename T>
    T& GetComponent(Entity entity)
    {
        return GetComponentArray<T>()->GetComponent(entity);
    }

    template<typename T>
    bool HasComponent(Entity entity)
    {
        if (m_componentArrays.find(typeid(T)) == m_componentArrays.end())
        {
            return false;
        }
        return GetComponentArray<T>()->HasComponent(entity);
    }

    // [DIP의 핵심]
    // Entity가 파괴될 때 호출됩니다.
    // 이 함수는 'Transform'이 뭔지, 'Mesh'가 뭔지 전혀 모릅니다.
    // 그저 'IComponentArray'의 'EntityDestroyed'만 호출합니다.
    void EntityDestroyed(Entity entity)
    {
        // m_componentArrays 맵의 모든 값(배열 포인터)을 순회합니다.
        for (auto const& [type, pArray] : m_componentArrays)
        {
            pArray->EntityDestroyed(entity);
        }
    }

private:
    template<typename T>
    ComponentArray<T>* GetComponentArray()
    {
        std::type_index typeName = std::type_index(typeid(T));

        // 1. 이 타입의 배열이 맵에 등록되어 있는지 확인합니다.
        if (m_componentArrays.find(typeName) == m_componentArrays.end())
        {
            // 2. [최초 생성]
            //    이 타입의 배열이 처음 요청되었습니다.
            //    - ComponentArray<T>를 '힙'에 생성합니다. (shared_ptr)
            //    - 이것을 'm_componentArrays' (타입 -> 배열) 맵에 등록합니다.
            //    - 이것을 'm_componentArraysList' (목록)에도 등록합니다. (DIP 활용)

            auto pArray = std::make_shared<ComponentArray<T>>();
            m_componentArrays[typeName] = pArray;

            return pArray.get();
        }

        // 3. 이미 생성된 배열을 캐스팅하여 반환합니다.
        // (static_pointer_cast는 shared_ptr을 다운 캐스팅할 때 사용)
        return std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeName]).get();
    }

    /*
     * 우리는 두 개의 저장소가 필요합니다.
     * 1. 타입(Type)으로 특정 배열을 '빠르게 찾기' 위한 맵 (O(1) 해시 접근)
     * 2. '모든' 배열을 순회하기 위한 목록 (EntityDestroyed 용)
     *
     * (참고: 위 코드에서는 m_componentArrays 맵 하나만 순회하여
     * m_componentArraysList를 생략하고 단순화했습니다.
     * 실무에서는 두 개를 분리하는 것이 더 명확할 수 있습니다.)
     */

     // 'std::type_index'를 키로 사용하여, '타입'과 '배열'을 매핑합니다.
     // 값은 'IComponentArray' 인터페이스 포인터로 저장합니다. (DIP)
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> m_componentArrays;
};


