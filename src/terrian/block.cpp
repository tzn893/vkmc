#include "block.h"
#include "ecs/ecs.h"
#include <tuple>


BlockCallback* BlockCallbackTable[BlockCallbackRegister::BLOCK_CALLBACK_TYPE_COUNT][BLOCK_TYPE_COUNT] = { NULL };

opt<FaceCode> FaceUpdateFromBlockMessage(BLOCK_MESSAGE msg)
{
	if (msg < BLOCK_MSG_POS_X_NEIGHBOR_UPDATE) return std::nullopt;

	return (FaceCode)(msg - BLOCK_MSG_POS_X_NEIGHBOR_UPDATE);
}

BLOCK_MESSAGE FaceUpdateToBlockMessage(FaceCode fcode)
{
	return (BLOCK_MESSAGE)(fcode + BLOCK_MSG_POS_X_NEIGHBOR_UPDATE);
}


BlockCallbackRegister::BlockCallbackRegister(Type type, BlockType block_type, BlockCallback* callback)
{
	BlockCallbackTable[type][block_type] = callback;
}

//modified based on PBRT
u32 Left3Shift(u32 x) 
{
	x = (x | (x << 8)) & 0b00000111000000001111;
	//x = ---- -654 ---- ---- 3210
	x = (x | (x << 4)) & 0b01000011000011000011;
	//x = -6-- --54 ---- 32-- --10
	x = (x | (x << 2)) & 0b01001001001001001001;
	//x = -6-- 5--4 --3- -2-- 1--0
	return x;
}

//for inverse morton code
u32 Right3Shift(u32 x) 
{
	x &= 0b01001001001001001001;
	//x = -6-- 5--4 --3- -2-- 1--0
	x = (x | (x >> 2)) & 0b01000011000011000011;
	//x = -6-- --54 ---- 32-- --10
	x = (x | (x >> 4)) & 0b00000111000000001111;
	//x = ---- -654 ---- ---- 3210
	x = (x | (x >> 8)) & 0b1111111;
	return x;
}


u32 MortonCode(u32 x, u32 y, u32 z)
{
	vkmc_assert(x < BLOCK_LEN&& y < BLOCK_LEN&& z < BLOCK_LEN);
	return (Left3Shift(z) << 2) | (Left3Shift(y) << 1) | Left3Shift(x);
}

Vector3i InverseMortonCode(u32 code) 
{
	return Vector3i(Right3Shift(code >> 2), Right3Shift(code >> 1), Right3Shift(code));
}

BlockCoordinate::BlockCoordinate(u32 x, u32 y, u32 z)
{
	m_MortonCode = MortonCode(x, y, z);
}

BlockCoordinate::BlockCoordinate(Vector3i v)
{
	m_MortonCode = MortonCode(v.x, v.y, v.z);
}

opt<BlockCoordinate> BlockCoordinate::Offset(i32 x, i32 y, i32 z)
{
	Vector3i offset = InverseMortonCode(m_MortonCode);
	
	offset = offset + Vector3i(x, y, z);
	if (offset.x >= BLOCK_LEN || offset.x < 0 || offset.y >= BLOCK_LEN
		|| offset.y < 0 || offset.z >= BLOCK_LEN || offset.z < 0)
	{
		return std::nullopt;
	}

	return BlockCoordinate{ MortonCode(offset.x,offset.y,offset.z) };
}


opt<RayBlockIntersection> TerrianChunk::CastRay(Ray r)
{
	Vector3f offset = r.o - m_WorldPosition;
	Vector3i curr_offset((u32)offset.x, (u32)offset.y, (u32)offset.z);
	
	float t = 0; FaceCode face_idx = FACE_CODE_POSITIVE_Y;
	while (true)
	{
		//out of boundary
		if (curr_offset.x >= BLOCK_LEN || curr_offset.x < 0 || curr_offset.y >= BLOCK_LEN
			|| curr_offset.y < 0 || curr_offset.z >= BLOCK_LEN || curr_offset.z < 0) 
		{
			return std::nullopt;
		}

		BlockCoordinate curr_block_pos(curr_offset);
		if (m_Blocks[curr_block_pos.m_MortonCode].block_type != BLOCK_TYPE_NONE)
		{
			RayBlockIntersection intersection;
			intersection.block_info = m_Blocks[curr_block_pos.m_MortonCode];
			intersection.coordinate = curr_block_pos;
			intersection.face_idx = face_idx;
			intersection.position = t * r.d + r.o;
			
			return intersection;
		}

		//find the next block
		Bound3f curr_block_bound(Vector3f(curr_offset),Vector3f(curr_offset + 1));

		if (!Math::ray_intersect_bound(curr_block_bound, r, &t, &face_idx, NULL)) 
		{
			return std::nullopt;
		}
		curr_offset = curr_offset - Math::get_face_direction(face_idx);
		
		//push a little forward to prevent self intersection
		r.o = r.o + r.d * (t + 1e-3);
	} 

	//this code should not be reached
	return std::nullopt;
}



