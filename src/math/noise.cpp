#include "math/noise.h"

namespace Math 
{
	float hashIQ(uint n)
	{
		// integer hash copied from Hugo Elias
		n = (n << 13U) ^ n;
		n = n * (n * n * 15731U + 789221U) + 1376312589U;
		return float(n & 0x7fffffffU) / float(0x7fffffff);
	}

	float hash2D(Vector2i pos)
	{
		return hashIQ(pos.x + 1549 * pos.y);
	}

	float Perlin2D(Vector2f pos, float seed)
	{
		uint u0 = (uint)pos.x, v0 = (uint)pos.y;
		uint u1 = u0 + 1, v1 = v0 + 1;

		float fu = pos.x - u0, fv = pos.y - v0;

		float u0v0 = hash2D(Vector2i(u0, v0));
		float u1v0 = hash2D(Vector2i(u1, v0));
		float u0v1 = hash2D(Vector2i(u0, v1));
		float u1v1 = hash2D(Vector2i(u1, v1));

		return lerp(
			lerp(u0v0, u1v0, fu),
			lerp(u0v1, u1v1, fu),
			fv
		);
	}
}

