// ComponentArray.h
#pragma once

#include <vector>
#include <cstdint>
#include <cassert> // 우리는 '절대' 잘못된 접근을 하면 안 됩니다.
#include <limits>   // 유효하지 않은 인덱스를 표기하기 위해

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX // Windows.h가 min/max 매크로를 정의하지 못하게 방지
#include <Windows.h>
#undef  min
#undef  max

// Entity는 단순한 32비트 ID입니다.
using Entity = uint32_t;

// 존재하지 않는 Entity 또는 유효하지 않은 인덱스를 나타내는 상수
const Entity INVALID_ENTITY = std::numeric_limits<Entity>::max();
const uint32_t INVALID_DENSE_INDEX = std::numeric_limits<uint32_t>::max();

/*
 * [IComponentArray]
 * 이것은 '타입 삭제(Type Erasure)'를 위한 인터페이스입니다.
 * 나중에 'ComponentManager'가 'Transform'이든 'Mesh'든 상관없이
 * 모든 컴포넌트 배열을 'IComponentArray*' 포인터로 관리할 수 있게 해줍니다.
 * * 지금 당장 가장 중요한 기능은 "Entity가 파괴되었음"을 모든 배열에게
 * 알려주는 것입니다.
 */
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;

    // 특정 Entity가 씬에서 제거될 때 호출됩니다.
    // 이 배열은 자신이 가진 해당 Entity의 컴포넌트를 즉시 제거해야 합니다.
    virtual void EntityDestroyed(Entity entity) = 0;
};


/*
 * [ComponentArray<T>]
 * 이것이 우리 ECS의 '심장'입니다. '희소 집합(Sparse Set)'의 구현체입니다.
 * 하드웨어가 어떻게 동작하는지 정확히 이해하고 설계해야 합니다.
 */
template<typename T>
class ComponentArray : public IComponentArray
{
public:
    // --- 시스템(System)을 위한 함수 ---

    /*
     * (핵심 성능) 이 함수가 ECS의 존재 이유입니다.
     * 실제 컴포넌트 데이터가 '연속적으로' 저장된 벡터의 참조를 반환합니다.
     * TransformSystem은 이 벡터를 '처음부터 끝까지' 순회(iterate)할 것입니다.
     * * [하드웨어 분석]
     * 'for(auto& comp : m_componentData)' 루프가 실행되면,
     * CPU는 comp[0]을 읽을 때 캐시 라인(Cache Line, 보통 64 bytes)을 채우기 위해
     * comp[1], comp[2], comp[3]... (데이터 크기에 따라)를 '미리(Prefetch)' L1 캐시로
     * 퍼 올립니다.
     * * 그 결과, CPU는 다음 루프에서 메인 메모리(DRAM)를 기다릴 필요 없이
     * L1 캐시에서 즉시 데이터를 읽어 연산(ALU)을 수행합니다.
     * 이것이 '데이터 지역성(Data Locality)'의 힘입니다.
     */
    std::vector<T>& GetDenseData()
    {
        return m_componentData;
    }

    /*
     * (핵심 성능)
     * 위 GetDenseData()로 데이터를 순회할 때, 'i'번째 컴포넌트가
     * '어떤 Entity'의 것인지 알아야 할 때가 있습니다.
     * 이 함수는 m_componentData[i]의 주인이 m_denseToEntityMap[i]임을
     * 알려줍니다. 이 두 벡터는 항상 1:1로 동기화됩니다.
     */
    std::vector<Entity>& GetDenseToEntityMap()
    {
        return m_denseToEntityMap;
    }


    // --- Entity 관리를 위한 함수 ---

    /*
     * (O(1) 추가)
     * 특정 Entity에 컴포넌트 데이터를 추가합니다.
     */
    T& AddComponent(Entity entity, T component)
    {
        // 1. m_sparseArray는 [Entity ID]를 인덱스로 사용합니다.
        //    만약 'entity' ID가 5000인데 배열 크기가 1000이라면, 5000까지 늘려야 합니다.
        if (entity >= m_sparseArray.size())
        {
            // INVALID_DENSE_INDEX로 채워서 '아직 데이터 없음'을 표시합니다.
            m_sparseArray.resize(entity + 1, INVALID_DENSE_INDEX);
        }

        // 2. 이미 컴포넌트를 가지고 있는지 확인합니다. (실무에서는 assert나 예외 처리)
        // (이 간단한 구현에서는 덮어쓴다고 가정합니다.)
        // assert(m_sparseArray[entity] == INVALID_DENSE_INDEX && "Entity already has this component");

        // 3. [핵심] 실제 데이터는 'm_componentData' (Dense Array)의 맨 뒤에 추가됩니다.
        //    이것이 데이터의 '연속성'을 보장합니다.
        uint32_t denseIndex = static_cast<uint32_t>(m_componentData.size());
        m_componentData.push_back(std::move(component)); // C++11 move semantics 사용

        // 4. 이 'denseIndex'가 어떤 Entity의 것인지 역으로 매핑합니다.
        m_denseToEntityMap.push_back(entity);

        // 5. 'entity' ID로 'denseIndex'를 즉시 찾을 수 있도록 희소 배열(Sparse Array)에 기록합니다.
        m_sparseArray[entity] = denseIndex;

        return m_componentData.back();
    }

