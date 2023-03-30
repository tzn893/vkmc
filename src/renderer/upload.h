#pragma once
#include "common.h"
#include "gvk.h"


class UploadQueue
{
public:
	UploadQueue(ptr<gvk::CommandQueue>& queue,ptr<gvk::Context>& ctx);

	opt<ptr<gvk::Image>> UploadImage2D(const std::string& image_path);

private:
	ptr<gvk::CommandQueue> m_upload_queue;
	ptr<gvk::Context>	   m_ctx;
};
