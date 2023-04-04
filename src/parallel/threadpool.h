#pragma once
#include <mutex>
#include <thread>
#include <queue>
#include "common.h"

#include <atomic>

class JobStatus
{
public:
	JobStatus();

	JobStatus(ptr<std::atomic<bool>> finish);

	bool Finish();

	~JobStatus();

private:
	ptr<std::atomic<bool>> m_Finish;
};

class ThreadPool : public Is_Signleton 
{
public:
	ThreadPool();

	JobStatus EnqueueJob(std::function<void()> job);

	void	  Stop();

	void	  JoinAll();

	~ThreadPool();

private:
	void		ThreadLoop();
	void		RecreatePool();


	struct Job
	{
		std::function<void()>	job;
		ptr<std::atomic<bool>>  finish;
	};

	std::queue<Job>			m_Jobs;
	std::mutex									m_JobQueueMutex;
	std::condition_variable						m_Notifier;
	std::vector<std::thread>					m_Threads;

	bool										m_Stop;
};
