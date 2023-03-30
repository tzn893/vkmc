#include "upload.h"
#include "stb_image.h"



UploadQueue::UploadQueue(ptr<gvk::CommandQueue>& queue,ptr<gvk::Context>& ctx)
{
	m_upload_queue = queue;
	m_ctx = ctx;
}


opt<ptr<gvk::Image>> UploadQueue::UploadImage2D(const std::string& image_path)
{
	int x, y, comp;
	void* data = stbi_load(image_path.c_str(), &x, &y, &comp, 4);

	if (data == nullptr)
	{
		return std::nullopt;
	}

	ptr<gvk::Image> image;
	
	VkFormat format = VK_FORMAT_UNDEFINED;
	switch (comp)
	{
		case 3: format = VK_FORMAT_R8G8B8_UNORM; break;
		case 4: format = VK_FORMAT_R8G8B8A8_UNORM; break;
	}

	match(m_ctx->CreateImage(GvkImageCreateInfo::Image2D(format,
		x, y, VK_IMAGE_USAGE_SAMPLED_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)), img,
		image = img.value(),
		return std::nullopt;
	);

	ptr<gvk::Buffer> staging_buffer;
	match(m_ctx->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		x * y * comp, GVK_HOST_WRITE_SEQUENTIAL),buf,
		staging_buffer = buf.value(),
		return std::nullopt
	);

	staging_buffer->Write(data, 0, x * y * comp);

	VkResult vr = m_upload_queue->SubmitTemporalCommand
	(
		[&](VkCommandBuffer cmd)
		{
			GvkBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT)
			.ImageBarrier(image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				0,
				VK_ACCESS_TRANSFER_WRITE_BIT)
			.Emit(cmd);
			
			VkBufferImageCopy copy_info{};
			copy_info.bufferOffset = 0;
			copy_info.bufferRowLength = 0;
			copy_info.bufferImageHeight = 0;
			copy_info.imageExtent = image->Info().extent;
			copy_info.imageOffset = { 0 };
			copy_info.imageSubresource.aspectMask = gvk::GetAllAspects(image->Info().format);
			copy_info.imageSubresource.baseArrayLayer = 0;
			copy_info.imageSubresource.layerCount = image->Info().arrayLayers;
			copy_info.imageSubresource.mipLevel = 0;

			vkCmdCopyBufferToImage(cmd, staging_buffer->GetBuffer(),
				image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);

			GvkBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
				.ImageBarrier(image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT).Emit(cmd);
		},
		gvk::SemaphoreInfo(),
		NULL,
		true
	);

	if (vr == VK_SUCCESS) return image;
	
	return std::nullopt;
}
