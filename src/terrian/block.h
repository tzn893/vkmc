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
	u8  neighbor_existance;
};

inline constexpr u32 BLOCK_LEN_BIT_COUNT = 7;
inline constexpr u32 BLOCK_LEN = 1 << BLOCK_LEN_BIT_COUNT;


//the morton code of the block is the block's coordinate
struct BlockCoordinate {
	BlockCoordinate() : m_MortonCode(0) {}

	BlockCoordinate(u32 code) : m_MortonCode(code) {}

	BlockCoordinate(u32 x, u32 y, u32 z);

	BlockCoordinate(Vector3i v);

	opt<BlockCoordinate> Offset(i32 x,i32 y,i32 z);

	opt<BlockCoordinate> Offset(Vector3i v)
	{
		return Offset(v.x, v.y, v.z);
	}

	u32 m_MortonCode;
};



struct RayBlockIntersection 
{
	Vector3f		position;
	BlockCoordinate coordinate;
	Block			block_info;
	u8				face_idx;
};

class TerrianChunk
{
public:
	opt<RayBlockIntersection>	CastRay(Ray r);

	void						Remove(BlockCoordinate coordinate);

	void						Create(BlockCoordinate coordinate, BlockType type);

	const Block&				Access(BlockCoordinate coord);

	void						Update(BlockCoordinate coord, BlockType type);
private:

	std::vector<Block>			m_Blocks;
	//left bottom coordinate of the chunk is world position
	Vector3f					m_WorldPosition;
};


enum BLOCK_MESSAGE 
{
	BLOCK_MSG_NONE,
	BLOCK_MSG_POS_X_NEIGHBOR_UPDATE,
	BLOCK_MSG_POS_Y_NEIGHBOR_UPDATE,
	BLOCK_MSG_POS_Z_NEIGHBOR_UPDATE,
	BLOCK_MSG_NEG_X_NEIGHBOR_UPDATE,
	BLOCK_MSG_NEG_Y_NEIGHBOR_UPDATE,
	BLOCK_MSG_NEG_Z_NEIGHBOR_UPDATE,
	BLOCK_MSG_TYPE_UPDATE
};

opt<FaceCode>		FaceUpdateFromBlockMessage(BLOCK_MESSAGE msg);
BLOCK_MESSAGE		FaceUpdateToBlockMessage(FaceCode fcode);

using BlockCallback = void(Vector3i, TerrianChunk*, Block& block,BLOCK_MESSAGE msg);
using BlockInitializeCallback = void();

struct BlockCallbackRegister 
{
	enum Type 
	{
		Create,Remove,Update,BLOCK_CALLBACK_TYPE_COUNT
	};
	BlockCallbackRegister(Type,BlockType,BlockCallback* callback);
};

struct BlockInitializeCallbackRegister
{
	BlockInitializeCallbackRegister(BlockType,BlockInitializeCallback* callback);
};


void BlockSystemInitialize();


#define BLOCK_CALLBACK_FN(State, BType) \
	void BType##State##Fn (Vector3i,TerrianChunk*, Block& block,BLOCK_MESSAGE msg);\
	static BlockCallbackRegister _##BType##State##Register = BlockCallbackRegister(BlockCallbackRegister::##State,BLOCK_TYPE_##BType,BType##State##Fn);\
	void BType##State##Fn (Vector3i pos,TerrianChunk* chunk, Block& block,BLOCK_MESSAGE msg)

#define BLOCK_INITIALIZE_FN(BType) \
	void BType##Init##Fn ();\
	static BlockInitializeCallbackRegister _##Btype##InitRegister = BlockInitializeCallbackRegister(BLOCK_TYPE_##BType,BType##Init##Fn);\
	void BType##Init##Fn()