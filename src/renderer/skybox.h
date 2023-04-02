#pragma once
#include "gvk.h"
#include "upload.h"
#include "world/camera.h"




class Skybox
{
public:
	Skybox(CubeMapDesc desc);

	bool Initialize(gvk::ptr<gvk::Context> ctx, gvk::ptr<gvk::RenderPass> render_pass, uint subpass_idx, UploadQueue& upload_quque,
		gvk::ptr<gvk::DescriptorAllocator> desc_alloc, std::string& error);

	void Render(VkCommandBuffer buffer, const RenderCamera& camera);
private:
	CubeMapDesc m_Desc;

	ptr<gvk::Pipeline>		m_SkyboxPipeline;
	ptr<gvk::Shader>		m_SkyboxVertexShader;
	ptr<gvk::Shader>		m_SkyboxFragmentShader;

	ptr<gvk::Buffer>		m_SkyboxVertexBuffer;
	ptr<gvk::Buffer>		m_SkyboxIndexbuffer;
	ptr<gvk::DescriptorSet> m_SkyboxDescriptorSet;
	ptr<gvk::Image>			m_SkyboxImage;
	VkImageView				m_SkyboxImageView;

	GvkPushConstant			m_SkyboxVPPushConstant;
	VkSampler				m_SkyboxSampler;
};
