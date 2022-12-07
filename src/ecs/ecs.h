#pragma once

#include <flecs.h>


namespace Ecs 
{
	using World  = flecs::world;
	using Entity = flecs::entity;
	using System = flecs::system;
	using Iter   = flecs::iter;

	template<typename ...Args>
	using Query  = flecs::query<Args...>;

	struct Timeout 
	{
		void   (*timeout_callback)(Iter& iterator);
		float timeout;
	};

	/// <summary>
	/// Enable the time out system
	/// </summary>
	void	EnableTimeout();

	/// <summary>
	/// Get reference to the world
	/// </summary>
	/// <returns></returns>
	World&	GetWorld();


	void	Progress();
}