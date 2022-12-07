#pragma once
#include "gvk.h"
#include "alloc/arena.h"

constexpr u32 VKMC_MAX_DESCRIPTOR_SET_COUNT = 8;

struct RenderInitContext
{
	gvk::Context*				context;
	gvk::DescriptorAllocator*	descriptor_allocator;
	ptr<gvk::RenderPass>		main_render_pass;
	u32							forward_pass_idx;
	u32							transparent_pass_idx;
};

struct RenderFinalizeContext
{
	gvk::Context*				context;
};

struct RenderContext 
{
	gvk::Context*		context;
	VkCommandBuffer		main_cmd_buffer;
	gvk::SemaphoreInfo*	main_queue_semaphore;
};

enum RenderObjectType
{
	RENDER_OBJECT_FORWARD,
	RENDER_OBJECT_TRANSPARENT,
	RENDER_OBJECT_CUSTOM
};

class RenderObject
{
public:
	RenderObject(RenderObjectType type) : m_Type(type) {}

	RenderObjectType GetType() { return m_Type; }

	virtual ~RenderObject() {}
private:
	RenderObjectType m_Type;
};

struct GraphicsObjectContext
{
	gvk::Pipeline*				graphics_pipeline;
	gvk::DescriptorSet*			descriptor_sets[VKMC_MAX_DESCRIPTOR_SET_COUNT];
	u32							descriptor_set_count;

	GraphicsObjectContext&		BindDescriptorSet(gvk::DescriptorSet* set);
};

class ForwardRenderObject : public RenderObject
{
public:
	ForwardRenderObject() : RenderObject(RENDER_OBJECT_FORWARD) {}

	virtual GraphicsObjectContext	Context() = 0;

	virtual void			Forward(RenderContext& ctx) = 0;
};

enum CustomRenderObjectOrder
{
	CUSTOM_ROBJECT_ORDERER_BEFORE_ALL,
	CUSTOM_ROBJECT_ORDERER_AFTER_FORWARD,
	CUSTOM_ROBJECT_ORDERER_AFTER_TRANSPARENT,
	CUSTOM_ROBJECT_ORDERER_AFTER_ALL = CUSTOM_ROBJECT_ORDERER_AFTER_TRANSPARENT
};

class CustomRenderObject : public RenderObject
{
public:
	CustomRenderObject(CustomRenderObjectOrder order) : RenderObject(RENDER_OBJECT_CUSTOM), m_Order(order) {}

	CustomRenderObjectOrder GetOrder() { return m_Order; }

	virtual void OnRender(RenderContext ctx) = 0;
private:
	CustomRenderObjectOrder m_Order;
};

class TransparentRenderObject : public RenderObject
{
public:
	TransparentRenderObject() : RenderObject(RENDER_OBJECT_TRANSPARENT) {}
};

class RenderComponent
{
public:
	virtual bool Activated() = 0;

	virtual bool Initialize(RenderInitContext ctx) = 0;

	virtual RenderObject* OnRender(MemoryArenaAllocator* allocator) = 0;

	virtual void Finalize(RenderFinalizeContext ctx) = 0;
};

