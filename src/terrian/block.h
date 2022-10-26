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

	BLOCK_TYPE_COUNT//count of the blocks
};


struct Block 
{
	u16 block_type;
	u8  adjust_existance;
};


class TerrianChunck 
{
public:


private:
	u32 MortonCode(u32 x, u32 y, u32 z);

	std::vector<Block> m_Blocks;
	Vector3f           m_WorldPosition;
};