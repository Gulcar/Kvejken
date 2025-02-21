#version 330 core

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;
layout (location = 3) in int a_is_text;
layout (location = 4) in int a_texture_index;

uniform mat4 u_view_proj;

out vec2 v_uv;
out vec4 v_color;
flat out int v_is_text;
flat out int v_texture_index;

void main()
{
    gl_Position = u_view_proj * vec4(a_pos, 0.0, 1.0);
    v_uv = a_uv;
    v_color = a_color;
    v_is_text = a_is_text;
    v_texture_index = a_texture_index;
}

