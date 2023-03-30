#include "world/world.h"
#include "camera.h"

bool World::Initialize(TaskManager* manager)
{
	u32 win_width, win_height;
	Singleton<MainCamera>::Get().Initialize(win_width, win_height);

	return true;
}

TaskTick World::Tick(TaskManager* manager, float delta_time)
{
	return TASK_TICK_CONTINUE;
}

void World::Finalize(TaskManager* manager)
{

}

