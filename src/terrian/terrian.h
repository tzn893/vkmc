#pragma once
#include "common.h"
#include "block.h"

#include "renderer/renderer.h"

class Terrian 
{
public:

	


	void GenerateTerrianChunk(u32 x, u32 y, u32 z);



	

private:
	std::vector<ptr<TerrianChunk>> m_LoadedChunks;

};