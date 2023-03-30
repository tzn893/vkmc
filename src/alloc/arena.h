#pragma once
#include "alloc.h"

class MemoryArenaAllocator : public Allocator
{
public:
	MemoryArenaAllocator();
	
	void Clear();

	//Reset the allocator and all memory allocated will be invalid 
	void Reset();

	virtual void* Allocate(size_t size) override;

	~MemoryArenaAllocator();

private:

	static constexpr size_t BASE_BLOCK_SIZE = 131072;

	struct BlockInfo
	{
		void*	addr;
		size_t	size;
	};

	std::vector<BlockInfo>		m_AllocatedBlocks;
	std::vector<BlockInfo>		m_AvailableBlocks;

	void*					m_CurrentBlock;
	size_t					m_CurrentBlockSize;
	size_t					m_CurrentBlockOffset = 0;
};