#include "shader/shader_common.h"

VERTEX_INPUT(Vertex)

VERTEX_ATTRIBUTE(0, vec3, pos);
VERTEX_ATTRIBUTE(2, vec3, normal);
VERTEX_ATTRIBUTE(1, vec2, uv);

VERTEX_INPUT_END(Vertex)

PUSH_CONSTANT(VkmcPushConstants)
//model->view->projection matrix
mat4	mvp;
//transpose of mvp matrix
mat4    transpose_mvp;
vec3	camera_position;
float	time;
PUSH_CONSTANT_END(VkmcPushConstants)