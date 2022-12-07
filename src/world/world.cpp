#include "world/world.h"

size_t World::TaskID() const
{
	return typeid(World).hash_code();
}

bool World::Initialize(TaskManager* manager)
{
	return true;
}

TaskTick World::Tick(TaskManager* manager, float delta_time)
{
	return TASK_TICK_CONTINUE;
}

void World::Finalize(TaskManager* manager)
{

}

