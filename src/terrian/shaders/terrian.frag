#version 450

#include "terrian_shader_common.h"
#include "terrian_shader_fragment.h"

layout(location = 0) in vec3        i_world_normal;
layout(location = 1) in vec3        i_world_pos;
//prevent interpolation
layout(location = 2) in flat int    i_mat_idx;
layout(location = 3) in vec2        i_uv;
//prevent interpolation
layout(location = 4) in flat int    i_face_idx;

layout(binding = 1,set = 0) uniform sampler2D terrian_altas;

layout(location = 0) out vec4 o_color;

void main()
{
    ivec2 ioffset = terrian_materials.terrian_material_props[i_mat_idx].terrian_atlas_offset[i_face_idx].xy;
    
    //prevent sampling edge of textures
    vec2 uv = i_uv * (1 - terrian_materials.terrian_atlas_stride.zw) + terrian_materials.terrian_atlas_stride.zw;
    uv = (vec2(ioffset.x,ioffset.y) + uv) * terrian_materials.terrian_atlas_stride.xy;

    o_color = texture(terrian_altas, uv); 
}