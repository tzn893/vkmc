#include "threadpool.h"

#include <chrono>

ThreadPool::ThreadPool()
{}

JobStatus ThreadPool::EnqueueJob(std::function<void()> job)
{
	auto finish = ptr<std::atomic<bool>>(new std::atomic<bool>(false));
	if (m_Threads.empty()) RecreatePool();
	{
		std::unique_lock<std::mutex> lock(m_JobQueueMutex);
		m_Jobs.push(Job{ job,finish });
	}
	m_Notifier.notify_one();
	
	return JobStatus(finish);
}

void ThreadPool::Stop()
{
	m_Stop = true;
	m_Notifier.notify_all();
	for (auto& t : m_Threads) 
	{
		t.join();
	}
	m_Threads.clear();
}

void ThreadPool::JoinAll()
{
	using namespace std::chrono_literals;

	while (true)
	{
		std::this_thread::sleep_for(1ms);
		std::unique_lock<std::mutex> lck(m_JobQueueMutex);
		
		if (m_Jobs.empty())
		{
			break;
		}
	}
}

ThreadPool::~ThreadPool()
{
	Stop();
}

void ThreadPool::ThreadLoop()
{
	while (true) 
	{
		Job job;

		{
			std::unique_lock<std::mutex> lock(m_JobQueueMutex);
			m_Notifier.wait(lock, [&]() 
				{
				return m_Stop || !m_Jobs.empty();
				}
			);
			if (m_Stop) return;
			

			job = m_Jobs.front();
			m_Jobs.pop();
		}

		job.job();
		job.finish->store(true);

		job.finish = nullptr;
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

JobStatus::JobStatus(ptr<std::atomic<bool>> finish)
{
	m_Finish = finish;
}

JobStatus::JobStatus()
{
	m_Finish = nullptr;
}

bool JobStatus::Finish()
{
	if (m_Finish == nullptr)
	{
		return true;
	}

	return m_Finish->load();
}

JobStatus::~JobStatus()
{
	m_Finish = nullptr;
}
