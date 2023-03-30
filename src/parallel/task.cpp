#include "task.h"

#include <thread>


opt<ptr<Task>> TaskManager::FindTask(size_t id)
{
	auto res = std::find(m_TaskIDs.begin(), m_TaskIDs.end(), id);
	if (res != m_TaskIDs.end())
	{
		return *(m_Tasks.begin() + (res - m_TaskIDs.begin()));
	}
	return std::nullopt;
}

bool TaskManager::Tick(float delta_time)
{
	std::vector<bool> task_continue(m_Tasks.size(),false);
	for (u32 i = 0;i < m_Tasks.size();i++) 
	{
		auto& task = m_Tasks[i];
		m_ThreadPool.EnqueueJob(
			[&]() {
				task_continue[i] = task->Tick(this, delta_time);
			}
		);
	}
	m_ThreadPool.Join();

	u32 p = 0;
	for(u32 i = 0;i < task_continue.size();i++)
	{
		if (task_continue[i]) p++;
		else 
		{
			m_Tasks[i]->Finalize(this);
			m_Tasks.erase(m_Tasks.begin() + p);
		}
	}

	return !m_Tasks.empty();
}

