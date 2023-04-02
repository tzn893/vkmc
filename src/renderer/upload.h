#pragma once
#include "common.h"
#include "gvk.h"


struct CubeMapDesc
{
	std::string right;
	std::string left;
	std::string up;
	std::string down;
	std::string front;
	std::string back;
};

class UploadQueue
{
public:
	UploadQueue(ptr<gvk::CommandQueue>& queue,ptr<gvk::Context>& ctx);

	opt<ptr<gvk::Image>> UploadImage2D(const std::string& image_path);

	opt<ptr<gvk::Image>> UploadImageCube(const CubeMapDesc& desc);

private:
	ptr<gvk::CommandQueue> m_upload_queue;
	ptr<gvk::Context>	   m_ctx;
};
