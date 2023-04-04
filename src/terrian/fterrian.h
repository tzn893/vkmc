#include "block.h"
#include <fstream>

struct FTerrianHeader
{
	const char magic_number[8] = {'v','k','m','c','t','e','r','\0'};

	u32 count;
	std::array<Vector3i, 128> terrian_idxs;

	FTerrianHeader();
};

class FTerrian
{
public:
	FTerrian(const std::string& path);

	static opt<ptr<FTerrian>>	Load(const std::string& path);

	opt<ptr<TerrianChunk>>		LoadChunk(Vector3i pos);

	void						StoreChunk(ptr<TerrianChunk> chunk);

	~FTerrian();
private:

	opt<u32>					FindHeaderIndex(Vector3i pos);

	u32							GetOffsetByIndex(u32 header_idx);

	void						ShiftCachedChunksBy1(i32 end);

	void						InsertCachedChunk(ptr<TerrianChunk> chunk);

	void						StoreChunkToDisk(ptr<TerrianChunk> chunk);

	opt<ptr<TerrianChunk>>		LoadChunkFromDisk(Vector3i pos);

	FTerrianHeader						m_Header;

	u32									m_CachedChunkCount;
	std::array<ptr<TerrianChunk>, 16>	m_CachedChunk;
	const std::string&					m_FilePath;
};