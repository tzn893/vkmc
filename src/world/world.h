#pragma once
#include "parallel/task.h"

class World : public Task 
{
public:

	virtual bool		Initialize(TaskManager* manager) ;

	virtual TaskTick	Tick(TaskManager* manager, float delta_time);

	virtual void		Finalize(TaskManager* manager);
private:
};