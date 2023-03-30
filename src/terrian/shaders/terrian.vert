#version 450

#include "terrian_shader_common.h"

layout(location = 0) out int    o_face_idx;
layout(location = 1) out int    o_mat_idx;
layout(location = 2) out vec3   o_world_pos;

void main()
{
    o_face_idx  = v_face_idx;
    o_mat_idx   = v_mat_idx;
    o_world_pos = v_pos;
}