#pragma once
#include "parallel/task.h"
#include "alloc/arena.h"
#include "gvk.h"

class Terrian;
class TerrianRenderer;

class Renderer : public Task 
{
public:

	Renderer(ptr<gvk::Window> window);

	virtual bool		Initialize(TaskManager* manager) ;

	virtual TaskTick	Tick(TaskManager* manager, float delta_time) ;

	virtual void		Finalize(TaskManager* manager) ;

	bool AddTerrian(ptr<Terrian> terrian);
private:

	bool				RecreateOffscreenBuffers();

	ptr<gvk::Context>		m_Context;
	ptr<gvk::Window>		m_Window;
	ptr<gvk::CommandQueue>	m_Queue;
	//currently we only consider single thread renderer
	//only one command pool for one thread
	ptr<gvk::CommandPool>	m_CommandPool;

	u32												m_OffscreenBufferWidth, m_OffscreenBufferHeight;

	ptr<gvk::Shader>								m_PostVertexShader;
	ptr<gvk::Shader>								m_PostFragmentShader;
	ptr<gvk::Pipeline>								m_PostProcessPipeline;
	std::vector<ptr<gvk::DescriptorSet>>			m_PostDescriptorSets;
	VkSampler										m_PostOffScreenBufferSampler;
	
	std::vector<ptr<gvk::Image>>					m_OffscreenBuffers;
	std::vector<VkFramebuffer>						m_OffscreenFrameBuffers;
	std::vector<VkImageView>						m_OffscreenImageView;


	u32												m_OffscreenBufferAttachmentIdx;
	u32												m_BackBufferAttachmentIdx;
	u32												m_DepthStencilBufferAttachmentIdx;

	//forward render pass
	ptr<gvk::RenderPass>							m_ForwardRenderPass;
	static constexpr VkFormat						m_DepthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
	std::vector<ptr<gvk::Image>>					m_DepthStencilBuffers;
	std::vector<VkImageView>						m_DepthStencilBufferViews;

	//frame in flight present fence
	std::vector<VkFence>							m_PostScreenFences;

	ptr<gvk::DescriptorAllocator>					m_DescriptorAllocator;
	VkFormat										m_BackBufferFormat;

	std::string										m_ShaderRootPath;
	std::vector<VkCommandBuffer>					m_CommandBuffers;

	//arena allocator for allocating render objects in every frame
	MemoryArenaAllocator							m_MemoryArenaAllocator;


	u32												m_TerrianPassIdx;
	//u32											m_HizPassIdx;
	//u32											m_CullingPassIdx;
	u32												m_PostProcessPassIdx;

	std::vector<VkSemaphore>						m_FinishSemaphores;

	ptr<TerrianRenderer>							m_TerrianRenderer;
};