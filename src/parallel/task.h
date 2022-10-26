#pragma once
#include <typeinfo>
#include "common.h"
#include "parallel/threadpool.h"

enum TaskTick 
{
	TASK_TICK_FINISH,
	TASK_TICK_CONTINUE,
	TASK_TICK_ERROR
};


class TaskManager;

class Task 
{
public:
	/// <summary>
	/// Every task should have a task id
	/// the task id of each task should be unique
	/// </summary>
	/// <returns> task's task id</returns>
	virtual size_t		TaskID() const = 0;

	virtual bool		Initialize(TaskManager* manager) = 0;

	virtual TaskTick	Tick(TaskManager* manager,float delta_time) = 0;

	virtual void		Finalize(TaskManager* manager) = 0;
};

class TaskManager 
{
public:

	template<typename TaskType,typename ...Args>
	opt<size_t>		CreateTask(Args... args) 
	{
		static_assert(std::is_base_of_v<Task, TaskType>, "tasks should be derived from Task type");
		ptr<TaskType> new_task = make_shared<TaskType>(args...);
		//new task's id should be unique
		if (FindTask(new_task->TaskID()).has_value())
		{
			return std::nullopt;
		}

		if (!new_task->Initialize(this)) return std::nullopt;
		m_Tasks.push_back(new_task);

		return new_task->TaskID();
	}

	opt<ptr<Task>>	FindTask(size_t id);

	bool			Tick(float delta_time);

private:
	std::vector<ptr<Task>> m_Tasks;
	ThreadPool			   m_ThreadPool;
};
