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
	virtual bool		Initialize(TaskManager* manager) = 0;

	virtual TaskTick	Tick(TaskManager* manager,float delta_time) = 0;

	virtual void		Finalize(TaskManager* manager) = 0;
};

template<typename T>
struct TaskID 
{
	static size_t Get()
	{
		return typeid(T).hash_code();
	}
};

class TaskManager : public Is_Signleton
{
public:

	template<typename TaskType,typename ...Args>
	opt<size_t>		CreateTask(Args... args) 
	{
		static_assert(std::is_base_of_v<Task, TaskType>, "tasks should be derived from Task type");
		ptr<TaskType> new_task = make_shared<TaskType>(args...);
		//new task's id should be unique
		if (FindTask(TaskID<TaskType>::Get()).has_value())
		{
			return std::nullopt;
		}

		if (!new_task->Initialize(this)) return std::nullopt;
		m_Tasks.push_back(new_task);
		m_TaskIDs.push_back(TaskID<TaskType>::Get());

		return TaskID<TaskType>::Get();
	}

	opt<ptr<Task>>	FindTask(size_t id);

	template<typename T>
	opt<ptr<T>>		FindTask()
	{
		if (auto t = FindTask(TaskID<T>::Get()); t.has_value())
		{
			return std::dynamic_pointer_cast<T>(t.value());
		}
		return std::nullopt;
	}

	bool			Tick(float delta_time);

private:
	std::vector<size_t>		m_TaskIDs;
	std::vector<ptr<Task>>	m_Tasks;
	ThreadPool				m_ThreadPool;
};
