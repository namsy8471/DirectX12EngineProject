#pragma once
#include <chrono>

class Timer
{
public:
	Timer() { Reset(); }
	void Reset() { m_startTime = std::chrono::high_resolution_clock::now(); }
	void Tick()
	{
		auto now = std::chrono::high_resolution_clock::now();
		m_deltaTime = std::chrono::duration<float>(now - m_lastTime).count();
		m_lastTime = now;
	}
	float GetDeltaTime() const { return m_deltaTime; }
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTime;
	float m_deltaTime = 0.0f;
};