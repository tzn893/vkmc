#include "world/world.h"
#include "camera.h"
#include "renderer/renderer.h"

bool World::Initialize(TaskManager* manager)
{
	u32 win_width, win_height;
	if(auto r = manager->FindTask<Renderer>();r.has_value())
	{
		auto [w, h] = r.value()->GetCurrentBackbufferExtent();
		win_width = w, win_height = h;
	}


	Singleton<MainCamera>::Get().Initialize(win_width, win_height, Transform());

	return true;
}

TaskTick World::Tick(TaskManager* manager, float delta_time)
{
	return TASK_TICK_CONTINUE;
}

void World::Finalize(TaskManager* manager)
{

}

