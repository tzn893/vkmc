#include "ecs/ecs.h"

namespace Ecs {

	World g_world;
	bool  g_timeout_enabled = false;

	void EnableTimeout()
	{
		if (g_timeout_enabled) return;

		g_timeout_enabled = true;
		g_world.system<Timeout>()
			.each(
			[](flecs::iter& it, size_t index, Timeout& t) {
				t.timeout -= it.delta_time();
				if (t.timeout <= 0.f)
				{
					t.timeout_callback(it);
					Entity time_out_component = it.entity(index);
					time_out_component.destruct();
				}
			}
		);
	}

	World& GetWorld()
	{
		return g_world;
	}

	void Progress()
	{
		g_world.progress();
	}

}