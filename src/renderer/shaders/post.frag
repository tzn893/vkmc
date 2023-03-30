#version 450

layout(binding = 0) uniform sampler2D off_screen_buffer;

layout(location = 0) in vec2 i_uv;

layout(location = 0) out vec4 o_color;

vec3 tone_mapping(vec3 c)
{
    //TODO implement tone mapping
    return c;
}


void main()
{
    vec3 color = texture(off_screen_buffer,i_uv).rgb;
    color = tone_mapping(color);
    o_color = vec4(color,1);
}