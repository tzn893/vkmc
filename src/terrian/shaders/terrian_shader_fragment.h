#ifndef TERRIAN_SHADER_FRAGMENT
#define TERRIAN_SHADER_FRAGMENT

#include "shader/shader_common.h"

struct TerrianMaterial
{
	ivec4 terrian_atlas_offset[6];
};

//TODO better memory layout
UNIFORM_BUFFER(TerrianMaterials, 0, 0)
	vec4 terrian_atlas_stride;
	TerrianMaterial terrian_material_props[128];
UNIFORM_BUFFER_END(TerrianMaterials, terrian_materials)

#endif