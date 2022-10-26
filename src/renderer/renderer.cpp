#include "renderer.h"

size_t Renderer::TaskID()
{
	return typeid(Renderer).hash_code();
}

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
		info.RequireQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT, 1);
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

	//create offscreen buffers
	{
		m_OffscreenBuffers.resize(m_Context->GetBackBufferCount());
		auto& back_buffer_info = m_Context->GetBackBuffers()[0]->Info();
		for (u32 i = 0;i < m_Context->GetBackBufferCount();i++)
		{
			GvkImageCreateInfo image_create_info = back_buffer_info;
			image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | 
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			image_create_info.extent.height = m_OffscreenBufferHeight;
			image_create_info.extent.width  = m_OffscreenBufferWidth ;
			match(m_Context->CreateImage(image_create_info), img,
			{
				m_OffscreenBuffers[i] = img.value();
				if (!m_OffscreenBuffers[i]->CreateView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D).has_value())
					return false;
			},
				return false
			);
			
		}
	}

	//create render pass and framebuffer for post screen pass semaphore and fences
	{
		GvkRenderPassCreateInfo render_pass_create{};
		render_pass_create.AddAttachment(0, m_BackBufferFormat,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		render_pass_create.AddSubpass(0, VK_PIPELINE_BIND_POINT_GRAPHICS);
		render_pass_create.AddSubpassColorAttachment(0, 0);
		//wait for off screen pass finishes rendering
		render_pass_create.AddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

		match(m_Context->CreateRenderPass(render_pass_create), rp,
			m_PostScreenRenderPass = rp.value(),
			return false
		);

		m_PostScreenFrameBuffers.resize(m_Context->GetBackBufferCount());
		auto back_buffers = m_Context->GetBackBuffers();
		for (u32 i = 0;i < m_Context->GetBackBufferCount();i++) 
		{
			match(m_Context->CreateFrameBuffer(m_PostScreenRenderPass, &back_buffers[i]->GetViews()[0],
				back_buffers[i]->Info().extent.width, back_buffers[i]->Info().extent.height), fb,
				m_PostScreenFrameBuffers[i] = fb.value(),
				return false;
			);
		}

		m_PostScreenFinishSemaphores.resize(m_Context->GetBackBufferCount());
		m_PostScreenFences.resize(m_Context->GetBackBufferCount());
		m_PostScreenFences.resize(m_Context->GetBackBufferCount());
		for (u32 i = 0;i < m_Context->GetBackBufferCount();i++) 
		{
			match(m_Context->CreateVkSemaphore(), s,
				m_PostScreenFinishSemaphores[i] = s.value(),
				return false;
			);
			match(m_Context->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT), f,
				m_PostScreenFences[i] = f.value(),
				return false
			);
		}
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
			m_PostScreenRenderPass,0,blend_states.data());
		
		match(m_Context->CreateGraphicsPipeline(graphics_pipeline_create), gp,
			m_PostScreenPipeline = gp.value(),
			return false;
		);
		
		//create descriptor set for post processing and write descriptors
		auto descriptor_set_layout = m_PostScreenPipeline->GetInternalLayout(0, VK_SHADER_STAGE_FRAGMENT_BIT).value();
		m_PostDescriptorSets.resize(m_Context->GetBackBufferCount());
		GvkDescriptorSetWrite descriptor_set_write;
		for (u32 i = 0; i < m_Context->GetBackBufferCount();i++) {
			match(m_DescriptorAllocator->Allocate(descriptor_set_layout), ds,
				m_PostDescriptorSets[i] = ds.value(),
				return false;
			);
			descriptor_set_write.ImageWrite(m_PostDescriptorSets[i], "off_screen_buffer", m_PostOffScreenBufferSampler,
				m_OffscreenBuffers[i]->GetViews()[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		descriptor_set_write.Emit(m_Context->GetDevice());

		match(m_Context->CreateSampler(GvkSamplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR)), s,
			m_PostOffScreenBufferSampler = s.value(),
			return false;
		);

	}
}

TaskTick Renderer::Tick(TaskManager* manager, float delta_time)
{
	ptr<gvk::Image> back_buffer;
	VkSemaphore     acquire_image_semaphore;
	if (auto img = m_Context->AcquireNextImage();img.has_value()) 
	{
		auto [b, a] = img.value();
		back_buffer = b;
		acquire_image_semaphore = a;
	}
	else 
	{
		return TASK_TICK_ERROR;
	}
	
	u32 frame_idx = m_Context->CurrentFrameIndex();
	VkCommandBuffer cmd = m_CommandBuffers[frame_idx];
	vkResetCommandBuffer(cmd, 0);
	
	VkCommandBufferBeginInfo cmd_begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	vkBeginCommandBuffer(cmd, &cmd_begin);
	//TODO render codes

	
	
	
	//post process pass
	VkClearValue clear_value{};
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

	m_PostScreenRenderPass->Begin(m_PostScreenFrameBuffers[frame_idx], &clear_value,
		render_area, viewport, scissor, cmd).Record(
			[&]() {
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostScreenPipeline->GetPipeline());
				VkDescriptorSet descriptor_sets[] = {m_PostDescriptorSets[frame_idx]->GetDescriptorSet()};
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostScreenPipeline->GetPipelineLayout(),
					0, vkmc_count(descriptor_sets), descriptor_sets, 0, NULL);
				//post screen pass don't have any vertex buffer to bind
				vkCmdDraw(cmd, 6, 1, 0, 0);
			}
	);

	vkEndCommandBuffer(cmd);

	m_Queue->Submit(&cmd, 1, gvk::SemaphoreInfo()
		.Wait(acquire_image_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
		.Signal(m_PostScreenFinishSemaphores[frame_idx]),m_PostScreenFences[frame_idx]);

	m_Context->Present(gvk::SemaphoreInfo().Wait(m_PostScreenFinishSemaphores[frame_idx],0));
	
	vkWaitForFences(m_Context->GetDevice(), 1, &m_PostScreenFences[frame_idx], VK_TRUE, UINT64_MAX);

	return TASK_TICK_CONTINUE;
}

void Renderer::Finalize(TaskManager* manager)
{
	for (auto f : m_PostScreenFences) 
	{
		m_Context->DestroyFence(f);
	}
	for (auto s : m_PostScreenFinishSemaphores)
	{
		m_Context->DestroyVkSemaphore(s);
	}
	m_PostVertexShader = nullptr;
	m_PostFragmentShader = nullptr;
	m_PostScreenPipeline = nullptr; 
	for (auto& s : m_PostDescriptorSets) s = nullptr;
	m_Context->DestroySampler(m_PostOffScreenBufferSampler);
	m_PostScreenRenderPass = nullptr;
	for (auto& fb : m_PostScreenFrameBuffers) m_Context->DestroyFrameBuffer(fb);
	m_DescriptorAllocator = nullptr;
	for (auto& ofb : m_OffscreenBuffers) ofb = nullptr;
	m_CommandPool = nullptr;
	m_Queue = nullptr;
	m_Context = nullptr;
	m_Window = nullptr;
}

Renderer::Renderer(float offscree_scale_ratio, ptr<gvk::Window> window)
:m_Window(window),m_OffscreenScaleRatio(offscree_scale_ratio),m_PostOffScreenBufferSampler(NULL)
{
	u32 win_height = window->GetHeight(), win_width = window->GetWidth();
	m_OffscreenBufferHeight = (u32)((float)win_height * offscree_scale_ratio);
	m_OffscreenBufferWidth  = (u32)((float)win_width  * offscree_scale_ratio);
	m_BackBufferFormat      = VK_FORMAT_UNDEFINED;
}