void TerrianChunk::Remove(BlockCoordinate coordinate)
{
	Block& current_block = m_Blocks[coordinate.m_MortonCode];
	if (current_block.block_type == BLOCK_TYPE_NONE)
		return;

	//if the block defines a callback on remove operation
	//call this callback
	BlockCallback* remove_callback = BlockCallbackTable[BlockCallbackRegister::Remove][current_block.block_type];
	if (remove_callback != NULL) 
	{
		remove_callback(InverseMortonCode(coordinate.m_MortonCode), this, current_block, BLOCK_MSG_NONE);
	}
	
	current_block.block_type = BLOCK_TYPE_NONE;
	
	for (u32 fidx = 0; fidx < 6; fidx++)
	{
		FaceCode fcode = (FaceCode)fidx;
		BlockCoordinate fcoord;
		match(coordinate.Offset(Math::get_face_direction(fcode)),c,
			fcoord = c.value(),
			continue
		);

		Block& neighbor_block = m_Blocks[coordinate.m_MortonCode];

		//call update of the neighbor blocks due to change of neighbor
		neighbor_block.neighbor_existance ^= 1 << OppsiteFaceCode(fcode);
		BlockCallback* update_callback = BlockCallbackTable[BlockCallbackRegister::Update][neighbor_block.block_type];
		if (update_callback != NULL)
		{
			update_callback(InverseMortonCode(fcoord.m_MortonCode), this, neighbor_block, FaceUpdateToBlockMessage(OppsiteFaceCode(fcode)) );
		}
	}
}

void TerrianChunk::Create(BlockCoordinate coordinate, BlockType type)
{
	Block& current_block = m_Blocks[coordinate.m_MortonCode];
	vkmc_assert(current_block.block_type != BLOCK_TYPE_NONE);
	
	current_block.block_type = type;

	for (u32 fidx = 0; fidx < 6; fidx++) 
	{
		FaceCode fcode = (FaceCode)fidx;
		BlockCoordinate neighbor_coord;
		match(coordinate.Offset(Math::get_face_direction(fcode)), c,
			neighbor_coord = c.value(),
			continue
		);

		Block& neighbor_block = m_Blocks[neighbor_coord.m_MortonCode];

		neighbor_block.neighbor_existance |= 1 << OppsiteFaceCode(fcode);
		BlockCallback* update_callback = BlockCallbackTable[BlockCallbackRegister::Update][neighbor_block.block_type];
		if (update_callback != NULL) 
		{
			update_callback(InverseMortonCode(neighbor_coord.m_MortonCode), this, neighbor_block, FaceUpdateToBlockMessage(OppsiteFaceCode(fcode)) );
		}
	}

	BlockCallback* create_callback = BlockCallbackTable[BlockCallbackRegister::Create][current_block.block_type];
	if (create_callback != NULL)
	{
		create_callback(InverseMortonCode(coordinate.m_MortonCode), this, current_block, BLOCK_MSG_NONE);
	}
}

const Block& TerrianChunk::Access(BlockCoordinate coord)
{
	return m_Blocks[coord.m_MortonCode];
}

void TerrianChunk::Update(BlockCoordinate coord, BlockType type)
{
	Block& current_block = m_Blocks[coord.m_MortonCode];
	BlockCallback* update_callback = BlockCallbackTable[BlockCallbackRegister::Update][current_block.block_type];
	if (update_callback != NULL)
	{
		update_callback(InverseMortonCode(coord.m_MortonCode), this, current_block, BLOCK_MSG_TYPE_UPDATE);
	}
	m_Blocks[coord.m_MortonCode].block_type = type;
}

BlockInitializeCallback* BlockInitializeCallbackTable[BLOCK_TYPE_COUNT] = {NULL};


BlockInitializeCallbackRegister::BlockInitializeCallbackRegister(BlockType type, BlockInitializeCallback* callback)
{
	BlockInitializeCallbackTable[type] = callback;
}


void BlockSystemInitialize()
{
	for (u32 i = 0;i < BLOCK_TYPE_COUNT;i++) 
	{
		if(BlockInitializeCallbackTable[i])
			BlockInitializeCallbackTable[i]();
	}
}