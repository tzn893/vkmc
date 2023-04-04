#include "block.h"
#include "ecs/ecs.h"
#include <tuple>


bool OutofBoundary(Vector3i pos)
{
	if (pos.x >= BLOCK_LEN || pos.x < 0 || pos.y >= BLOCK_LEN || pos.y < 0 || pos.z >= BLOCK_LEN || pos.z < 0)
		return true;
	return false;
}


TerrianChunk::TerrianChunk(Vector3i coord)
{
	m_Idx = coord;
	m_WorldPosition = Vector3f(coord.x * (i32)BLOCK_LEN, coord.y * (i32)BLOCK_LEN, coord.z * (i32)BLOCK_LEN);
	m_Blocks.resize(BLOCK_LEN * BLOCK_LEN * BLOCK_LEN, Block{ 0,BLOCK_TYPE_NONE });
}

void TerrianChunk::Remove(Vector3i coordinate)
{
	Block& current_block = m_Blocks[PositionToIdx(coordinate)];
	if (current_block.block_type == BLOCK_TYPE_NONE)
		return;

	current_block.block_type = BLOCK_TYPE_NONE;
	
	for (u32 fidx = 0; fidx < 6; fidx++)
	{
		FaceCode fcode = (FaceCode)fidx;
		Vector3i fcoord = coordinate + Math::get_face_dir(fcode);
		
		if (OutofBoundary(fcoord)) continue;

		Block& neighbor_block = m_Blocks[PositionToIdx(fcoord)];

		neighbor_block.neighbor_existance ^= 1 << OppsiteFaceCode(fcode);
	}
}

void TerrianChunk::Create(Vector3i coordinate, BlockType type)
{
	Block& current_block = m_Blocks[PositionToIdx(coordinate)];
	vkmc_assert(type != BLOCK_TYPE_NONE);

	current_block.block_type = type;

	for (u32 fidx = 0; fidx < 6; fidx++)
	{
		FaceCode fcode = (FaceCode)fidx;
		Vector3i fcoord = coordinate + Math::get_face_dir(fcode);

		if (OutofBoundary(fcoord)) continue;

		Block& neighbor_block = m_Blocks[PositionToIdx(fcoord)];

		neighbor_block.neighbor_existance |= 1 << OppsiteFaceCode(fcode);
	}
}

const Block& TerrianChunk::Access(Vector3i pos)
{
	return m_Blocks[PositionToIdx(pos)];
}


Vector3f TerrianChunk::GetWorldPosition()
{
	return m_WorldPosition;
}

Vector3i TerrianChunk::GetTrunkIndex()
{
	return m_Idx;
}

uint TerrianChunk::PositionToIdx(Vector3i pos)
{
	return pos.x + pos.y * BLOCK_LEN + pos.z * BLOCK_LEN * BLOCK_LEN;
}

Vector3i TerrianChunk::IdxToPosition(uint idx)
{
	vkmc_assert(idx < BLOCK_CHUNK_SIZE)
	
	Vector3i pos;
	pos.x = idx % BLOCK_LEN;
	uint tmp = idx / BLOCK_LEN;
	pos.y = tmp % BLOCK_LEN;
	tmp = tmp / BLOCK_LEN;
	pos.z = BLOCK_LEN;

	return pos;
}
