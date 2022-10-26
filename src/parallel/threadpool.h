#pragma once
#include <mutex>
#include <thread>
#include <queue>
#include "common.h"

class ThreadPool 
{
	friend class TaskManager;
public:

	ThreadPool& EnqueueJob(std::function<void()> job);

	void		Join();

	~ThreadPool();
private:
	ThreadPool();

	void		ThreadLoop();
	void		RecreatePool();

	std::queue<std::function<void()>>			m_Jobs;
	std::mutex									m_JobQueueMutex;
	std::condition_variable						m_Notifier;
	std::vector<std::thread>					m_Threads;

	bool										m_Stop;
};
