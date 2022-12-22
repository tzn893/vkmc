#include "terrian/terrian.h"
#include "parallel/task.h"
#include "renderer/renderer.h"

TerrianRenderObject::TerrianRenderObject():
	CustomRenderObject(CUSTOM_ROBJECT_ORDERER_BEFORE_ALL)
{

}

void TerrianRenderObject::OnRender(RenderContext ctx)
{

}

TerrianRenderer::TerrianRenderer(const Terrian* terrian)
{
	m_Terrian = terrian;
}

bool TerrianRenderer::Activated()
{
	return true;
}

bool TerrianRenderer::Initialize(RenderInitContext ctx)
{
	for(u32 i = 0;i < ctx.back_buffer_count;i++)
	{ 
		match(ctx.context->CreateBuffer( VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				BLOCK_CHUNK_SIZE * 4, GVK_HOST_WRITE_NONE), b,
			m_TerrianDrawBuffer[i] = b.value(),
			return false;m_TerrianDrawBuffer
		);

		match(ctx.context->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, BLOCK_CHUNK_SIZE * 4, GVK_HOST_WRITE_RANDOM),b,
			m_TerrianStorageBuffer[i] = b.value(),
			return false
		);

		//one draw indexed indirect command
		match(ctx.context->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			sizeof(VkDrawIndirectCommand), GVK_HOST_WRITE_NONE), b,
			m_TerrianDrawIndirectBuffer[i] = b.value(),
			return false;
		);
	}
	

}

RenderObject* TerrianRenderer::OnRender(MemoryArenaAllocator* allocator)
{
	return allocator->Create<TerrianRenderObject>();
}

void TerrianRenderer::Finalize(RenderFinalizeContext ctx)
{

}

bool Terrian::Initialize()
{
	ptr<Renderer> render;
	match(Singleton<TaskManager>::Get().FindTask<Renderer>(), r,
		render = r.value(),
		return false;
	);

	render->RegisterRenderComponent(m_Renderer);

	GenerateTerrianChunk(0, 0, 0);
}

void Terrian::Update()
{
	
}

void Terrian::GenerateTerrianChunk(u32 x, u32 y, u32 z)
{
	BlockCoordinate coord(0, 0, 0);
	
	ptr<TerrianChunk> chunk(new TerrianChunk());
	chunk->Create(coord, BLOCK_TYPE_ROCK);
}
