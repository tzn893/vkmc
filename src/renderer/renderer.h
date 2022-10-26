#pragma once
#include "parallel/task.h"
#include "gvk.h"

class Renderer : public Task 
{
public:

	Renderer(float offscree_scale_ratio,ptr<gvk::Window> window);

	virtual size_t		TaskID() ;

	virtual bool		Initialize(TaskManager* manager) ;

	virtual TaskTick	Tick(TaskManager* manager, float delta_time) ;

	virtual void		Finalize(TaskManager* manager) ;



private:
	ptr<gvk::Context>		m_Context;
	ptr<gvk::Window>		m_Window;
	ptr<gvk::CommandQueue>	m_Queue;
	//currently we only consider single thread renderer
	//only one command pool for one thread
	ptr<gvk::CommandPool>	m_CommandPool;

	float											m_OffscreenScaleRatio;
	u32												m_OffscreenBufferWidth, m_OffscreenBufferHeight;
	ptr<gvk::Shader>								m_PostVertexShader;
	ptr<gvk::Shader>								m_PostFragmentShader;
	ptr<gvk::Pipeline>								m_PostScreenPipeline;
	std::vector<ptr<gvk::DescriptorSet>>			m_PostDescriptorSets;
	VkSampler										m_PostOffScreenBufferSampler;
	//render pass for post screen operations
	ptr<gvk::RenderPass>							m_PostScreenRenderPass;
	std::vector<VkFramebuffer>						m_PostScreenFrameBuffers;
	//for synchronization between offscreen pass and swap chain
	std::vector<VkSemaphore>						m_PostScreenFinishSemaphores;
	std::vector<ptr<gvk::Image>>					m_OffscreenBuffers;

	//frame in flight present fence
	std::vector<VkFence>							m_PostScreenFences;

	ptr<gvk::DescriptorAllocator>					m_DescriptorAllocator;
	VkFormat										m_BackBufferFormat;


	std::string										m_ShaderRootPath;
	std::vector<VkCommandBuffer>					m_CommandBuffers ;
};