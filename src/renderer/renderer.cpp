#include "renderer.h"
#include "robject.h"

#include <map>


class RenderObjectGroupCollector
{
public:
	struct RenderObjectInfo
	{
		u32					descriptor_set_group_count;
		struct
		{
			u32 set_index, set_count;
		} descriptor_set_groups[VKMC_MAX_DESCRIPTOR_SET_COUNT];
		VkDescriptorSet descriptor_sets[VKMC_MAX_DESCRIPTOR_SET_COUNT];
		RenderObject*			object;
	};
private:
	std::map<gvk::Pipeline*, std::vector<RenderObjectInfo>> render_object_groups;
public:
	void Collect(RenderObject* robj,GraphicsObjectContext& ctx)
	{
		RenderObjectInfo info;
		info.descriptor_set_group_count = 0;
		u32 set_cnt = 0, set_idx = 0xffff;
		for (u32 i = 0; i < VKMC_MAX_DESCRIPTOR_SET_COUNT; i++)
		{
			if (ctx.descriptor_sets[i] != nullptr)
			{
				vkmc_debug_assert(ctx.descriptor_sets[i]->GetSetIndex() == i);
				
				if (set_idx != 0xffff)
				{
					set_cnt = 1; set_idx = i;
				}
				else
				{
					set_cnt++;
				}
			}
			else
			{
				auto& group = info.descriptor_set_groups[info.descriptor_set_group_count++];
				group.set_count = set_cnt;
				group.set_index = set_idx;
			}
			info.descriptor_sets[i] = ctx.descriptor_sets[i]->GetDescriptorSet();
		}
		info.object = robj;

		auto iter = render_object_groups.find( ctx.graphics_pipeline);

		if (iter != render_object_groups.end())
		{
			iter->second.push_back(info);
		}
		else
		{
			render_object_groups.insert(make_pair(ctx.graphics_pipeline, std::vector<RenderObjectInfo>{ info }));
		}
	}

	struct Iterator
	{

		Iterator(decltype(render_object_groups)& rgroups)
		{
			curr_group = rgroups.begin();
			curr_group_end = rgroups.end();

			if (curr_group != curr_group_end)
			{
				curr_obj = curr_group->second.begin();
				curr_obj_end = curr_group->second.end();
			}
		}

		opt<gvk::Pipeline*> NextGroup()
		{
			if (curr_group == curr_group_end)
			{
				return std::nullopt;
			}
			curr_obj = curr_group->second.begin();
			curr_obj_end = curr_group->second.end();
			
			gvk::Pipeline* rv = curr_group->first;
			curr_group++;
			return rv;
		}


		opt<RenderObjectInfo*> NextObject()
		{
			if (curr_obj == curr_obj_end)
			{
				return std::nullopt;
			}
			RenderObjectInfo* rv = &*curr_obj;
			curr_obj++;
			return rv;
		}

	private:
		decltype(render_object_groups)::iterator curr_group,curr_group_end;
		std::vector<RenderObjectInfo>::iterator  curr_obj,curr_obj_end;
	};

