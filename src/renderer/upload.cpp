#include "upload.h"
#include "stb_image.h"



UploadQueue::UploadQueue(ptr<gvk::CommandQueue>& queue,ptr<gvk::Context>& ctx)
{
	m_upload_queue = queue;
	m_ctx = ctx;
}

//prevent memory leak
struct PtrGaurd
{
	PtrGaurd(void* data)
	{
		this->data = data;
	}

	PtrGaurd(PtrGaurd&& pg) noexcept 
	{
		data = NULL;
		std::swap(data, pg.data);
	}

	PtrGaurd& operator=(PtrGaurd&& pg) noexcept 
	{
		std::swap(pg.data, data);
		return *this;
	}

	PtrGaurd()
	{
		data = NULL;
	}

	~PtrGaurd()
	{
		if (data != NULL) 
		{ 
			free(data);
		}
		data = NULL;
	}

	void* data;
};


//reason https://www.reddit.com/r/vulkan/comments/w69mnb/how_can_i_create_an_image_with_nonalpha_format/
//most modern GPU doesn't support R8G8B8 format,so we use R8G8B8A8 format simulate it
bool WriteImageData(void* dst,void* src,int width,int height,int comp)
{

	if (comp == 4)
	{
		memcpy(dst, src, width * height * comp);
	}
	else
	{
		u8* dstp = (u8*)dst, * srcp = (u8*)src;
		for (u32 y = 0;y < height;y++)
		{
			for (u32 x =0;x < width;x++)
			{
				dstp[0] = srcp[0];
				dstp[1] = srcp[1];
				dstp[2] = srcp[2];
				dstp[3] = 255;

				dstp += 4;
				srcp += 4;
			}
		}
	}

	return true;
}



opt<ptr<gvk::Image>> UploadQueue::UploadImage2D(const std::string& image_path)
{
	int x, y, comp;
	PtrGaurd data = stbi_load(image_path.c_str(), &x, &y, &comp, 4);

	if (data.data == nullptr)
	{
		return std::nullopt;
	}

	ptr<gvk::Image> image;
	
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	match(m_ctx->CreateImage(GvkImageCreateInfo::Image2D(format,
		x, y, VK_IMAGE_USAGE_SAMPLED_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)), img,
		image = img.value(),
		return std::nullopt;
	);

	ptr<gvk::Buffer> staging_buffer;
	match(m_ctx->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		x * y * 4, GVK_HOST_WRITE_RANDOM),buf,
		staging_buffer = buf.value(),
		return std::nullopt
	);

	void* staging_buffer_pt;
	match
	(
		staging_buffer->Map(), pt,
		staging_buffer_pt = pt.value(),
		return std::nullopt;
	)

	WriteImageData(staging_buffer_pt, data.data, x, y, comp);

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
			copy_info.imageSubresource.layerCount = 1;
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

opt<ptr<gvk::Image>> UploadQueue::UploadImageCube(const CubeMapDesc& desc)
{
	PtrGaurd pos_x, neg_x, pos_y, neg_y, pos_z, neg_z;
	int width, height, comp;

	{
		pos_x = std::move(PtrGaurd(stbi_load(desc.right.c_str(), &width, &height, &comp, 4)));

		if (pos_x.data == NULL)
		{
			return std::nullopt;
		}
	}

	{
		int n_width, n_height, n_comp;
		pos_y = std::move(PtrGaurd(stbi_load(desc.up.c_str(), &n_width, &n_height, &n_comp, 4)));

		if (pos_y.data == NULL || n_width != width || n_height != height || n_comp != comp)
		{
			return std::nullopt;
		}
	}

	{
		int n_width, n_height, n_comp;
		pos_z = std::move(PtrGaurd(stbi_load(desc.front.c_str(), &n_width, &n_height, &n_comp, 4)));

		if (pos_z.data == NULL || n_width != width || n_height != height || n_comp != comp)
		{
			return std::nullopt;
		}
	}

	{
		int n_width, n_height, n_comp;
		neg_x = std::move(PtrGaurd(stbi_load(desc.left.c_str(), &n_width, &n_height, &n_comp, 4)));

		if (neg_x.data == NULL || n_width != width || n_height != height || n_comp != comp)
		{
			return std::nullopt;
		}
	}

	{
		int n_width, n_height, n_comp;
		neg_y = std::move(PtrGaurd(stbi_load(desc.down.c_str(), &n_width, &n_height, &n_comp, 4)));

		if (neg_y.data == NULL || n_width != width || n_height != height || n_comp != comp)
		{
			return std::nullopt;
		}
	}

	{
		int n_width, n_height, n_comp;
		neg_z = std::move(PtrGaurd(stbi_load(desc.back.c_str(), &n_width, &n_height, &n_comp, 4)));

		if (pos_y.data == NULL || n_width != width || n_height != height || n_comp != comp)
		{
			return std::nullopt;
		}
	}

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	

	int image_size = width * height * 4;
	ptr<gvk::Buffer> staging_buffer;
	match
	(
		m_ctx->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, image_size * 6, GVK_HOST_WRITE_RANDOM), buf,
		staging_buffer = buf.value(),
		return std::nullopt;
	);

	u8* staging_buffer_pt;
	match
	(
		staging_buffer->Map(), sbpt,
		staging_buffer_pt = (u8*)(sbpt.value()),
		return std::nullopt;
	)

	//how to arrange textures: (+x,-x,+y,-y,+z,-z)
	//https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap16.html#_cube_map_face_selection
	WriteImageData(staging_buffer_pt, pos_x.data, width, height, comp);
	WriteImageData(staging_buffer_pt + image_size, neg_x.data, width, height, comp);
	WriteImageData(staging_buffer_pt + image_size * 2, pos_y.data, width, height, comp);
	WriteImageData(staging_buffer_pt + image_size * 3, neg_y.data, width, height, comp);
	WriteImageData(staging_buffer_pt + image_size * 4, pos_z.data, width, height, comp);
	WriteImageData(staging_buffer_pt + image_size * 5, neg_z.data, width, height, comp);

	ptr<gvk::Image>	image;
	match
	(
		m_ctx->CreateImage(GvkImageCreateInfo::ImageCube(format,width,height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)), img,
		image = img.value(),
		return std::nullopt
	);

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

			for (u32 i = 0; i < 6; i++)
			{
				VkBufferImageCopy copy_info{};
				copy_info.bufferOffset = image_size * i;
				copy_info.bufferRowLength = 0;
				copy_info.bufferImageHeight = 0;
				copy_info.imageExtent = image->Info().extent;
				copy_info.imageOffset = { 0 };
				copy_info.imageSubresource.aspectMask = gvk::GetAllAspects(image->Info().format);
				copy_info.imageSubresource.baseArrayLayer = i;
				copy_info.imageSubresource.layerCount = 1;
				copy_info.imageSubresource.mipLevel = 0;

				vkCmdCopyBufferToImage(cmd, staging_buffer->GetBuffer(),
					image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);
			}

			GvkBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
			.ImageBarrier(image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT)
			.Emit(cmd);
		},
		gvk::SemaphoreInfo::None(),
		NULL,
		true
	);

	if (vr != VK_SUCCESS)
	{
		return std::nullopt;
	}

	return image;
}
