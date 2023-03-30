#ifndef TERRIAN_SHADER_COMMON
#define TERRIAN_SHADER_COMMON

#include "shader/shader_common.h"

VERTEX_INPUT(TerrianVertex)
	VERTEX_ATTRIBUTE(0, vec3, v_pos)
	VERTEX_ATTRIBUTE(1, int,  v_face_idx)
	VERTEX_ATTRIBUTE(2, int,  v_mat_idx)
VERTEX_INPUT_END(TerrianVertex)

PUSH_CONSTANT(TerrianPushConstant)
		mat4	p_mvp;
		vec3	p_camera_position;
		float	p_time;
PUSH_CONSTANT_END(TerrianPushConstant)

#endif
