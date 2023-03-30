#version 450

//draw a whole quad on the whole screen
layout(location = 0) out vec2 o_uv;

vec2 pos[6] = {
    { 1,-1},{ 1, 1},{-1, 1},
    {-1,-1},{ 1,-1},{-1, 1}
};

vec2 uvs[6] = {
    {1,0},{1,1},{0,1},
    {0,0},{1,0},{0,1}
};

void main()
{
    gl_Position = vec4(pos[gl_VertexIndex],0,1);
    o_uv = uvs[gl_VertexIndex];
}