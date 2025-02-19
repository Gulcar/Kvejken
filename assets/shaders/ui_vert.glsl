#version 330 core

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in int a_texture_index;

uniform mat4 u_view_proj;

out vec2 v_uv;
flat out int v_texture_index;

void main()
{
    gl_Position = u_view_proj * vec4(a_pos, 0.0, 1.0);
    v_uv = a_uv;
    v_texture_index = a_texture_index;
}

