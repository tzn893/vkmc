#include "skybox.h"
#include "renderer/shaders/skybox_shader_common.h"

Skybox::Skybox(CubeMapDesc desc)
{
	m_Desc = desc;
	m_SkyboxImageView = NULL;
}

bool Skybox::Initialize(gvk::ptr<gvk::Context> ctx, gvk::ptr<gvk::RenderPass> render_pass, uint subpass_idx, UploadQueue& upload_queue, gvk::ptr<gvk::DescriptorAllocator> desc_alloc, std::string& error)
{
	{
		match
		(
			upload_queue.UploadImageCube(m_Desc), img,
			m_SkyboxImage = img.value(),
			return false;
		);
		m_SkyboxImage->SetDebugName("sky box");

		match
		(
			m_SkyboxImage->CreateView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6, VK_IMAGE_VIEW_TYPE_CUBE), v,
			m_SkyboxImageView = v.value(),
			return false;
		);
	}

	{
		std::string skybox_shader_root_path = std::string(VKMC_ROOT_DIRECTORY) + "/src/renderer/shaders";
		const char* shader_pathes[] = { skybox_shader_root_path.c_str() };
		match(
			ctx->CompileShader("skybox.vert", gvk::ShaderMacros(),
			shader_pathes, vkmc_count(shader_pathes),
			shader_pathes, vkmc_count(shader_pathes),
			&error), vert,
			m_SkyboxVertexShader = vert.value(),
			return false;
		);

		match(
			ctx->CompileShader("skybox.frag", gvk::ShaderMacros(),
			shader_pathes, vkmc_count(shader_pathes),
			shader_pathes, vkmc_count(shader_pathes),
			&error), frag,
			m_SkyboxFragmentShader = frag.value(),
			return false;
		);

		GvkGraphicsPipelineCreateInfo info(m_SkyboxVertexShader, m_SkyboxFragmentShader,
			render_pass, subpass_idx);
		info.rasterization_state.cullMode = VK_CULL_MODE_NONE;
		info.depth_stencil_state.enable_depth_stencil = true;
		info.depth_stencil_state.depthWriteEnable = VK_FALSE;

		match(
			ctx->CreateGraphicsPipeline(info), pipe,
			m_SkyboxPipeline = pipe.value(),
			return false
		);
		m_SkyboxPipeline->SetDebugName("sky box pipeline");

		match(
			m_SkyboxPipeline->GetPushConstantRange("p_vp"),pc,
			m_SkyboxVPPushConstant = pc.value(),
			return false
		);
	}

	{
		ptr<gvk::DescriptorSetLayout> layout;
		match(
			m_SkyboxPipeline->GetInternalLayout(0, VK_SHADER_STAGE_FRAGMENT_BIT), l,
			layout = l.value(),
			return false
		);

		match(
			desc_alloc->Allocate(layout),desc,
			m_SkyboxDescriptorSet = desc.value(),
			return false
		);

		match(
			ctx->CreateSampler(GvkSamplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR)), sampler,
			m_SkyboxSampler = sampler.value(),
			return false;
		);

		GvkDescriptorSetWrite()
		.ImageWrite(m_SkyboxDescriptorSet, "sky_texture", m_SkyboxSampler, m_SkyboxImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		.Emit(ctx->GetDevice());
	}
	
	//create vertex buffer and index buffer
	{
		SkyboxVertex verts[] =
		{
			{-1,-1,-1},
			{-1,-1, 1},
			{-1, 1,-1},
			{-1, 1, 1},
			{ 1,-1,-1},
			{ 1,-1, 1},
			{ 1, 1,-1},
			{ 1, 1, 1}
		};

		u16 indices[] =
		{
			0,1,2, 1,2,3,
			5,1,7, 1,7,3,
			4,5,6, 5,6,7,
			0,4,2, 4,2,6,
			7,6,3, 6,3,2,
			0,1,4, 1,4,5
		};

		match(
			ctx->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(verts), GVK_HOST_WRITE_SEQUENTIAL), buf,
			m_SkyboxVertexBuffer = buf.value(),
			return false;
		);
		m_SkyboxVertexBuffer->SetDebugName("sky box vertex buffer");
		m_SkyboxVertexBuffer->Write(verts, 0, sizeof(verts));

		match(
			ctx->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sizeof(indices), GVK_HOST_WRITE_SEQUENTIAL), idx,
			m_SkyboxIndexbuffer = idx.value(),
			return false;
		);
		m_SkyboxIndexbuffer->SetDebugName("sky box index buffer");
		m_SkyboxIndexbuffer->Write(indices, 0, sizeof(indices));
	};

	return true;
}

void Skybox::Render(VkCommandBuffer buffer, const RenderCamera& camera)
{
	
	GvkDebugMarker(buffer, "sky box", GVK_MARKER_COLOR_BLUE);

	Mat4x4 view = Math::look_at(Vector3f(0.f), camera.front, Vector3f(0, 1, 0));
	Mat4x4 VP = camera.P * view;

	m_SkyboxVPPushConstant.Update(buffer, &VP);

	GvkBindPipeline(buffer, m_SkyboxPipeline);

	GvkDescriptorSetBindingUpdate(buffer, m_SkyboxPipeline)
	.BindDescriptorSet(m_SkyboxDescriptorSet)
	.Update();

	GvkBindVertexIndexBuffers(buffer)
	.BindIndex(m_SkyboxIndexbuffer, VK_INDEX_TYPE_UINT16)
	.BindVertex(m_SkyboxVertexBuffer, 0)
	.Emit();

	vkCmdDrawIndexed(buffer, 36, 1, 0, 0, 0);
}