    /*
     * (O(1) 제거)
     * 이것이 'unordered_map'보다 훨씬 빠르고 효율적인 제거 방식입니다.
     * O(N)의 재앙(vector 중간 원소 제거)을 O(1)의 마법('swap-and-pop')으로 바꿉니다.
     */
    void RemoveComponent(Entity entity)
    {
        // 0. 유효한 Entity인지, 컴포넌트를 가지고 있는지 확인
        if (entity >= m_sparseArray.size() || m_sparseArray[entity] == INVALID_DENSE_INDEX)
        {
            // 제거할 컴포넌트가 없음
            return;
        }

        // 1. 제거할 컴포넌트의 'denseIndex'를 O(1)에 찾습니다.
        uint32_t denseIndexToRemove = m_sparseArray[entity];

        // 2. [SWAP-AND-POP의 핵심 1]
        //    Dense Array의 *맨 마지막* 원소를 가져옵니다.
        T& lastComponent = m_componentData.back();
        Entity lastEntity = m_denseToEntityMap.back();

        // 3. [SWAP-AND-POP의 핵심 2]
        //    *제거할 위치*(denseIndexToRemove)에 *맨 마지막* 원소를 덮어 써서 '구멍'을 메웁니다.
        //    (std::move를 사용하여 비싼 복사(copy) 대신 이동(move)을 합니다.)
        m_componentData[denseIndexToRemove] = std::move(lastComponent);
        m_denseToEntityMap[denseIndexToRemove] = lastEntity;

        // 4. [SWAP-AND-POP의 핵심 3]
        //    이제 'lastEntity'의 컴포넌트는 'denseIndexToRemove' 위치로 이동했습니다.
        //    Sparse Array(조견표)를 O(1)에 업데이트합니다.
        m_sparseArray[lastEntity] = denseIndexToRemove;

        // 5. [SWAP-AND-POP의 핵심 4]
        //    제거 대상이었던 'entity'의 조견표를 무효화합니다.
        m_sparseArray[entity] = INVALID_DENSE_INDEX;

        // 6. [SWAP-AND-POP의 핵심 5]
        //    이제 맨 마지막 원소는 쓸모없어졌으므로 O(1)에 제거합니다.
        m_componentData.pop_back();
        m_denseToEntityMap.pop_back();

        /*
         * [하드웨어 분석]
         * 이 작업은 'std::vector::erase()'와 달리, 데이터를 밀어내는(shift) 'memcpy'가
         * 전혀 발생하지 않습니다. 오직 두 원소의 위치 교환과 pop_back()만 일어납니다.
         * 데이터의 '연속성'은 100% 유지됩니다.
         */
    }

    /*
     * (O(1) 접근)
     * Entity ID로 컴포넌트 데이터를 O(1)에 가져옵니다.
     */
    T& GetComponent(Entity entity)
    {
        // (실무에서는 HasComponent()로 먼저 확인해야 함)
        assert(entity < m_sparseArray.size() && m_sparseArray[entity] != INVALID_DENSE_INDEX && "GetComponent: Entity does not have this component");

        // 1. 희소 배열(조견표)을 O(1)에 조회하여 'denseIndex'를 얻습니다.
        uint32_t denseIndex = m_sparseArray[entity];

        // 2. 밀집 배열(실제 데이터)을 O(1)에 인덱싱하여 데이터를 반환합니다.
        return m_componentData[denseIndex];
    }

    bool HasComponent(Entity entity) const
    {
        return (entity < m_sparseArray.size() && m_sparseArray[entity] != INVALID_DENSE_INDEX);
    }

    // --- IComponentArray 인터페이스 구현 ---

    /*
     * EntityDestroyed는 단순히 RemoveComponent를 호출하면 됩니다.
     * 이 함수가 'virtual'이기 때문에, ComponentManager는
     * 이 클래스가 'Transform'인지 'Mesh'인지 몰라도
     * 'IComponentArray->EntityDestroyed()' 호출만으로
     * 모든 타입의 컴포넌트를 올바르게 제거할 수 있습니다.
     */
    void EntityDestroyed(Entity entity) override
    {
        RemoveComponent(entity);
    }


private:
    /*
     * [m_componentData] (Dense Array - 밀집 배열)
     * 실제 컴포넌트 데이터(T)가 '연속적으로' 저장되는 곳입니다.
     * 시스템(System)은 오직 이 배열만 순회합니다. (캐시 효율성 극대화)
     */
    std::vector<T> m_componentData;

    /*
     * [m_sparseArray] (Sparse Array - 희소 배열)
     * 'Entity ID'를 인덱스로 사용하여 'm_componentData'의 인덱스('denseIndex')를
     * O(1)에 찾기 위한 '조견표'입니다.
     *
     * 예: m_sparseArray[Entity 503] = 10;
     * -> "503번 Entity의 컴포넌트는 m_componentData[10]에 있다."
     *
     * 이 배열은 Entity ID의 최대값만큼 커질 수 있으므로 메모리를 차지하지만,
     * O(1) 접근을 위한 '공간-시간 트레이드오프'입니다.
     */
    std::vector<uint32_t> m_sparseArray;

    /*
     * [m_denseToEntityMap] (역방향 조견표)
     * m_componentData와 항상 1:1로 동기화됩니다.
     * 'm_componentData[i]'의 주인이 'm_denseToEntityMap[i]' Entity임을 알려줍니다.
     *
     * 예: m_denseToEntityMap[10] = 503;
     * -> "m_componentData[10]에 있는 컴포넌트의 주인은 503번 Entity다."
     *
     * 이 배열은 'RemoveComponent'의 'swap-and-pop' 트릭을 구현하는 데 필수적입니다.
     */
    std::vector<Entity> m_denseToEntityMap;
};

