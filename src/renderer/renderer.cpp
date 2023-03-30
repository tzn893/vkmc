#include "renderer.h"
#include "terrian/terrian.h"

#include "world/camera.h"

#include <map>


bool Renderer::Initialize(TaskManager* manager)
{
	std::string error;

	match(gvk::Context::CreateContext("vulkan minecraft", GVK_VERSION{ 1,0,0 }, VK_API_VERSION_1_3, m_Window, &error),c,
		m_Context = c.value(),
		return false
	);

	//create instance
	{
		GvkInstanceCreateInfo info;
		info.AddInstanceExtension(GVK_INSTANCE_EXTENSION_DEBUG);
		info.AddLayer(GVK_LAYER_DEBUG);
		if (!m_Context->InitializeInstance(info, &error))
		{
			return false;
		}
	}

	//create device
	{
		GvkDeviceCreateInfo info;
		info.AddDeviceExtension(GVK_DEVICE_EXTENSION_SWAP_CHAIN);
		info.AddDeviceExtension(GVK_DEVICE_EXTENSION_GEOMETRY_SHADER);
		info.AddDeviceExtension(GVK_DEVICE_EXTENSION_DEBUG_MARKER);
		info.RequireQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT, 1);
		info.RequireQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, 3);
		if (!m_Context->InitializeDevice(info,&error)) 
		{
			return false;
		}
	}
	
	//create swapchain
	{
		m_BackBufferFormat = m_Context->PickBackbufferFormatByHint({ VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_B8G8R8_UNORM });
		if (!m_Context->CreateSwapChain(m_BackBufferFormat, &error)) 
		{
			return false;
		}
	}

	//command queue,command buffer,command pool
	{
		m_CommandBuffers.resize(m_Context->GetBackBufferCount());

		match(m_Context->CreateQueue(VK_QUEUE_GRAPHICS_BIT), q,
			m_Queue = q.value(),
			return false;
		);
		match(m_Context->CreateCommandPool(m_Queue.get()), p,
			m_CommandPool = p.value(),
			return false;
		);
		for (u32 i = 0;i < m_Context->GetBackBufferCount();i++) 
		{
			match(m_CommandPool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY), b,
				m_CommandBuffers[i] = b.value(),
				return false;
			);
		}
	}

	//create forward pass render pass
	{
		GvkRenderPassCreateInfo info{};
		
		//render target
		m_OffscreenBufferAttachmentIdx = info.AddAttachment(0, m_BackBufferFormat,
			VK_SAMPLE_COUNT_1_BIT, 
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
			//off screen buffer used as color output in this pass, used as shader resource in the next pass
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//depth stencil buffer
		m_DepthStencilBufferAttachmentIdx = info.AddAttachment(0, m_DepthStencilFormat,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		m_BackBufferAttachmentIdx = info.AddAttachment(0, m_BackBufferFormat,
			VK_SAMPLE_COUNT_1_BIT, 
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
			//off screen buffer used as color output in this pass, used as shader resource in the next pass
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//TODO: HIZ pass, for culling
		//info.AddSubpass(0, VK_PIPELINE_BIND_POINT_COMPUTE);
		//info.AddSubpassInputAttachment(1, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//info.AddSubpassDependency(0, 1, 
			//VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			//VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);

		//terrian forward pass
		m_TerrianPassIdx = info.AddSubpass(0, VK_PIPELINE_BIND_POINT_GRAPHICS);
		info.AddSubpassColorAttachment(m_TerrianPassIdx, m_OffscreenBufferAttachmentIdx);
		info.AddSubpassDepthStencilAttachment(m_TerrianPassIdx, m_DepthStencilBufferAttachmentIdx);
		info.AddSubpassDependency(VK_SUBPASS_EXTERNAL, m_TerrianPassIdx,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
		

		//TODO: transparent pass
		//info.AddSubpass(0, VK_PIPELINE_BIND_POINT_GRAPHICS);
		//info.AddSubpassColorAttachment(2, 0);
		//info.AddSubpassDepthStencilAttachment(2, 1);
		//info.AddSubpassDependency(0, 2,
			//VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			//VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);

		//post process pass
		m_PostProcessPassIdx = info.AddSubpass(0, VK_PIPELINE_BIND_POINT_GRAPHICS);
		info.AddSubpassColorAttachment(m_PostProcessPassIdx, m_BackBufferAttachmentIdx);
		info.AddSubpassInputAttachment(m_PostProcessPassIdx, m_OffscreenBufferAttachmentIdx, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//wait for off screen pass finishes rendering
		info.AddSubpassDependency(m_TerrianPassIdx, m_PostProcessPassIdx,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

		match(m_Context->CreateRenderPass(info), pass,
			m_ForwardRenderPass = pass.value(),
			return false
		);

		m_ForwardRenderPass->SetDebugName("Forward Render Pass");
	}

	

	//create global descriptor allocator
	{
		m_DescriptorAllocator = m_Context->CreateDescriptorAllocator();
	}

	//compile post processing shader,create shader pipeline
	{
		m_ShaderRootPath = std::string(VKMC_ROOT_DIRECTORY) + "/src/renderer/shaders";
		const char* shader_pathes[] = {m_ShaderRootPath.c_str()};
		match(m_Context->CompileShader("post.vert", gvk::ShaderMacros(),
			shader_pathes, vkmc_count(shader_pathes),
			shader_pathes, vkmc_count(shader_pathes),
			&error), vert,

			m_PostVertexShader = vert.value(),
			return false;
		);

		match(m_Context->CompileShader("post.frag", gvk::ShaderMacros(),
			shader_pathes, vkmc_count(shader_pathes),
			shader_pathes, vkmc_count(shader_pathes),
			&error), frag,

			m_PostFragmentShader = frag.value(),
			return false;
		);

		std::vector<GvkGraphicsPipelineCreateInfo::BlendState> blend_states(1, GvkGraphicsPipelineCreateInfo::BlendState());
		GvkGraphicsPipelineCreateInfo graphics_pipeline_create(m_PostVertexShader,m_PostFragmentShader,
			m_ForwardRenderPass, m_PostProcessPassIdx ,blend_states.data());
		
		match(
			m_Context->CreateGraphicsPipeline(graphics_pipeline_create), gp,
			m_PostProcessPipeline = gp.value(),
			return false;
		);
		m_PostProcessPipeline->SetDebugName("post process");

		match(m_Context->CreateSampler(GvkSamplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR)), s,
			m_PostOffScreenBufferSampler = s.value(),
			return false;
		);
		m_Context->SetDebugNameSampler(m_PostOffScreenBufferSampler, "post offscreen sampler");
		
		//create descriptor set for post processing and write descriptors
		auto descriptor_set_layout = m_PostProcessPipeline->GetInternalLayout(0, VK_SHADER_STAGE_FRAGMENT_BIT).value();
		m_PostDescriptorSets.resize(m_Context->GetBackBufferCount());
		GvkDescriptorSetWrite descriptor_set_write;
		for (u32 i = 0; i < m_Context->GetBackBufferCount();i++) {
			match(m_DescriptorAllocator->Allocate(descriptor_set_layout), ds,
				m_PostDescriptorSets[i] = ds.value(),
				return false;
			);
		}
		descriptor_set_write.Emit(m_Context->GetDevice());
	}


	//create offscreen buffers and frame buffers	
	if (!RecreateOffscreenBuffers())
		return false;
	//create finish semaphores and fences
	{
		m_FinishSemaphores.resize(m_Context->GetBackBufferCount());	
		m_PostScreenFences.resize(m_Context->GetBackBufferCount());

		for (uint i = 0; i < m_Context->GetBackBufferCount(); i++)
		{
			match
			(
				m_Context->CreateVkSemaphore(), semaphore,
				m_FinishSemaphores[i] = semaphore.value(),
				return false
			);
			match
			(
				m_Context->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT), fence,
				m_PostScreenFences[i] = fence.value(),
				return false
			);
		}
	}

	return true;
}

TaskTick Renderer::Tick(TaskManager* manager, float delta_time)
{
	u32 frame_idx = m_Context->CurrentFrameIndex();
	vkWaitForFences(m_Context->GetDevice(), 1, &m_PostScreenFences[frame_idx], VK_TRUE, UINT64_MAX);
	vkResetFences(m_Context->GetDevice(), 1, &m_PostScreenFences[frame_idx]);

	ptr<gvk::Image> back_buffer;
	VkSemaphore     acquire_image_semaphore;
	u32				image_idx;

	std::string error;
	if (auto img = m_Context->AcquireNextImageAfterResize([&](u32 w, u32 h)
		{
			return RecreateOffscreenBuffers();
		},&error); img.has_value())
	{
		auto [b, a,i] = img.value();
		back_buffer = b;
		acquire_image_semaphore = a;
		image_idx = i;
	}
	else 
	{
		return TASK_TICK_ERROR;
	}

	//TODO : changing view port and scissor rects
	VkRect2D	 render_area = { {0,0},{back_buffer->Info().extent.width,back_buffer->Info().extent.height} };
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)back_buffer->Info().extent.width;
	viewport.height = (float)back_buffer->Info().extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = VkExtent2D{ back_buffer->Info().extent.width,
		back_buffer->Info().extent.height };

	//main queue's semaphore
	VkCommandBuffer cmd = m_CommandBuffers[frame_idx];
	vkResetCommandBuffer(cmd, 0);
	
	VkCommandBufferBeginInfo cmd_begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	vkBeginCommandBuffer(cmd, &cmd_begin);

	GvkDebugMarker marker(cmd, "vkmc", GVK_MARKER_COLOR_GRAY);

	m_MemoryArenaAllocator.Reset();

	//clear screen before everything start
	VkClearValue color_clear{ 0,0,0,1.0f };
	VkClearValue depth_stencil_clear;
	depth_stencil_clear.depthStencil = { 1.0f, 0 };
	VkClearValue clear_values[] = { color_clear,depth_stencil_clear };
	
	RenderCamera camera = Singleton<MainCamera>().Get().OnRender();

	m_ForwardRenderPass->Begin(m_OffscreenFrameBuffers[image_idx], clear_values,
		render_area, viewport, scissor, cmd)
		.NextSubPass([&]()
		{
			GvkDebugMarker marker(cmd, "render terrian", GVK_MARKER_COLOR_GREEN);

			m_TerrianRenderer->Render(cmd, camera);
		}
		)
		//TODO hiz pass and transparent pass
		//TODO post process 
		.EndPass([&]()
		{
			GvkDebugMarker marker(cmd, "post process", GVK_MARKER_COLOR_YELLOW);

			GvkBindPipeline(cmd,m_PostProcessPipeline);
			
			GvkDescriptorSetBindingUpdate(cmd,m_PostProcessPipeline)
			.BindDescriptorSet(m_PostDescriptorSets[frame_idx])
			.Update();

			//post screen pass don't have any vertex buffer to bind
			vkCmdDraw(cmd, 6, 1, 0, 0);
		}
	);

	marker.End();
	vkEndCommandBuffer(cmd);

	m_Queue->Submit(&cmd, 1, gvk::SemaphoreInfo()
		.Wait(acquire_image_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
		.Signal(m_FinishSemaphores[frame_idx]),
		m_PostScreenFences[frame_idx]);

	m_Context->Present(gvk::SemaphoreInfo().Wait(m_FinishSemaphores[frame_idx], 0));

	return TASK_TICK_CONTINUE;
}

void Renderer::Finalize(TaskManager* manager)
{
	for (auto f : m_PostScreenFences) 
	{
		m_Context->DestroyFence(f);
	}
	m_PostVertexShader = nullptr;
	m_PostFragmentShader = nullptr;
	m_PostProcessPipeline = nullptr; 
	for (auto& s : m_PostDescriptorSets) s = nullptr;
	m_Context->DestroySampler(m_PostOffScreenBufferSampler);
	m_DescriptorAllocator = nullptr;
	for (auto& ofb : m_OffscreenBuffers) ofb = nullptr;
	m_CommandPool = nullptr;
	m_Queue = nullptr;
	m_Context = nullptr;
	m_Window = nullptr;
}

bool Renderer::AddTerrian(ptr<Terrian> terrian)
{
	m_TerrianRenderer = terrian->GetRenderer();

	UploadQueue upload_queue(m_Queue, m_Context);

	std::string error;

	if (!m_TerrianRenderer->Initialize(m_Context, m_ForwardRenderPass, m_TerrianPassIdx,
		upload_queue, m_DescriptorAllocator, error))
	{
		return false;
	}

	m_TerrianRenderer->UpdateRenderData();

	return true;
}

bool Renderer::RecreateOffscreenBuffers()
{
	m_DepthStencilBufferViews.resize(m_Context->GetBackBufferCount(), NULL);
	m_DepthStencilBuffers.resize(m_Context->GetBackBufferCount(), NULL);
	m_OffscreenBuffers.resize(m_Context->GetBackBufferCount(), NULL);
	m_OffscreenFrameBuffers.resize(m_Context->GetBackBufferCount(),NULL);
	m_OffscreenImageView.resize(m_Context->GetBackBufferCount(), NULL);

	for (u32 i = 0; i < m_Context->GetBackBufferCount(); i++)
	{
		if(m_OffscreenBuffers[i] != NULL)
			m_Context->DestroyFrameBuffer(m_OffscreenFrameBuffers[i]);
		m_OffscreenBuffers[i] = NULL;
	}

	auto& back_buffer_info = m_Context->GetBackBuffers()[0]->Info();

	m_OffscreenBufferHeight = (u32)(back_buffer_info.extent.height);
	m_OffscreenBufferWidth  = (u32)(back_buffer_info.extent.width);

	GvkImageCreateInfo depth_create_info = GvkImageCreateInfo::Image2D(m_DepthStencilFormat, m_OffscreenBufferWidth, m_OffscreenBufferHeight,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	
	GvkDescriptorSetWrite descriptor_set_write;

	for (u32 i = 0; i < m_Context->GetBackBufferCount(); i++)
	{
		auto& back_buffer = m_Context->GetBackBuffers()[i];
		back_buffer->SetDebugName("back buffer " + std::to_string(i));
		m_Context->SetDebugNameImageView(back_buffer->GetViews()[0], "back buffer view " + std::to_string(i));

		GvkImageCreateInfo image_create_info = back_buffer_info;
		image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT 
			| VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT 
			| VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT 
			| VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image_create_info.extent.height = m_OffscreenBufferHeight;
		image_create_info.extent.width = m_OffscreenBufferWidth;
		match(m_Context->CreateImage(image_create_info), img,
			{
				m_OffscreenBuffers[i] = img.value();
			},
			return false
		);
		m_OffscreenBuffers[i]->SetDebugName("offscreen buffer " + std::to_string(i));

		match(m_Context->CreateImage(depth_create_info), img,
			m_DepthStencilBuffers[i] = img.value();
			m_DepthStencilBuffers[i]->SetDebugName("depth stencil buffer " + std::to_string(i));
			if (auto v = m_DepthStencilBuffers[i]->CreateView(gvk::GetAllAspects(m_DepthStencilFormat), 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D); v.has_value())
			{
				m_DepthStencilBufferViews[i] = v.value();
			}
			else
			{
				return false;
			}
		,
			return false;
		);

		match(m_OffscreenBuffers[i]->CreateView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D), view,
			m_OffscreenImageView[i] = view.value(),
			return false;
		);

		VkImageView views[3];
		views[m_OffscreenBufferAttachmentIdx] = m_OffscreenImageView[i];
		views[m_BackBufferAttachmentIdx] = back_buffer->GetViews()[0];
		views[m_DepthStencilBufferAttachmentIdx] = m_DepthStencilBufferViews[i];

		match
		(
			m_Context->CreateFrameBuffer(m_ForwardRenderPass, views, m_OffscreenBufferWidth, m_OffscreenBufferHeight), f,
			m_OffscreenFrameBuffers[i] = f.value(),
			return false
		);

		descriptor_set_write.ImageWrite(m_PostDescriptorSets[i], "off_screen_buffer", m_PostOffScreenBufferSampler,
			m_OffscreenImageView[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	descriptor_set_write.Emit(m_Context->GetDevice());

	Singleton<MainCamera>().Get().OnResize(m_OffscreenBufferWidth, m_OffscreenBufferHeight);

	return true;
}

Renderer::Renderer(ptr<gvk::Window> window)
: m_Window(window),m_PostOffScreenBufferSampler(NULL)
{
	u32 win_height = window->GetHeight(), win_width = window->GetWidth();
	m_OffscreenBufferHeight = win_height;
	m_OffscreenBufferWidth  = win_width;
	m_BackBufferFormat      = VK_FORMAT_UNDEFINED;
}

