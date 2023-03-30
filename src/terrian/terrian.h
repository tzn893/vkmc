#pragma once
#include <gvk.h>
#include <mutex>

#include "common.h"
#include "block.h"
#include "world/camera.h"
#include "renderer/upload.h"

#include "terrian/shaders/terrian_shader_common.h"


class Terrian;

class TerrianRenderer 
{
public:
	TerrianRenderer(const std::string& atlas_path, Terrian* terrian);

	bool Initialize(gvk::ptr<gvk::Context> ctx,gvk::ptr<gvk::RenderPass> render_pass,
		uint subpass_idx, UploadQueue& upload_queue, gvk::ptr<gvk::DescriptorAllocator> alloc,std::string& error);

	void UpdateRenderData();

	void Render(VkCommandBuffer cmd,const RenderCamera& camera);

	void Release(gvk::ptr<gvk::Context>& ctx);

private:
	gvk::ptr<gvk::Pipeline> m_TerrianPipeline;

	gvk::ptr<gvk::Image>	m_TerrianAtlas;
	std::string				m_TerrianAtlasPath;
	VkImageView				m_TerrianAtlasView;
	VkSampler				m_TerrianAtlasSampler;

	ptr<gvk::Buffer>	m_TerrianVertexBuffer;
	ptr<gvk::Shader>	m_TerrianVertexShader;
	ptr<gvk::Shader>	m_TerrianGeometryShader;
	ptr<gvk::Shader>	m_TerrianFramgmentShader;

	gvk::ptr<gvk::DescriptorSet> m_TerrianDescriptorSet;

	gvk::ptr<gvk::Buffer>	m_TerrianMaterialBuffer;

	u32						m_TerrianVertexCount;
	Terrian*				m_Terrian;

	TerrianVertex*			m_TerrianVertexData;

	GvkPushConstant			m_TerrianMVPPushConstant;
	GvkPushConstant			m_TerrianCameraPosConstant;
	GvkPushConstant			m_TerrianTimeConstant;
};

class Terrian 
{
	friend class TerrianRenderer;
public:
	Terrian(const std::string& terrian_atlas_path,uint seed);
	
	bool Initialize();

	ptr<TerrianRenderer> GetRenderer();

private:

	void GenerateTerrianChunk(i32 x, i32 z);

	std::mutex						m_AccessLock;

	ptr<TerrianChunk>				m_LoadedChunk;
	//std::vector<ptr<TerrianChunk>>	m_LoadedChunks;
	ptr<TerrianRenderer>			m_Renderer;

	uint							m_Seed;


	std::string m_TerrianAtlasPath;
};



