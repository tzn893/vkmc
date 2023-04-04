#pragma once
#include "common.h"
#include "math/math.h"


/// <summary>
///     front face        back face
///     --------          -------
///	   /  2   / |        /  4   / |
///	  /      /  |       /      /  |
///	  ------- 1 |       ------- 3 |
///  |       |  /      |       |  |
///	 |   0   | /       |   5   |  /
///  |       |/        |       | /
///   -------           ------- 
///   numbering of faces
/// </summary>

enum BlockType 
{
	BLOCK_TYPE_NONE,//nothing in the block
	BLOCK_TYPE_MUD,//mud block
	BLOCK_TYPE_GRASS,//mud block with grass on face 2
	BLOCK_TYPE_ROCK,//rock block
	BLOCK_TYPE_TEST,// test block

	BLOCK_TYPE_COUNT//count of the blocks
};


struct Block 
{
	u8  neighbor_existance;
	u16 block_type;
};

inline constexpr i32 BLOCK_LEN_BIT_COUNT = 7;
inline constexpr i32 BLOCK_LEN = 1 << BLOCK_LEN_BIT_COUNT;
inline constexpr i32 BLOCK_CHUNK_SIZE = BLOCK_LEN * BLOCK_LEN * BLOCK_LEN * sizeof(Block);


class TerrianChunk
{
	friend class FTerrian;
public:
	//opt<RayBlockIntersection>	CastRay(Ray r);
	TerrianChunk(Vector3i terrian_chunk_idx);

	void						Remove(Vector3i pos);

	void						Create(Vector3i pos, BlockType type);

	const Block&				Access(Vector3i pos);

	Vector3f					GetWorldPosition();

	Vector3i					GetTrunkIndex();

private:

	uint						PositionToIdx(Vector3i pos);
	Vector3i					IdxToPosition(uint     idx);

	std::vector<Block>			m_Blocks;
	//left bottom coordinate of the chunk is world position
	Vector3f					m_WorldPosition;
	Vector3i					m_Idx;
};