	Iterator Iterate()
	{
		return Iterator(render_object_groups);
	}

} ;


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

	//create forward pass render pass
	{
		GvkRenderPassCreateInfo info{};
		
		//render target
		info.AddAttachment(0, m_BackBufferFormat,
			VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
			//off screen buffer used as color output in this pass, used as shader resource in the next pass
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//depth stencil buffer
		info.AddAttachment(0, m_DepthStencilFormat,
			VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		//opaque forward pass
		info.AddSubpass(0, VK_PIPELINE_BIND_POINT_GRAPHICS);
		info.AddSubpassColorAttachment(0, 0);
		info.AddSubpassDepthStencilAttachment(0, 1);
		info.AddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

		//TODO: HIZ pass, for culling
		//info.AddSubpass(0, VK_PIPELINE_BIND_POINT_COMPUTE);
		//info.AddSubpassInputAttachment(1, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//info.AddSubpassDependency(0, 1, 
			//VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			//VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);

		//TODO: transparent pass
		//info.AddSubpass(0, VK_PIPELINE_BIND_POINT_GRAPHICS);
		//info.AddSubpassColorAttachment(2, 0);
		//info.AddSubpassDepthStencilAttachment(2, 1);
		//info.AddSubpassDependency(0, 2,
			//VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			//VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);

		match(m_Context->CreateRenderPass(info), pass,
			m_ForwardRenderPass = pass.value(),
			return false
		);
	}

	//create offscreen buffers and frame buffers
	{

		//create depth stencil buffer and view
		GvkImageCreateInfo depth_create_info = GvkImageCreateInfo::Image2D(m_DepthStencilFormat, m_OffscreenBufferHeight, m_OffscreenBufferWidth,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
		match(m_Context->CreateImage(depth_create_info), img,
			m_DepthStencilBuffer = img.value();
			if (m_DepthStencilBuffer->CreateView(gvk::GetAllAspects(m_DepthStencilFormat), 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D).has_value())
				return false;
			,
			return false;
		);


		m_OffscreenBuffers.resize(m_Context->GetBackBufferCount());
		auto& back_buffer_info = m_Context->GetBackBuffers()[0]->Info();
		for (u32 i = 0; i < m_Context->GetBackBufferCount(); i++)
		{
			GvkImageCreateInfo image_create_info = back_buffer_info;
			image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			image_create_info.extent.height = m_OffscreenBufferHeight;
			image_create_info.extent.width = m_OffscreenBufferWidth;
			match(m_Context->CreateImage(image_create_info), img,
				{
					m_OffscreenBuffers[i] = img.value();
					if (!m_OffscreenBuffers[i]->CreateView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D).has_value())
						return false;
				},
				return false
			);

			VkImageView views[] = {m_OffscreenBuffers[i]->GetViews()[0],m_DepthStencilBuffer->GetViews()[0]};
			match(m_Context->CreateFrameBuffer(m_ForwardRenderPass, views, m_OffscreenBufferWidth, m_OffscreenBufferHeight), fb,
				m_OffscreenFrameBuffers[i] = fb.value(),
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
	u32 frame_idx = m_Context->CurrentFrameIndex();
	vkWaitForFences(m_Context->GetDevice(), 1, &m_PostScreenFences[frame_idx], VK_TRUE, UINT64_MAX);
	vkResetFences(m_Context->GetDevice(), 1, &m_PostScreenFences[frame_idx]);

	ptr<gvk::Image> back_buffer;
	VkSemaphore     acquire_image_semaphore;
	u32				image_idx;

	std::string error;
	if (auto img = m_Context->AcquireNextImageAfterResize([&](u32 w, u32 h)
		{
			auto back_buffers = m_Context->GetBackBuffers();
			for (u32 i = 0; i < m_Context->GetBackBufferCount(); i++)
			{
				m_Context->DestroyFrameBuffer(m_PostScreenFrameBuffers[i]);
				match(m_Context->CreateFrameBuffer(m_PostScreenRenderPass, &back_buffers[i]->GetViews()[0],
					back_buffers[i]->Info().extent.width, back_buffers[i]->Info().extent.height), fb,
					m_PostScreenFrameBuffers[i] = fb.value(),
					return false;
				);
			}
			Singleton<EventSystem>::Get().Dispatch(ResizeEvent(w, h));
			//TODO resize offscreen buffers

			return true;
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

	//main queue's semaphore
	gvk::SemaphoreInfo main_queue_semaphore;
	
	VkCommandBuffer cmd = m_CommandBuffers[frame_idx];
	vkResetCommandBuffer(cmd, 0);
	
	VkCommandBufferBeginInfo cmd_begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	vkBeginCommandBuffer(cmd, &cmd_begin);

	m_MemoryArenaAllocator.Reset();
	
	//forward render objects
	std::vector<CustomRenderObject*> before_all_robjs;
	RenderObjectGroupCollector opaque_groups;//, transparent_groups;
	std::vector<CustomRenderObject*> after_transparent_robjs;
	for (auto component : m_RegisteredRenderComponents)
	{
		if (component->Activated())
		{
			RenderObject* robj = component->OnRender(&m_MemoryArenaAllocator);
			if (robj->GetType() == RENDER_OBJECT_FORWARD)
			{
				ForwardRenderObject* frobj = dynamic_cast<ForwardRenderObject*>(robj);
				//get context of forward render object
				GraphicsObjectContext fctx = frobj->Context();
				opaque_groups.Collect(frobj, fctx);
			}
			if (robj->GetType() == RENDER_OBJECT_CUSTOM)
			{
				CustomRenderObject* crobj = dynamic_cast<CustomRenderObject*>(robj);
				switch (crobj->GetOrder())
				{
				case CUSTOM_ROBJECT_ORDERER_BEFORE_ALL:
					before_all_robjs.push_back(crobj);
					break;
				case CUSTOM_ROBJECT_ORDERER_AFTER_TRANSPARENT:
					after_transparent_robjs.push_back(crobj);
					break;
				}
			}
		}
	}
	
	RenderContext rctx;
	rctx.context = m_Context.get();
	rctx.main_cmd_buffer = cmd;
	rctx.main_queue_semaphore = &main_queue_semaphore;
	rctx.frame_index = frame_idx;
	rctx.image_index = image_idx;

	for (auto crobj : before_all_robjs)
	{
		crobj->OnRender(rctx);
	}

	m_ForwardRenderPass->Begin(m_OffscreenFrameBuffers[image_idx], &clear_value,
		render_area, viewport, scissor, cmd)
		.Record([&]() 
	{
		auto iter = opaque_groups.Iterate();
		while (true)
		{
			gvk::Pipeline* pipe;
			match(iter.NextGroup(), p,
				pipe = p.value(),
				break;
			);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->GetPipeline());

			auto robj_iter = iter.NextObject();
			while (robj_iter.has_value())
			{
				//bind descriptor sets
				auto& robj_info = *robj_iter.value();
				for (u32 i = 0; i < robj_info.descriptor_set_group_count; i++)
				{
					auto group = robj_info.descriptor_set_groups[i];
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipe->GetPipelineLayout(), group.set_index, group.set_count,
						&robj_info.descriptor_sets[group.set_index], 0, 0);
				}


				ForwardRenderObject* target_object = dynamic_cast<ForwardRenderObject*>(robj_info.object);
				target_object->Forward(rctx);
			}
		}

		//TODO hiz pass and transparent pass

		
	});

	//render after transparent custom render objects
	for (auto crobj : after_transparent_robjs)
	{
		crobj->OnRender(rctx);
	}

	//post process pass
	m_PostScreenRenderPass->Begin(m_PostScreenFrameBuffers[image_idx], &clear_value,
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

	m_Context->Present(main_queue_semaphore.Wait(m_PostScreenFinishSemaphores[frame_idx],0));
	

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

bool Renderer::RegisterRenderComponent(ptr<RenderComponent> component)
{
	//we should not register repeated components
	if (std::find( m_RegisteredRenderComponents.begin(),m_RegisteredRenderComponents.end(), component)
		== m_RegisteredRenderComponents.end())
	{
		RenderInitContext ctx;
		ctx.context = m_Context.get();
		ctx.descriptor_allocator = m_DescriptorAllocator.get();
		ctx.main_render_pass = m_ForwardRenderPass;
		ctx.forward_pass_idx = 0;
		ctx.transparent_pass_idx = 2;
		ctx.back_buffer_count = m_OffscreenBuffers.size();

		if (component->Initialize(ctx))
		{
			m_RegisteredRenderComponents.push_back(component);
		}
		else
		{
			//fail to initialize component -> 
			return false;
		}
	}
	return true;
}

void Renderer::UnregisterRenderComponent(ptr<RenderComponent> component)
{
	auto pos = std::find(m_RegisteredRenderComponents.begin(), m_RegisteredRenderComponents.end(), component);
	if (pos != m_RegisteredRenderComponents.end())
	{
		RenderFinalizeContext ctx;
		ctx.context = m_Context.get();
		(*pos)->Finalize(ctx);

		m_RegisteredRenderComponents.erase(pos);
	}
}



std::tuple<u32, u32> Renderer::GetCurrentBackbufferExtent()
{
	auto extent = m_Context->GetBackBuffers()[0]->Info().extent;
	return std::make_tuple(extent.width, extent.height);
}

Renderer::Renderer(float offscree_scale_ratio, ptr<gvk::Window> window)
:m_Window(window),m_OffscreenScaleRatio(offscree_scale_ratio),m_PostOffScreenBufferSampler(NULL)
{
	u32 win_height = window->GetHeight(), win_width = window->GetWidth();
	m_OffscreenBufferHeight = (u32)((float)win_height * offscree_scale_ratio);
	m_OffscreenBufferWidth  = (u32)((float)win_width  * offscree_scale_ratio);
	m_BackBufferFormat      = VK_FORMAT_UNDEFINED;
}

