#include "robject.h"

GraphicsObjectContext& GraphicsObjectContext::BindDescriptorSet(gvk::DescriptorSet* set)
{
	vkmc_assert(set != nullptr);

	u32 set_idx = set->GetSetIndex();
	//TODO currently we only support 8 descriptor sets
	vkmc_assert(set_idx < VKMC_MAX_DESCRIPTOR_SET_COUNT);
	descriptor_sets[set_idx] = set;

	return *this;
}