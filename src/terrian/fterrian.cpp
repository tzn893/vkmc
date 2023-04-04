#include "fterrian.h"

#include <filesystem>


opt<ptr<FTerrian>> FTerrian::Load(const std::string& path)
{
	ptr<FTerrian> terrian = std::make_shared<FTerrian>(path);

	if (!std::filesystem::exists(path))
	{
		std::fstream file(path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
		file.write((char*)&terrian->m_Header, sizeof(terrian->m_Header));
		file.close();
	}
	else
	{
		std::fstream file(path, std::ios_base::in | std::ios_base::binary );
		file.read((char*)&terrian->m_Header, sizeof(FTerrianHeader));
		file.close();

		if (strcmp(terrian->m_Header.magic_number,"vkmcter") != 0)
		{
			vkmc_error("saved data at {} has been corrupted",path);
			return std::nullopt;
		}
	}

	return terrian;
}

opt<ptr<TerrianChunk>> FTerrian::LoadChunk(Vector3i pos)
{
	//find chunk in cached chunks
	for (u32 i = 0;i < m_CachedChunkCount;i++)
	{
		if (m_CachedChunk[i]->GetTrunkIndex() == pos)
		{
			//move finded cached chunk to first position
			ptr<TerrianChunk> chunk = m_CachedChunk[i];
			ShiftCachedChunksBy1(i);
			m_CachedChunk[0] = chunk;

			return chunk;
		}
	}

	//load chunk from file
	ptr<TerrianChunk> chunk;
	match
	(
		LoadChunkFromDisk(pos), c,
		chunk = c.value(),
		return std::nullopt;
	);

	//add chunk to cached chunks
	InsertCachedChunk(chunk);

	return chunk;
}

void FTerrian::StoreChunk(ptr<TerrianChunk> chunk)
{
	InsertCachedChunk(chunk);
}

FTerrian::~FTerrian()
{
	for (u32 i = 0; i < m_CachedChunkCount; i++)
	{		
		ptr<TerrianChunk> chunk = m_CachedChunk[i];
		m_CachedChunk[i] = nullptr;
		
		StoreChunkToDisk(chunk);
	}
}

FTerrian::FTerrian(const std::string& path): m_FilePath(path)
{
	m_CachedChunkCount = 0;
}

opt<u32> FTerrian::FindHeaderIndex(Vector3i pos)
{
	u32 chunk_header_idx = 0;
	for (; chunk_header_idx < m_Header.count; chunk_header_idx++)
	{
		if (m_Header.terrian_idxs[chunk_header_idx] == pos)
		{
			break;
		}
	}

	if (chunk_header_idx >= m_Header.count)
	{
		return std::nullopt;
	}
	else
	{
		return chunk_header_idx;
	}
}

u32 FTerrian::GetOffsetByIndex(u32 header_idx)
{
	return header_idx * BLOCK_CHUNK_SIZE + sizeof(m_Header);
}

void FTerrian::ShiftCachedChunksBy1(i32 end)
{
	vkmc_assert(end < m_CachedChunk.size());
	for (i32 j = end - 1; j >= 0; j--)
	{
		m_CachedChunk[j + 1] = m_CachedChunk[j];
	}
}

void FTerrian::InsertCachedChunk(ptr<TerrianChunk> chunk)
{
	//prevent store repeated chunks
	for (u32 i = 0; i < m_CachedChunkCount; i++)
	{
		if (m_CachedChunk[i]->GetTrunkIndex() == chunk->GetTrunkIndex())
		{
			//move finded cached chunk to first position
			ptr<TerrianChunk> chunk = m_CachedChunk[i];
			ShiftCachedChunksBy1(i);
			m_CachedChunk[0] = chunk;

			return;
		}
	}

	if (m_CachedChunkCount == m_CachedChunk.size())
	{
		//store the last chunk to disk
		ptr<TerrianChunk> last_chunk = *m_CachedChunk.rbegin();

		StoreChunkToDisk(last_chunk);
	}

	ShiftCachedChunksBy1(m_CachedChunk.size() - 1);
	m_CachedChunk[0] = chunk;
	m_CachedChunkCount++;
}

void FTerrian::StoreChunkToDisk(ptr<TerrianChunk> chunk)
{
	std::fstream file(m_FilePath);
	vkmc_assert(file.is_open());

	u32 idx = 0;
	if (auto header_idx = FindHeaderIndex(chunk->GetTrunkIndex()); header_idx.has_value())
	{
		//update existing chunk
		idx = header_idx.value();
	}
	else
	{
		//add new chunk to file
		idx = m_Header.count;
		m_Header.terrian_idxs[idx] = chunk->GetTrunkIndex();
		m_Header.count++;

		//update header data
		file.seekp(0);
		file.write((char*)&m_Header, sizeof(m_Header));
	}

	u32 offset = GetOffsetByIndex(idx);
	file.seekp(offset);

	vkmc_assert(chunk->m_Blocks.size() * sizeof(Block) == BLOCK_CHUNK_SIZE);
	file.write((char*)chunk->m_Blocks.data(), BLOCK_CHUNK_SIZE);

	file.close();
}

opt<ptr<TerrianChunk>> FTerrian::LoadChunkFromDisk(Vector3i pos)
{
	std::fstream file(m_FilePath, std::ios_base::in | std::ios_base::binary);
	//load chunk from file
	u32 chunk_header_idx;
	match
	(
		FindHeaderIndex(pos), idx,
		chunk_header_idx = idx.value(),
		return std::nullopt
	);

	u32 offset = GetOffsetByIndex(chunk_header_idx);
	file.seekg(offset);

	ptr<TerrianChunk> chunk = std::make_shared<TerrianChunk>(pos);
	file.read((char*)chunk->m_Blocks.data(), BLOCK_CHUNK_SIZE);
	file.close();

	return chunk;
}

FTerrianHeader::FTerrianHeader()
{
	memset(&terrian_idxs, 0, sizeof(terrian_idxs));
	count = 0;
}
