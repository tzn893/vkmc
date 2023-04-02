#version 450
#include "skybox_shader_common.h"

layout(location = 0) out vec3 o_world_pos;

void main()
{
    gl_Position = p_vp * vec4(pos, 1);

    o_world_pos = pos;
    gl_Position.z = gl_Position.w * 0.999999f;
}