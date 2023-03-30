#include "timer.h"
#include <chrono>
using namespace std::chrono;

Timer::Timer()
{
	m_StartTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	m_LastTime =  0;
	m_DeltaTime = 0;
}

float Timer::DeltaTime()
{
	return (float)m_DeltaTime / 1000.f;
}

float Timer::TotalTime()
{
	return (float)m_LastTime / 1000.f;
}

void Timer::Tick()
{
	long long current_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - m_StartTime;
	m_DeltaTime = current_time -  m_LastTime;
	m_LastTime = current_time;
}

