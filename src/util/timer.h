#pragma once
#include "common.h"
#include "singleton.h"

class Timer : public Is_Signleton
{
public:
	Timer();

	float DeltaTime();

	float TotalTime();

	void Tick();

private:
	long long m_StartTime;
	long long m_LastTime;
	long long m_DeltaTime;
};