#include "block.h"
#include "ecs/ecs.h"

void  MudTimeoutCallback(Ecs::Iter& iterator) 
{
	//TODO
}

//for ecs system
struct MudBlockPosition
{
	BlockCoordinate		morton;
	TerrianChunk* chunk;
};

Ecs::Query<MudBlockPosition> g_mud_position_query;

BLOCK_INITIALIZE_FN(MUD)
{
	Ecs::EnableTimeout();
	//create a query for removing
	g_mud_position_query = Ecs::GetWorld().query<MudBlockPosition>();
}

BLOCK_CALLBACK_FN(Create, MUD)
{
	Ecs::GetWorld().entity()
		.set<MudBlockPosition>(MudBlockPosition{ BlockCoordinate(pos),chunk })
		.set<Ecs::Timeout>(Ecs::Timeout{ MudTimeoutCallback,10.f });
}

BLOCK_CALLBACK_FN(Remove, MUD)
{
	//TODO
}