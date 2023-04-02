#version 450

layout(location = 0) in vec3 i_world_pos;
layout(location = 0) out vec4 o_color;

layout(binding = 0,set = 0) uniform samplerCube sky_texture;

void main()
{
    o_color = texture(sky_texture,normalize(i_world_pos));
}