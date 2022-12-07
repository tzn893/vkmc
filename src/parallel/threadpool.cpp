#include "threadpool.h"

ThreadPool::ThreadPool()
{}

ThreadPool& ThreadPool::EnqueueJob(std::function<void()> job)
{
	if (m_Threads.empty()) RecreatePool();
	{
		std::unique_lock<std::mutex> lock(m_JobQueueMutex);
		m_Jobs.push(job);
	}
	m_Notifier.notify_one();

	return *this;
}

void ThreadPool::Join()
{
	m_Stop = true;
	m_Notifier.notify_all();
	for (auto& t : m_Threads) 
	{
		t.join();
	}
	m_Threads.clear();
}

ThreadPool::~ThreadPool()
{
	Join();
}

void ThreadPool::ThreadLoop()
{
	while (true) 
	{
		std::function<void()> job;

		{
			std::unique_lock<std::mutex> lock(m_JobQueueMutex);
			m_Notifier.wait(lock, [&]() {
				return m_Stop || !m_Jobs.empty();
				});
			if (m_Stop) return;
			
			job = m_Jobs.front();
			m_Jobs.pop();
		}

		job();
	}
}

void ThreadPool::RecreatePool()
{
	uint32_t thread_count = std::thread::hardware_concurrency();
	m_Threads.resize(thread_count);
	for (uint32_t i = 0; i < thread_count; i++)
	{
		m_Threads[i] = std::thread(&ThreadPool::ThreadLoop, this);
	}
	m_Stop = false;
}

