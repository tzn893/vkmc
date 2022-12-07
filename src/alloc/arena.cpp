#include "arena.h"


MemoryArenaAllocator::MemoryArenaAllocator()
{
	m_CurrentBlockOffset = 0;

	m_CurrentBlock = malloc(BASE_BLOCK_SIZE);
	m_CurrentBlockSize = BASE_BLOCK_SIZE;
}

void MemoryArenaAllocator::Clear()
{
	m_CurrentBlockOffset = 0;
	free(m_CurrentBlock);

	for (auto block : m_AllocatedBlocks)
	{
		free(block.addr);
	}

	for (auto block : m_AvailableBlocks) 
	{
		free(block.addr);
	}

}

void MemoryArenaAllocator::Reset()
{
	for (auto block : m_AllocatedBlocks) 
	{
		m_AvailableBlocks.push_back(block);
	}
	m_AllocatedBlocks.clear();

	m_CurrentBlockOffset = 0;
}

void* MemoryArenaAllocator::Allocate(size_t size)
{
	if (size > BASE_BLOCK_SIZE)
	{
		void* rv = malloc(size);
		m_AllocatedBlocks.push_back({ rv, size });
		return rv;
	}

	if (m_CurrentBlockOffset + size > m_CurrentBlockSize)
	{
		m_CurrentBlockOffset = 0;
		m_AllocatedBlocks.push_back({ m_CurrentBlock, m_CurrentBlockSize });

		if (m_AvailableBlocks.empty())
		{
			m_CurrentBlock = malloc(BASE_BLOCK_SIZE);
			m_CurrentBlockSize = BASE_BLOCK_SIZE;
		}
		else
		{
			m_CurrentBlock = m_AvailableBlocks.rbegin()->addr;
			m_CurrentBlockSize = m_AllocatedBlocks.rbegin()->size;
			m_AllocatedBlocks.pop_back();
		}
	}
	void* rv = ((u8*)m_CurrentBlock) + m_CurrentBlockOffset;
	m_CurrentBlockOffset += size;
	return rv;
}

MemoryArenaAllocator::~MemoryArenaAllocator()
{
	Clear();
}

