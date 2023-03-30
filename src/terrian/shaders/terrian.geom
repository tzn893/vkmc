#version 450
#include "terrian_shader_common.h"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in int     i_face_idx[];
layout(location = 1) in int     i_mat_idx[];
layout(location = 2) in vec3    i_world_pos[];

layout(location = 0) out vec3   o_world_normal;
layout(location = 1) out vec3   o_world_pos;
layout(location = 2) out int    o_mat_idx;
layout(location = 3) out vec2   o_uv;
layout(location = 4) out int    o_face_idx;

/// rule of face encoding on cube
/// <summary>
///     front face        back face
///     --------          -------
///	   /  1   / |        /  4   / |
///	  /      /  |       /      /  |
///	  ------- 2 |       ------- 5 |
///  |       |  /      |       |  |
///	 |   0   | /       |   3   |  /
///  |       |/        |       | /
///   -------           ------- 
///   numbering of faces
/// </summary>

vec3 face_offset[6] =
{
    vec3( 1.0f, 0.0f, 0.0f),
    vec3( 1.0f, 1.0f, 0.0f),
    vec3( 1.0f, 0.0f, 1.0f),
    vec3( 0.0f, 0.0f, 1.0f),
    vec3( 1.0f, 0.0f, 1.0f),
    vec3( 0.0f, 0.0f, 0.0f),
};


vec3 face_normal[6] = 
{
    vec3( 1.0f, 0.0f, 0.0f),
    vec3( 1.0f, 1.0f, 0.0f),
    vec3( 0.0f, 0.0f, 1.0f),
    vec3(-1.0f, 0.0f, 0.0f),
    vec3( 0.0f,-1.0f, 0.0f),
    vec3( 0.0f, 0.0f,-1.0f),
};

vec3 face_tangent[6] = 
{
    vec3( 0.0f, 0.0f, 1.0f),
    vec3( 0.0f, 0.0f, 1.0f),
    vec3(-1.0f, 0.0f, 0.0f),
    vec3( 0.0f, 0.0f,-1.0f),
    vec3( 0.0f, 0.0f,-1.0f),
    vec3( 1.0f, 0.0f, 0.0f)
};

vec3 face_bitangent[6] = 
{
    vec3( 0.0f, 1.0f, 0.0f),
    vec3(-1.0f, 0.0f, 0.0f),
    vec3( 0.0f, 1.0f, 0.0f),
    vec3( 0.0f, 1.0f, 0.0f),
    vec3(-1.0f, 0.0f, 0.0f),
    vec3( 0.0f, 1.0f, 0.0f)
};

void GenerateVertex(int face_idx,int mat_idx,vec3 world_pos,vec2 offset)
{
    o_world_normal  = face_normal[face_idx];
    vec3 tangent    = face_tangent[face_idx];
    vec3 bitangent  = face_bitangent[face_idx];

    o_world_pos = world_pos + face_offset[face_idx] + offset.x * tangent + offset.y * bitangent;
    gl_Position = p_mvp * vec4(o_world_pos, 1);

    o_mat_idx = mat_idx;
    o_uv = 1 - offset;

    o_face_idx = face_idx;
    EmitVertex();
}

void main()
{

    GenerateVertex(i_face_idx[0],i_mat_idx[0],i_world_pos[0],vec2( 1.0f, 1.0f));

    GenerateVertex(i_face_idx[0],i_mat_idx[0],i_world_pos[0],vec2( 1.0f, 0.0f));

    GenerateVertex(i_face_idx[0],i_mat_idx[0],i_world_pos[0],vec2( 0.0f, 1.0f));

    GenerateVertex(i_face_idx[0],i_mat_idx[0],i_world_pos[0],vec2( 0.0f, 0.0f));
}
