#pragma once
#include <gvk.h>
#include <mutex>

#include "common.h"
#include "block.h"
#include "world/camera.h"
#include "renderer/upload.h"

#include "terrian/shaders/terrian_shader_common.h"
#include "fterrian.h"

#include "parallel/threadpool.h"

class Terrian;

class TerrianRenderer 
{
public:
	TerrianRenderer(const std::string& atlas_path, Terrian* terrian);

	bool Initialize(gvk::ptr<gvk::Context> ctx,gvk::ptr<gvk::RenderPass> render_pass,
		uint subpass_idx, UploadQueue& upload_queue, gvk::ptr<gvk::DescriptorAllocator> alloc,std::string& error);

	void UpdateRenderData(ptr<TerrianChunk> chunk);

	void FlushRenderData();

	void Render(VkCommandBuffer cmd,const RenderCamera& camera);

	void Release(gvk::ptr<gvk::Context>& ctx);

private:
	gvk::ptr<gvk::Pipeline> m_TerrianPipeline;

	gvk::ptr<gvk::Image>	m_TerrianAtlas;
	std::string				m_TerrianAtlasPath;
	VkImageView				m_TerrianAtlasView;
	VkSampler				m_TerrianAtlasSampler;

	ptr<gvk::Buffer>	m_TempTerrianVertexBuffer;

	ptr<gvk::Buffer>	m_TerrianVertexBuffer;
	ptr<gvk::Shader>	m_TerrianVertexShader;
	ptr<gvk::Shader>	m_TerrianGeometryShader;
	ptr<gvk::Shader>	m_TerrianFramgmentShader;

	gvk::ptr<gvk::DescriptorSet> m_TerrianDescriptorSet;

	gvk::ptr<gvk::Buffer>	m_TerrianMaterialBuffer;

	u32						m_TempTerrianVertexCount;
	u32						m_TerrianVertexCount;
	Terrian*				m_Terrian;

	GvkPushConstant			m_TerrianMVPPushConstant;
	GvkPushConstant			m_TerrianCameraPosConstant;
	GvkPushConstant			m_TerrianTimeConstant;
};

class Terrian 
{
	friend class TerrianRenderer;
public:
	Terrian(const std::string& terrian_atlas_path,const std::string& terrian_file,uint seed);
	
	bool Initialize();

	ptr<TerrianRenderer> GetRenderer();

	void  Update(ptr<gvk::Window> window);

	~Terrian();

private:

	ptr<TerrianChunk> GenerateTerrianChunk(i32 x, i32 z);

	ptr<TerrianChunk> LoadTerrianChunk(i32 x, i32 z);

	Vector3i		  ToTerrianChunkIndex(Vector3f pos);

	ptr<TerrianChunk>				m_SwapChunk;
	ptr<TerrianChunk>				m_LoadedChunk;
	//std::vector<ptr<TerrianChunk>>	m_LoadedChunks;
	ptr<TerrianRenderer>			m_Renderer;

	JobStatus						m_ReloadTerrianJob;
	bool							m_TerrianReloadTriggered = false;

	ptr<FTerrian>					m_TerrianFile;

	uint							m_Seed;


	std::string m_TerrianAtlasPath;
	std::string m_TerrianFilePath;
};



