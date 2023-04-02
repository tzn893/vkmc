#ifndef SKYBOX_SHADER_COMMON
#define SKYBOX_SHADER_COMMON

#include "shader/shader_common.h"

VERTEX_INPUT(SkyboxVertex)
VERTEX_ATTRIBUTE(0, vec3, pos)
VERTEX_INPUT_END(Vertex)

PUSH_CONSTANT(SkyboxPushConstants)
mat4	p_vp;
PUSH_CONSTANT_END(VkmcPushConstants)

#endif /*SKYBOX_SHADER_COMMON*/